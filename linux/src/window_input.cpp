// M4: SDL3-based Wayland-first window/event abstraction implementation.

#include "linux_port/window_input.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <utility>

namespace fl_port {

namespace {

std::uint32_t map_sdl_keycode_to_vk(SDL_Keycode key) {
  // Letters and digits: SDL keycodes for printable chars generally match ASCII.
  if (key >= 'a' && key <= 'z') return static_cast<std::uint32_t>('A' + (key - 'a'));
  if (key >= 'A' && key <= 'Z') return static_cast<std::uint32_t>(key);
  if (key >= '0' && key <= '9') return static_cast<std::uint32_t>(key);
  // Function keys (SDLK_F1..F24)
  if (key >= SDLK_F1 && key <= SDLK_F24) return 0x70u + static_cast<std::uint32_t>(key - SDLK_F1); // VK_F1+
  switch (key) {
    case SDLK_RETURN: case SDLK_RETURN2: return 0x0D; // VK_RETURN
    case SDLK_ESCAPE: return 0x1B; // VK_ESCAPE
    case SDLK_BACKSPACE: return 0x08;
    case SDLK_TAB: return 0x09;
    case SDLK_SPACE: return 0x20;
    case SDLK_MINUS: return 0xBD; // VK_OEM_MINUS
    case SDLK_EQUALS: return 0xBB;
    case SDLK_LEFTBRACKET: return 0xDB;
    case SDLK_RIGHTBRACKET: return 0xDD;
    case SDLK_BACKSLASH: return 0xDC;
    case SDLK_SEMICOLON: return 0xBA;
    case SDLK_APOSTROPHE: return 0xDE;
    case SDLK_GRAVE: return 0xC0;
    case SDLK_COMMA: return 0xBC;
    case SDLK_PERIOD: return 0xBE;
    case SDLK_SLASH: return 0xBF;
    case SDLK_CAPSLOCK: return 0x14;
    case SDLK_PRINTSCREEN: return 0x2C;
    case SDLK_SCROLLLOCK: return 0x91;
    case SDLK_PAUSE: return 0x13;
    case SDLK_INSERT: return 0x2D;
    case SDLK_HOME: return 0x24;
    case SDLK_PAGEUP: return 0x21;
    case SDLK_DELETE: return 0x2E;
    case SDLK_END: return 0x23;
    case SDLK_PAGEDOWN: return 0x22;
    case SDLK_RIGHT: return 0x27;
    case SDLK_LEFT: return 0x25;
    case SDLK_DOWN: return 0x28;
    case SDLK_UP: return 0x26;
    case SDLK_NUMLOCKCLEAR: return 0x90;
    case SDLK_KP_DIVIDE: return 0x6F;
    case SDLK_KP_MULTIPLY: return 0x6A;
    case SDLK_KP_MINUS: return 0x6D;
    case SDLK_KP_PLUS: return 0x6B;
    case SDLK_KP_ENTER: return 0x0D;
    case SDLK_KP_1: return 0x61; case SDLK_KP_2: return 0x62; case SDLK_KP_3: return 0x63;
    case SDLK_KP_4: return 0x64; case SDLK_KP_5: return 0x65; case SDLK_KP_6: return 0x66;
    case SDLK_KP_7: return 0x67; case SDLK_KP_8: return 0x68; case SDLK_KP_9: return 0x69;
    case SDLK_KP_0: return 0x60; case SDLK_KP_PERIOD: return 0x6E;
    case SDLK_LCTRL: return 0xA2; case SDLK_RCTRL: return 0xA3;
    case SDLK_LSHIFT: return 0xA0; case SDLK_RSHIFT: return 0xA1;
    case SDLK_LALT: return 0xA4; case SDLK_RALT: return 0xA5;
    case SDLK_LGUI: return 0x5B; case SDLK_RGUI: return 0x5C;
    case SDLK_APPLICATION: return 0x5D;
    default: return 0;
  }
}

MouseButton sdl_button_to_enum(Uint8 b) {
  switch (b) {
    case SDL_BUTTON_LEFT: return MouseButton::Left;
    case SDL_BUTTON_MIDDLE: return MouseButton::Middle;
    case SDL_BUTTON_RIGHT: return MouseButton::Right;
    case SDL_BUTTON_X1: return MouseButton::X1;
    case SDL_BUTTON_X2: return MouseButton::X2;
    default: return MouseButton::Left;
  }
}

} // namespace

std::uint32_t sdl_key_to_win_vk(int sdl_keycode, int /*sdl_scancode*/) {
  return map_sdl_keycode_to_vk(static_cast<SDL_Keycode>(sdl_keycode));
}

Window::~Window() { destroy(); }

Window::Window(Window&& other) noexcept {
  *this = std::move(other);
}

Window& Window::operator=(Window&& other) noexcept {
  if (this == &other) return *this;
  destroy();
  window_ = other.window_; other.window_ = nullptr;
  window_id_ = other.window_id_; other.window_id_ = 0;
  state_ = std::move(other.state_);
  last_lifecycle_ = other.last_lifecycle_;
  cursor_visible_ = other.cursor_visible_;
  relative_mouse_ = other.relative_mouse_;
  mouse_grabbed_ = other.mouse_grabbed_;
  opened_gamepads_ = std::move(other.opened_gamepads_);
  sdl_video_initialized_ = other.sdl_video_initialized_; other.sdl_video_initialized_ = false;
  return *this;
}

bool Window::create(const WindowConfig& config) {
  destroy();
  state_ = InputState{};

  if (!SDL_WasInit(SDL_INIT_VIDEO)) {
    // Prefer Wayland, but allow fallback if unavailable (SDL will honor hint when possible).
    if (config.prefer_wayland) SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "wayland");
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMEPAD)) {
      // Retry without forcing wayland driver hint.
      SDL_ClearError();
      SDL_SetHint(SDL_HINT_VIDEO_DRIVER, nullptr);
      if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMEPAD)) {
        std::cerr << "[fl-port][window] SDL_Init failed: " << SDL_GetError() << "\n";
        return false;
      }
    }
    sdl_video_initialized_ = true;
  }

  SDL_WindowFlags flags = 0;
  if (config.resizable) flags |= SDL_WINDOW_RESIZABLE;
  if (config.vulkan) flags |= SDL_WINDOW_VULKAN;
  if (config.high_dpi) flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;
  if (!config.visible) flags |= SDL_WINDOW_HIDDEN;

  window_ = SDL_CreateWindow(config.title.c_str(), config.width, config.height, flags);
  if (!window_ && config.prefer_wayland) {
    // Fallback: restart video subsystem so driver hint change actually takes effect.
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    SDL_ResetHint(SDL_HINT_VIDEO_DRIVER);
    if (SDL_InitSubSystem(SDL_INIT_VIDEO)) {
      window_ = SDL_CreateWindow(config.title.c_str(), config.width, config.height, flags);
    }
  }
  if (!window_) {
    std::cerr << "[fl-port][window] SDL_CreateWindow failed: " << SDL_GetError() << "\n";
    if (sdl_video_initialized_) { SDL_QuitSubSystem(SDL_INIT_GAMEPAD | SDL_INIT_EVENTS | SDL_INIT_VIDEO); sdl_video_initialized_ = false; }
    return false;
  }

  window_id_ = SDL_GetWindowID(window_);
  state_.focused = (SDL_GetWindowFlags(window_) & SDL_WINDOW_INPUT_FOCUS) != 0;
  state_.mouse_in_window = (SDL_GetWindowFlags(window_) & SDL_WINDOW_MOUSE_FOCUS) != 0;
  last_lifecycle_ = WindowLifecycleEvent::Shown;

  // Open already-connected gamepads/controllers for mapping.
  int count = 0;
  SDL_JoystickID* ids = SDL_GetGamepads(&count);
  if (ids) {
    for (int i = 0; i < count; ++i) {
      SDL_Gamepad* gp = SDL_OpenGamepad(ids[i]);
      if (gp) opened_gamepads_.push_back(static_cast<std::uint32_t>(ids[i]));
    }
    SDL_free(ids);
  }

  std::cout << "[fl-port][window] created: platform=" << SDL_GetPlatform()
            << " driver=" << (SDL_GetCurrentVideoDriver() ? SDL_GetCurrentVideoDriver() : "<none>")
            << " id=" << window_id_ << " adapter=" << adapter_handle() << "\n";
  return true;
}

