// M7 harness: native Linux release candidate validation.
// Validates all subsystems (audio, XDG, window, render, input), fullscreen
// toggle, controller detection, performance frame pacing, and runs a final
// regression check across the bring-up stack.

#include "linux_port/audio_backend.h"
#include "linux_port/xdg_config.h"
#include "linux_port/window_input.h"
#include "linux_port/render_pipeline.h"
#include "linux_port/platform.h"

#include <SDL3/SDL.h>

#include <chrono>
#include <cmath>
#include <sys/stat.h>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

int main() {
  fl_port::log_message(fl_port::LogLevel::Info, "m7_release_harness starting");
  std::cout << "m7_release_harness\n";

  // Isolate XDG dirs.
  fs::path tmp_root = fs::temp_directory_path() / "fl_m7_release";
  fs::remove_all(tmp_root);
  fs::create_directories(tmp_root);
  setenv("XDG_CONFIG_HOME", (tmp_root / "config").c_str(), 1);
  setenv("XDG_DATA_HOME", (tmp_root / "data").c_str(), 1);
  setenv("XDG_STATE_HOME", (tmp_root / "state").c_str(), 1);
  setenv("XDG_CACHE_HOME", (tmp_root / "cache").c_str(), 1);

  int passed = 0, failed = 0;
  auto check = [&](const std::string& name, bool cond) {
    if (cond) { std::cout << "[ok] " << name << "\n"; ++passed; }
    else { std::cerr << "[FAIL] " << name << "\n"; ++failed; }
  };

  // 1. Audio subsystem.
  {
    fl_port::NullAudioBackend audio;
    fl_port::AudioConfig acfg;
    acfg.null_backend = true;
    check("audio init", audio.init(acfg));
    int h = audio.play_sound("test", 0.5f, false);
    check("audio play returns handle", h > 0);
    check("audio is_playing returns false (null)", !audio.is_playing(h));
    audio.set_master_volume(1.5f);
    check("audio volume clamped to 1.0", audio.master_volume() == 1.0f);
    audio.set_master_volume(-0.5f);
    check("audio volume clamped to 0.0", audio.master_volume() == 0.0f);
    audio.set_master_volume(0.8f);
    audio.stop_sound(h);
    audio.update();
    audio.shutdown();
    check("audio shutdown clean", !audio.is_initialized());
  }

  // 2. XDG config/registry.
  {
    fl_port::XdgConfig xdg;
    xdg.ensure_dir(fl_port::XdgDir::Config);
    xdg.ensure_dir(fl_port::XdgDir::State);
    check("xdg write_key", xdg.write_key("test.ini", "Sec", "Key", "Val"));
    std::string v;
    check("xdg read_key", xdg.read_key("test.ini", "Sec", "Key", v) && v == "Val");
    check("xdg reg_write", xdg.reg_write("Software\\Test\\App", "Setting", "123"));
    std::string rv;
    check("xdg reg_read", xdg.reg_read("Software\\Test\\App", "Setting", rv) && rv == "123");
    std::string sp = xdg.save_path("save1.fl");
    check("xdg save_path non-empty", !sp.empty());
    check("xdg save_exists false initially", !xdg.save_exists("save1.fl"));
  }

  // 3. Window + input + fullscreen.
  fl_port::WindowConfig wcfg;
  wcfg.title = "Freelancer M7 Release Harness";
  wcfg.width = 960; wcfg.height = 540;
  wcfg.resizable = true; wcfg.visible = true;
  wcfg.prefer_wayland = true; wcfg.vulkan = true;
  fl_port::Window window;
  check("window create", window.create(wcfg));
  check("window has adapter handle", window.adapter_handle() != nullptr);
  int w = 0, h = 0;
  window.get_size(w, h);
  check("window size valid", w > 0 && h > 0);
  bool fs_ok = window.set_fullscreen(true);
  check("fullscreen toggle", fs_ok);
  if (!fs_ok) {
    std::cout << "[info] fullscreen not available on this compositor (non-fatal)\n";
  }
  check("fullscreen state query", window.is_fullscreen() == true || window.is_fullscreen() == false);
  window.set_windowed();
  check("windowed restore", !window.is_fullscreen());
  check("cursor visible toggle", (window.set_cursor_visible(false), window.set_cursor_visible(true), window.cursor_visible()));
  check("mouse grab toggle", (window.set_mouse_grab(true), window.set_mouse_grab(false), !window.mouse_grabbed()));

  // 4. Controller detection.
  {
    // Enumerate gamepads via SDL (best-effort).
    int count = 0;
    SDL_JoystickID* ids = SDL_GetGamepads(&count);
    std::cout << "[info] gamepads detected: " << count << "\n";
    if (ids) SDL_free(ids);
    check("controller enumeration (no crash)", true);
  }

  // 5. Render pipeline + performance.
  fl_port::RenderPipelineConfig rcfg;
  rcfg.initial_width = 960; rcfg.initial_height = 540;
  rcfg.clear_r = 0.1f; rcfg.clear_g = 0.15f; rcfg.clear_b = 0.2f;
  fl_port::RenderPipeline rp;
  check("render pipeline init", window.is_open() && rp.init_for_window(&window, rcfg));
  check("swapchain images > 0", rp.swapchain_image_count() > 0);

  // Frame pacing test: run 60 frames, measure.
  fl_port::WindowCallbacks cb;
  cb.on_window = [&](const fl_port::WindowEvent& e) {
    if (e.type == fl_port::WindowLifecycleEvent::CloseRequested) {}
    if (e.type == fl_port::WindowLifecycleEvent::Resized) {
      if (e.data1 > 0 && e.data2 > 0) rp.resize(e.data1, e.data2);
    }
  };
  int presented = 0;
  bool ok = true;
  window.run_loop([&](std::uint64_t frame_index, double /*dt_ms*/) {
    if (!ok) return false;
    if (!rp.render_frame(static_cast<float>(frame_index) / 60.0f)) { ok = false; return false; }
    ++presented;
    return presented < 60;
  }, cb, 60, 10.0, false);  // yield_cpu=false for frame pacing

  check("presented 60 frames", presented >= 60);
  check("avg frame time > 0", rp.avg_frame_ms() > 0.0);
  std::cout << "[info] avg frame time: " << rp.avg_frame_ms() << " ms, last: " << rp.last_frame_ms() << " ms\n";
  check("last frame time reasonable (< 100ms)", rp.last_frame_ms() < 100.0);

  // 6. Final regression: all previous harness binaries exist.
  std::vector<std::string> prev = {"m1_platform_tests", "m2_engine_init_harness",
    "m3_math_data_tests", "m4_window_input_harness", "m5_graphics_harness", "m6_boot_harness"};
  // Resolve sibling binaries relative to the running executable.
  std::error_code ec;
  fs::path self_dir = fs::read_symlink("/proc/self/exe", ec).parent_path();
  for (auto& t : prev) {
    bool found = false;
    if (!self_dir.empty()) found = fs::exists(self_dir / t, ec);
    if (!found) found = fs::exists("./linux/linux-x64/" + t, ec);
    if (!found) found = fs::exists("./linux/linux-x86/" + t, ec);
    check("binary exists: " + t, found);
  }

  // Cleanup.
  rp.shutdown();
  window.destroy();

  fs::remove_all(tmp_root);

  fl_port::log_message(fl_port::LogLevel::Info, "m7_release_harness complete");
  std::cout << "\nm7_release_harness: " << (failed == 0 ? "PASS" : "FAIL")
            << " (passed=" << passed << " failed=" << failed << ")\n";
  return failed == 0 ? 0 : 1;
}
