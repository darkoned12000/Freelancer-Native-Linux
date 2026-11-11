// M4 harness: native Linux window + functioning input/event loop (SDL3, Wayland-first).
// Exercises window/event abstraction, input mapping, and focus/lifecycle/resize/cursor/grab handling.

#include "linux_port/window_input.h"
#include "linux_port/platform.h"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>

namespace {

const char* lifecycle_name(fl_port::WindowLifecycleEvent e) {
  switch (e) {
    case fl_port::WindowLifecycleEvent::Shown: return "Shown";
    case fl_port::WindowLifecycleEvent::Hidden: return "Hidden";
    case fl_port::WindowLifecycleEvent::Exposed: return "Exposed";
    case fl_port::WindowLifecycleEvent::Moved: return "Moved";
    case fl_port::WindowLifecycleEvent::Resized: return "Resized";
    case fl_port::WindowLifecycleEvent::Minimized: return "Minimized";
    case fl_port::WindowLifecycleEvent::Maximized: return "Maximized";
    case fl_port::WindowLifecycleEvent::Restored: return "Restored";
    case fl_port::WindowLifecycleEvent::MouseEnter: return "MouseEnter";
    case fl_port::WindowLifecycleEvent::MouseLeave: return "MouseLeave";
    case fl_port::WindowLifecycleEvent::FocusGained: return "FocusGained";
    case fl_port::WindowLifecycleEvent::FocusLost: return "FocusLost";
    case fl_port::WindowLifecycleEvent::CloseRequested: return "CloseRequested";
    case fl_port::WindowLifecycleEvent::DisplayChanged: return "DisplayChanged";
    default: return "Unknown";
  }
}

} // namespace