void Window::destroy() {
  // Close all opened gamepad handles to avoid leaks (especially when SDL was
  // initialized externally, where sdl_video_initialized_ == false).
  for (std::uint32_t id : opened_gamepads_) {
    SDL_Gamepad* gp = SDL_GetGamepadFromID(static_cast<SDL_JoystickID>(id));
    if (gp) SDL_CloseGamepad(gp);
  }
  opened_gamepads_.clear();
  if (window_) {
    SDL_DestroyWindow(window_);
    window_ = nullptr;
  }
  window_id_ = 0;
  if (sdl_video_initialized_) {
    SDL_QuitSubSystem(SDL_INIT_GAMEPAD | SDL_INIT_EVENTS | SDL_INIT_VIDEO);
    sdl_video_initialized_ = false;
  }
  state_ = InputState{};
  cursor_visible_ = true;
  relative_mouse_ = false;
  mouse_grabbed_ = false;
  fullscreen_ = false;
}

bool Window::is_open() const { return window_ != nullptr; }

void Window::reset_transient_state() {
  state_.mouse_dx_accum = 0;
  state_.mouse_dy_accum = 0;
}

bool Window::pump_events(const WindowCallbacks& callbacks) {
  if (!window_) return false;
  reset_transient_state();
  SDL_Event ev;
  while (SDL_PollEvent(&ev)) {
    handle_event(ev, callbacks);
  }
  return is_open() && !state_.close_requested;
}

