// M5 harness: native Linux graphics bring-up (scoped Vulkan backend behind
// IRenderPipeline). Creates an M4 window, initializes the render pipeline,
// and runs an engine-like frame loop that clears + presents N frames.

#include "linux_port/render_pipeline.h"
#include "linux_port/window_input.h"
#include "linux_port/platform.h"

#include <SDL3/SDL.h>

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

namespace {

}  // namespace

int main() {
  fl_port::log_message(fl_port::LogLevel::Info, "m5_graphics_harness starting");
  std::cout << "m5_graphics_harness\n";

  fl_port::WindowConfig wcfg;
  wcfg.title = "Freelancer M5 Graphics Harness";
  wcfg.width = 960;
  wcfg.height = 540;
  wcfg.resizable = true;
  wcfg.visible = true;
  wcfg.prefer_wayland = true;
  wcfg.vulkan = true;

  fl_port::Window window;
  if (!window.create(wcfg)) {
    std::cerr << "[FAIL] failed to create native window\n";
    return 2;
  }
  std::cout << "[ok] native window created (driver=" << SDL_GetCurrentVideoDriver() << ")\n";

  fl_port::RenderPipelineConfig rcfg;
  rcfg.initial_width = 960;
  rcfg.initial_height = 540;
  rcfg.clear_r = 0.08f;
  rcfg.clear_g = 0.12f;
  rcfg.clear_b = 0.20f;

  fl_port::RenderPipeline rp;
  if (!rp.init_for_window(&window, rcfg)) {
    std::cerr << "[FAIL] render pipeline init failed: " << rp.last_error() << "\n";
    window.destroy();
    return 3;
  }
  std::cout << "[ok] render pipeline initialized (swapchain images=" << rp.swapchain_image_count() << ")\n";

  // Engine-like frame loop: pump input, render+present frame, exit after N frames or timeout.
  const int target_frames = 120;
  int presented = 0;
  bool ok = true;
  fl_port::WindowCallbacks cb;
  cb.on_window = [&](const fl_port::WindowEvent& e) {
    if (e.type == fl_port::WindowLifecycleEvent::CloseRequested) {
      ok = false;
    }
    if (e.type == fl_port::WindowLifecycleEvent::Resized) {
      rp.resize(e.data1, e.data2);
    }
  };

  window.run_loop([&](std::uint64_t frame_index, double /*dt_ms*/) {
    if (!ok) return false;
    if (!rp.render_frame(static_cast<float>(frame_index) / 60.0f)) {
      std::cerr << "[FAIL] render_frame failed: " << rp.last_error() << "\n";
      ok = false;
      return false;
    }
    ++presented;
    if (presented >= target_frames) return false;
    return true;
  }, cb, target_frames, 10.0, true);

  std::cout << "[ok] presented frames: " << presented << "/" << target_frames << "\n";
  if (presented < target_frames) {
    std::cerr << "[FAIL] did not reach target frame count (ok=" << (ok ? "yes" : "no") << ")\n";
    rp.shutdown();
    window.destroy();
    return 4;
  }
  std::cout << "[ok] minimal in-engine frame rendered and presented successfully\n";

  rp.shutdown();
  window.destroy();
  fl_port::log_message(fl_port::LogLevel::Info, "m5_graphics_harness complete");
  std::cout << "m5_graphics_harness: PASS\n";
  return 0;
}