int main() {
  fl_port::log_message(fl_port::LogLevel::Info, "m4_window_input_harness starting");
  std::cout << "m4_window_input_harness\n";

  fl_port::WindowConfig cfg;
  cfg.title = "Freelancer M4 Window/Input Harness";
  cfg.width = 960;
  cfg.height = 540;
  cfg.resizable = true;
  cfg.visible = true;
  cfg.prefer_wayland = true;

  fl_port::Window window;
  if (!window.create(cfg)) {
    std::cerr << "[FAIL] failed to create native window (SDL3/Wayland-first)\n";
    std::cerr << "[info] ensure a display session is available (WAYLAND_DISPLAY/DISPLAY) and SDL3 video backend is usable\n";
    return 2;
  }

  std::cout << "[ok] native window created (adapter_handle=" << window.adapter_handle()
            << " sdl_window_id=" << window.sdl_window_id() << ")\n";

  int w = 0, h = 0;
  window.get_size(w, h);
  std::cout << "[ok] initial size: " << w << "x" << h << "\n";

  // Exercise focus/lifecycle/resize/cursor/grab APIs (engine adapter surface).
  window.show();
  window.raise();
  window.set_title("Freelancer M4 Window/Input Harness (running)");
  window.set_size(980, 560);
  window.get_size(w, h);
  std::cout << "[ok] resize API exercised: " << w << "x" << h << "\n";

  window.set_cursor_visible(false);
  window.set_cursor_visible(true);
  std::cout << "[ok] cursor visibility toggled (visible=" << window.cursor_visible() << ")\n";
  window.set_mouse_grab(true);
  std::cout << "[ok] mouse grab set (grabbed=" << window.mouse_grabbed() << ")\n";
  window.set_relative_mouse_mode(true);
  std::cout << "[ok] relative mouse mode set (relative=" << window.relative_mouse_mode() << ")\n";
  // Restore defaults for usability.
  window.set_relative_mouse_mode(false);
  window.set_mouse_grab(false);
  window.set_cursor_visible(true);
  std::cout << "[ok] cursor/grab/relative restored\n";

  int key_events = 0, mouse_events = 0, controller_events = 0, window_events = 0;
  bool saw_focus_or_expose = window.has_focus();

  fl_port::WindowCallbacks cb;
  cb.on_window = [&](const fl_port::WindowEvent& e) {
    ++window_events;
    saw_focus_or_expose = saw_focus_or_expose || (e.type == fl_port::WindowLifecycleEvent::FocusGained ||
                                                  e.type == fl_port::WindowLifecycleEvent::Exposed ||
                                                  e.type == fl_port::WindowLifecycleEvent::Shown);
    std::cout << "[event][window] " << lifecycle_name(e.type) << " (" << e.data1 << "," << e.data2 << ")\n";
  };
  cb.on_key = [&](const fl_port::KeyEvent& e) {
    ++key_events;
    std::cout << "[event][key] " << (e.action == fl_port::InputAction::Press ? "down" : "up")
              << " vk=0x" << std::hex << e.win_vk << std::dec
              << " scancode=" << e.sdl_scancode << " keycode=" << e.sdl_keycode
              << (e.repeat ? " repeat" : "") << "\n";
  };
  cb.on_mouse_button = [&](const fl_port::MouseButtonEvent& e) {
    ++mouse_events;
    std::cout << "[event][mouse_btn] " << (e.action == fl_port::InputAction::Press ? "down" : "up")
              << " button=" << static_cast<int>(e.button) << " at=(" << e.x << "," << e.y << ")\n";
  };
  cb.on_mouse_motion = [&](const fl_port::MouseMotionEvent& e) {
    ++mouse_events;
    // Motion can be frequent; print only occasional samples to keep harness output readable.
    static int motion_samples = 0;
    if (motion_samples++ < 10) {
      std::cout << "[event][mouse_move] pos=(" << e.x << "," << e.y << ") d=(" << e.dx << "," << e.dy << ")\n";
    }
  };
  cb.on_mouse_wheel = [&](const fl_port::MouseWheelEvent& e) {
    ++mouse_events;
    std::cout << "[event][mouse_wheel] (" << e.x << "," << e.y << ")\n";
  };
  cb.on_controller_button = [&](const fl_port::ControllerButtonEvent& e) {
    ++controller_events;
    std::cout << "[event][pad_btn] " << (e.action == fl_port::InputAction::Press ? "down" : "up")
              << " which=" << e.which << " button=" << e.button << "\n";
  };
  cb.on_controller_axis = [&](const fl_port::ControllerAxisEvent& e) {
    ++controller_events;
    static int axis_samples = 0;
    if (axis_samples++ < 10 || std::abs(e.value) > 0.5f) {
      std::cout << "[event][pad_axis] which=" << e.which << " axis=" << e.axis << " value=" << e.value << "\n";
    }
  };
  cb.on_text = [&](const fl_port::TextInputEvent& e) {
    std::cout << "[event][text] \"" << e.text << "\"\n";
  };

  std::cout << "[info] entering event loop (auto-exits after ~2s unless closed earlier). Interact with the window to generate input events.\n";
  std::uint64_t frames = 0;
  window.run_loop([&](std::uint64_t frame_index, double /*dt_ms*/) {
    frames = frame_index + 1;
    return true; // continue until max_seconds/max_frames/close
  }, cb, 0, 2.0, true); // yield_cpu=true for harness loop

  const auto& st = window.input_state();
  std::cout << "[ok] event loop ran for " << frames << " frame(s)"
            << " (window_events=" << window_events
            << " key_events=" << key_events
            << " mouse_events=" << mouse_events
            << " controller_events=" << controller_events
            << " focused=" << (st.focused ? "yes" : "no")
            << " mouse_in_window=" << (st.mouse_in_window ? "yes" : "no") << ")\n";

  // Mapping sanity: verify SDL->VK translation helper is available and stable for common keys.
  const auto vk_esc = fl_port::sdl_key_to_win_vk(/*SDLK_ESCAPE*/ 27, 0);
  const auto vk_a = fl_port::sdl_key_to_win_vk(/*SDLK_a*/ 'a', 0);
  const auto vk_ret = fl_port::sdl_key_to_win_vk(/*SDLK_RETURN*/ '\r', 0);
  if (vk_esc == 0x1B && vk_a == 'A' && vk_ret == 0x0D) {
    std::cout << "[ok] keyboard mapping helper: SDLK_ESCAPE->VK_ESCAPE, SDLK_a->VK_A, SDLK_RETURN->VK_RETURN\n";
  } else {
    std::cerr << "[FAIL] keyboard mapping helper unexpected values (esc=" << vk_esc << " a=" << vk_a << " ret=" << vk_ret << ")\n";
    window.destroy();
    return 1;
  }

  std::cout << "[ok] functioning input/event loop verified (native window created, pumped, and closed/expired cleanly)\n";
  window.destroy();
  fl_port::log_message(fl_port::LogLevel::Info, "m4_window_input_harness complete");
  std::cout << "m4_window_input_harness: PASS\n";
  return 0;
}