void Window::handle_event(const SDL_Event& ev, const WindowCallbacks& callbacks) {
  switch (ev.type) {
    case SDL_EVENT_QUIT: {
      state_.close_requested = true;
      last_lifecycle_ = WindowLifecycleEvent::CloseRequested;
      if (callbacks.on_window) callbacks.on_window({WindowLifecycleEvent::CloseRequested, 0, 0});
      break;
    }
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
    case SDL_EVENT_WINDOW_DESTROYED: {
      if (ev.window.windowID == window_id_ || window_id_ == 0) {
        state_.close_requested = true;
        last_lifecycle_ = WindowLifecycleEvent::CloseRequested;
        if (callbacks.on_window) callbacks.on_window({WindowLifecycleEvent::CloseRequested, 0, 0});
      }
      break;
    }
    case SDL_EVENT_WINDOW_SHOWN:
    case SDL_EVENT_WINDOW_HIDDEN:
    case SDL_EVENT_WINDOW_EXPOSED:
    case SDL_EVENT_WINDOW_MOVED:
    case SDL_EVENT_WINDOW_RESIZED:
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
    case SDL_EVENT_WINDOW_MINIMIZED:
    case SDL_EVENT_WINDOW_MAXIMIZED:
    case SDL_EVENT_WINDOW_RESTORED:
    case SDL_EVENT_WINDOW_MOUSE_ENTER:
    case SDL_EVENT_WINDOW_MOUSE_LEAVE:
    case SDL_EVENT_WINDOW_FOCUS_GAINED:
    case SDL_EVENT_WINDOW_FOCUS_LOST:
    case SDL_EVENT_WINDOW_DISPLAY_CHANGED: {
      if (ev.window.windowID != window_id_) break;
      WindowEvent we{};
      we.data1 = ev.window.data1;
      we.data2 = ev.window.data2;
      switch (ev.type) {
        case SDL_EVENT_WINDOW_SHOWN: we.type = WindowLifecycleEvent::Shown; break;
        case SDL_EVENT_WINDOW_HIDDEN: we.type = WindowLifecycleEvent::Hidden; break;
        case SDL_EVENT_WINDOW_EXPOSED: we.type = WindowLifecycleEvent::Exposed; break;
        case SDL_EVENT_WINDOW_MOVED: we.type = WindowLifecycleEvent::Moved; break;
        case SDL_EVENT_WINDOW_RESIZED:
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: we.type = WindowLifecycleEvent::Resized; break;
        case SDL_EVENT_WINDOW_MINIMIZED: we.type = WindowLifecycleEvent::Minimized; break;
        case SDL_EVENT_WINDOW_MAXIMIZED: we.type = WindowLifecycleEvent::Maximized; break;
        case SDL_EVENT_WINDOW_RESTORED: we.type = WindowLifecycleEvent::Restored; break;
        case SDL_EVENT_WINDOW_MOUSE_ENTER: we.type = WindowLifecycleEvent::MouseEnter; state_.mouse_in_window = true; break;
        case SDL_EVENT_WINDOW_MOUSE_LEAVE: we.type = WindowLifecycleEvent::MouseLeave; state_.mouse_in_window = false; break;
        case SDL_EVENT_WINDOW_FOCUS_GAINED: we.type = WindowLifecycleEvent::FocusGained; state_.focused = true; break;
        case SDL_EVENT_WINDOW_FOCUS_LOST: we.type = WindowLifecycleEvent::FocusLost; state_.focused = false; break;
        case SDL_EVENT_WINDOW_DISPLAY_CHANGED: we.type = WindowLifecycleEvent::DisplayChanged; break;
        default: break;
      }
      last_lifecycle_ = we.type;
      if (callbacks.on_window) callbacks.on_window(we);
      break;
    }
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP: {
      if (ev.key.windowID != window_id_) break;
      KeyEvent ke{};
      ke.action = (ev.type == SDL_EVENT_KEY_DOWN) ? InputAction::Press : InputAction::Release;
      ke.sdl_scancode = static_cast<int>(ev.key.scancode);
      ke.sdl_keycode = static_cast<int>(ev.key.key);
      ke.win_vk = map_sdl_keycode_to_vk(ev.key.key);
      ke.repeat = ev.key.repeat;
      ke.modifiers = static_cast<std::uint16_t>(ev.key.mod);
      if (ke.sdl_scancode >= 0 && ke.sdl_scancode < 512) {
        state_.scancodes_down[static_cast<std::size_t>(ke.sdl_scancode)] = (ke.action == InputAction::Press);
      }
      if (ke.win_vk < 256) {
        state_.vk_down[ke.win_vk] = (ke.action == InputAction::Press);
        // Mirror specific modifier into generic Win32 counterpart so legacy
        // engine code checking VK_SHIFT/CONTROL/MENU sees combined state.
        const bool pressed = (ke.action == InputAction::Press);
        if (ke.win_vk == 0xA0 || ke.win_vk == 0xA1) {
          // Recompute combined state accurately on both press and release.
          state_.vk_down[0x10] = state_.vk_down[0xA0] || state_.vk_down[0xA1];
        } else if (ke.win_vk == 0xA2 || ke.win_vk == 0xA3) {
          state_.vk_down[0x11] = state_.vk_down[0xA2] || state_.vk_down[0xA3];
        } else if (ke.win_vk == 0xA4 || ke.win_vk == 0xA5) {
          state_.vk_down[0x12] = state_.vk_down[0xA4] || state_.vk_down[0xA5];
        }
      }
      if (callbacks.on_key) callbacks.on_key(ke);
      break;
    }
    case SDL_EVENT_TEXT_INPUT: {
      if (ev.text.windowID != window_id_) break;
      TextInputEvent te{};
      if (ev.text.text) te.text = ev.text.text;
      if (callbacks.on_text) callbacks.on_text(te);
      break;
    }
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP: {
      if (ev.button.windowID != window_id_) break;
      MouseButtonEvent me{};
      me.action = (ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN) ? InputAction::Press : InputAction::Release;
      me.button = sdl_button_to_enum(ev.button.button);
      me.x = static_cast<int>(ev.button.x);
      me.y = static_cast<int>(ev.button.y);
      me.clicks = ev.button.clicks;
      if (ev.button.button < 8) state_.mouse_buttons_down[ev.button.button] = (me.action == InputAction::Press);
      state_.mouse_x = me.x; state_.mouse_y = me.y;
      if (callbacks.on_mouse_button) callbacks.on_mouse_button(me);
      break;
    }
    case SDL_EVENT_MOUSE_MOTION: {
      if (ev.motion.windowID != window_id_) break;
      MouseMotionEvent mm{};
      mm.x = static_cast<int>(ev.motion.x);
      mm.y = static_cast<int>(ev.motion.y);
      mm.dx = static_cast<int>(ev.motion.xrel);
      mm.dy = static_cast<int>(ev.motion.yrel);
      mm.button_state = ev.motion.state;
      state_.mouse_x = mm.x; state_.mouse_y = mm.y;
      // Accumulate as float to preserve sub-pixel deltas (high-DPI/Wayland).
      state_.mouse_dx_accum += ev.motion.xrel;
      state_.mouse_dy_accum += ev.motion.yrel;
      if (callbacks.on_mouse_motion) callbacks.on_mouse_motion(mm);
      break;
    }
    case SDL_EVENT_MOUSE_WHEEL: {
      if (ev.wheel.windowID != window_id_) break;
      MouseWheelEvent mw{};
      mw.x = ev.wheel.x;
      mw.y = ev.wheel.y;
      mw.mouse_x = static_cast<int>(ev.wheel.mouse_x);
      mw.mouse_y = static_cast<int>(ev.wheel.mouse_y);
      if (callbacks.on_mouse_wheel) callbacks.on_mouse_wheel(mw);
      break;
    }
    case SDL_EVENT_GAMEPAD_ADDED: {
      SDL_Gamepad* gp = SDL_OpenGamepad(ev.gdevice.which);
      if (gp) opened_gamepads_.push_back(static_cast<std::uint32_t>(ev.gdevice.which));
      break;
    }
    case SDL_EVENT_GAMEPAD_REMOVED: {
      const auto id = static_cast<SDL_JoystickID>(ev.gdevice.which);
      SDL_Gamepad* gp = SDL_GetGamepadFromID(id);
      if (gp) SDL_CloseGamepad(gp);
      const auto removed = static_cast<std::uint32_t>(id);
      auto it = std::remove(opened_gamepads_.begin(), opened_gamepads_.end(), removed);
      opened_gamepads_.erase(it, opened_gamepads_.end());
      break;
    }
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
    case SDL_EVENT_GAMEPAD_BUTTON_UP: {
      ControllerButtonEvent cb{};
      cb.action = (ev.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) ? InputAction::Press : InputAction::Release;
      cb.which = static_cast<int>(ev.gbutton.which);
      cb.button = ev.gbutton.button;
      if (callbacks.on_controller_button) callbacks.on_controller_button(cb);
      break;
    }
    case SDL_EVENT_GAMEPAD_AXIS_MOTION: {
      ControllerAxisEvent ca{};
      ca.which = static_cast<int>(ev.gaxis.which);
      ca.axis = ev.gaxis.axis;
      // Normalize SDL axis [-32768,32767] to [-1,1], triggers to [0,1].
      const float denom = (ev.gaxis.value < 0) ? 32768.0f : 32767.0f;
      ca.value = std::clamp(ev.gaxis.value / denom, -1.0f, 1.0f);
      if (callbacks.on_controller_axis) callbacks.on_controller_axis(ca);
      break;
    }
    default:
      break;
  }
}

