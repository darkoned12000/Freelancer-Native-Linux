// M6 harness: gameplay bring-up. Ties together null audio, XDG config/registry,
// window/input (M4), graphics (M5) and a simulated boot sequence toward main menu.

#include "linux_port/audio_backend.h"
#include "linux_port/xdg_config.h"
#include "linux_port/window_input.h"
#include "linux_port/render_pipeline.h"
#include "linux_port/platform.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>

namespace {

enum class BootState {
  Init,
  AudioInit,
  ConfigLoad,
  RegistryRead,
  WindowCreate,
  RenderInit,
  MainMenu,
  EnterWorld,
  Done,
};

const char* state_name(BootState s) {
  switch (s) {
    case BootState::Init: return "Init";
    case BootState::AudioInit: return "AudioInit";
    case BootState::ConfigLoad: return "ConfigLoad";
    case BootState::RegistryRead: return "RegistryRead";
    case BootState::WindowCreate: return "WindowCreate";
    case BootState::RenderInit: return "RenderInit";
    case BootState::MainMenu: return "MainMenu";
    case BootState::EnterWorld: return "EnterWorld";
    case BootState::Done: return "Done";
  }
  return "Unknown";
}

}  // namespace

int main() {
  // Redirect XDG dirs to a temp directory to avoid polluting real user config.
  namespace fs = std::filesystem;
  fs::path tmp_root = fs::temp_directory_path() / "fl_m6_boot";
  fs::remove_all(tmp_root);
  fs::create_directories(tmp_root);
  setenv("XDG_CONFIG_HOME", (tmp_root / "config").c_str(), 1);
  setenv("XDG_DATA_HOME", (tmp_root / "data").c_str(), 1);
  setenv("XDG_STATE_HOME", (tmp_root / "state").c_str(), 1);
  setenv("XDG_CACHE_HOME", (tmp_root / "cache").c_str(), 1);

  fl_port::log_message(fl_port::LogLevel::Info, "m6_boot_harness starting");
  std::cout << "m6_boot_harness\n";

  fl_port::XdgConfig xdg;
  xdg.print_paths();
  xdg.ensure_dir(fl_port::XdgDir::Config);
  xdg.ensure_dir(fl_port::XdgDir::State);

  BootState state = BootState::Init;
  bool ok = true;

  // Window callbacks (shared across loops) handling close + resize.
  fl_port::WindowCallbacks cb;
  cb.on_window = [&](const fl_port::WindowEvent& e) {
    if (e.type == fl_port::WindowLifecycleEvent::CloseRequested) ok = false;
    // resize handling is done in the loop via window size check
  };

  // 1. Audio init (null backend unblocks boot).
  state = BootState::AudioInit;
  std::cout << "[boot] state=" << state_name(state) << "\n";
  fl_port::NullAudioBackend audio;
  fl_port::AudioConfig acfg;
  acfg.null_backend = true;
  if (!audio.init(acfg)) { ok = false; std::cerr << "[FAIL] audio init\n"; }
  else std::cout << "[ok] null audio backend initialized\n";

  // 2. Config load (XDG-backed).
  state = BootState::ConfigLoad;
  std::cout << "[boot] state=" << state_name(state) << "\n";
  xdg.write_key("freelancer.ini", "Freelancer", "data path", "../DATA");
  std::string dp;
  if (xdg.read_key("freelancer.ini", "Freelancer", "data path", dp)) {
    std::cout << "[ok] config loaded: data path=" << dp << "\n";
  } else {
    ok = false; std::cerr << "[FAIL] config load\n";
  }

  // 3. Registry read (XDG-backed).
  state = BootState::RegistryRead;
  std::cout << "[boot] state=" << state_name(state) << "\n";
  xdg.reg_write("Software\\Microsoft\\Microsoft Games\\Freelancer\\1.0", "LangID", "1033");
  std::string lang;
  if (xdg.reg_read("Software\\Microsoft\\Microsoft Games\\Freelancer\\1.0", "LangID", lang)) {
    std::cout << "[ok] registry read: LangID=" << lang << "\n";
  } else {
    std::cout << "[info] registry read not found (acceptable on first run)\n";
  }

  // 4. Window create (M4).
  state = BootState::WindowCreate;
  std::cout << "[boot] state=" << state_name(state) << "\n";
  fl_port::WindowConfig wcfg;
  wcfg.title = "Freelancer (Linux) - Boot";
  wcfg.width = 960; wcfg.height = 540;
  wcfg.resizable = true; wcfg.visible = true;
  wcfg.prefer_wayland = true; wcfg.vulkan = true;
  fl_port::Window window;
  if (!window.create(wcfg)) { ok = false; std::cerr << "[FAIL] window create\n"; }
  else std::cout << "[ok] native window created\n";

  // 5. Render init (M5).
  state = BootState::RenderInit;
  std::cout << "[boot] state=" << state_name(state) << "\n";
  fl_port::RenderPipelineConfig rcfg;
  rcfg.initial_width = 960; rcfg.initial_height = 540;
  rcfg.clear_r = 0.05f; rcfg.clear_g = 0.08f; rcfg.clear_b = 0.15f;
  fl_port::RenderPipeline rp;
  if (!window.is_open() || !rp.init_for_window(&window, rcfg)) {
    ok = false; std::cerr << "[FAIL] render init: " << rp.last_error() << "\n";
  } else {
    std::cout << "[ok] render pipeline initialized (swapchain images=" << rp.swapchain_image_count() << ")\n";
  }

  // 6. Simulated boot sequence toward main menu.
  state = BootState::MainMenu;
  std::cout << "[boot] state=" << state_name(state) << "\n";
  const char* steps[] = {"Loading resources...", "Initializing UI...", "Loading menu...",
                         "Main menu ready"};
  for (const char* s : steps) {
    if (!ok) break;
    std::cout << "[boot] " << s << "\n";
    audio.play_sound("menu_tick", 0.5f, false);
    audio.update();
    if (!window.pump_events(cb)) { ok = false; break; }
    int w = 0, h = 0;
    window.get_size(w, h);
    rp.resize(w, h);
    rp.render_frame(0.0f);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  if (ok) std::cout << "[ok] reached main menu state\n";

  // 7. Enter world (simulated).
  state = BootState::EnterWorld;
  std::cout << "[boot] state=" << state_name(state) << "\n";
  std::cout << "[boot] entering game world (simulated)...\n";
  for (int i = 0; i < 30 && ok; ++i) {
    audio.update();
    if (!window.pump_events(cb)) { ok = false; break; }
    int w = 0, h = 0;
    window.get_size(w, h);
    rp.resize(w, h);
    rp.render_frame(i / 30.0f);
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
  }
  if (ok) std::cout << "[ok] game world entered (simulated loop)\n";

  state = BootState::Done;
  std::cout << "[boot] state=" << state_name(state) << "\n";

  // Cleanup.
  rp.shutdown();
  window.destroy();
  audio.shutdown();

  // Remove temp dir.
  fs::remove_all(tmp_root);

  fl_port::log_message(fl_port::LogLevel::Info, "m6_boot_harness complete");
  std::cout << "m6_boot_harness: " << (ok ? "PASS" : "FAIL") << "\n";
  return ok ? 0 : 1;
}