void Window::run_loop(const std::function<bool(std::uint64_t, double)>& tick,
                      const WindowCallbacks& callbacks, int max_frames, double max_seconds,
                      bool yield_cpu) {
  using clock = std::chrono::steady_clock;
  const auto start = clock::now();
  auto last = start;
  std::uint64_t frame = 0;
  while (is_open() && !state_.close_requested) {
    if (max_frames > 0 && static_cast<int>(frame) >= max_frames) break;
    const auto now = clock::now();
    const double elapsed_s = std::chrono::duration<double>(now - start).count();
    if (max_seconds > 0.0 && elapsed_s >= max_seconds) break;
    const double dt_ms = std::chrono::duration<double, std::milli>(now - last).count();
    last = now;

    if (!pump_events(callbacks)) break;
    if (tick) {
      if (!tick(frame, dt_ms)) break;
    }
    ++frame;

    // Only yield when explicitly requested (harness/headless). During active
    // rendering, avoid SDL_Delay to prevent frame-pacing micro-stutter.
    if (yield_cpu) SDL_Delay(1);
  }
}

void Window::show() { if (window_) SDL_ShowWindow(window_); }
void Window::hide() { if (window_) SDL_HideWindow(window_); }
void Window::raise() { if (window_) SDL_RaiseWindow(window_); }
void Window::set_title(const std::string& title) { if (window_) SDL_SetWindowTitle(window_, title.c_str()); }

void Window::get_size(int& w, int& h) const {
  w = 0; h = 0;
  if (window_) SDL_GetWindowSize(window_, &w, &h);
}
void Window::set_size(int w, int h) {
  if (window_) SDL_SetWindowSize(window_, w, h);
}
bool Window::set_fullscreen(bool enabled) {
  if (!window_) return false;
  if (enabled == fullscreen_) return true;
  if (SDL_SetWindowFullscreen(window_, enabled)) {
    fullscreen_ = enabled;
    std::cout << "[fl-port][window] fullscreen=" << (fullscreen_ ? "on" : "off") << "\n";
    return true;
  }
  std::cerr << "[fl-port][window] SDL_SetWindowFullscreen failed: " << SDL_GetError() << "\n";
  return false;
}
bool Window::has_focus() const { return state_.focused; }
WindowLifecycleEvent Window::last_lifecycle_event() const { return last_lifecycle_; }

void Window::set_cursor_visible(bool visible) {
  cursor_visible_ = visible;
  if (visible) SDL_ShowCursor(); else SDL_HideCursor();
}
void Window::set_relative_mouse_mode(bool enabled) {
  relative_mouse_ = enabled;
  if (window_) SDL_SetWindowRelativeMouseMode(window_, enabled);
}
void Window::set_mouse_grab(bool grabbed) {
  mouse_grabbed_ = grabbed;
  if (window_) SDL_SetWindowMouseGrab(window_, grabbed);
}
bool Window::cursor_visible() const { return cursor_visible_; }
bool Window::relative_mouse_mode() const { return relative_mouse_; }
bool Window::mouse_grabbed() const { return mouse_grabbed_; }

} // namespace fl_port
