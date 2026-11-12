#pragma once

// M4: SDL3-based Wayland-first window/event abstraction for Linux bring-up.
// Provides engine-facing adapters (avoid fake HWND assumptions by using
// explicit window handles/tokens) plus input/event loop integration hooks.

#include <array>
#include <bitset>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

struct SDL_Window;
union SDL_Event;

namespace fl_port {

enum class MouseButton : std::uint8_t {
  Left = 1,
  Middle = 2,
  Right = 3,
  X1 = 4,
  X2 = 5,
};

enum class InputAction {
  Press,
  Release,
};

struct KeyEvent {
  InputAction action{InputAction::Press};
  int sdl_scancode{0};
  int sdl_keycode{0};
  std::uint32_t win_vk{0};
  bool repeat{false};
  std::uint16_t modifiers{0};
};

struct MouseButtonEvent {
  InputAction action{InputAction::Press};
  MouseButton button{MouseButton::Left};
  int x{0};
  int y{0};
  int clicks{1};
};

struct MouseMotionEvent {
  int x{0};
  int y{0};
  int dx{0};
  int dy{0};
  std::uint32_t button_state{0};
};

struct MouseWheelEvent {
  float x{0.0f};
  float y{0.0f};
  int mouse_x{0};
  int mouse_y{0};
};

struct ControllerButtonEvent {
  InputAction action{InputAction::Press};
  int which{0};
  int button{0};
};

struct ControllerAxisEvent {
  int which{0};
  int axis{0};
  float value{0.0f}; // normalized [-1,1] (triggers [0,1])
};

struct TextInputEvent {
  std::string text;
};

enum class WindowLifecycleEvent {
  Shown,
  Hidden,
  Exposed,
  Moved,
  Resized,
  Minimized,
  Maximized,
  Restored,
  MouseEnter,
  MouseLeave,
  FocusGained,
  FocusLost,
  CloseRequested,
  DisplayChanged,
};

struct WindowEvent {
  WindowLifecycleEvent type{WindowLifecycleEvent::Shown};
  int data1{0};
  int data2{0};
};

struct WindowConfig {
  std::string title{"Freelancer (Linux)"};
  int width{1280};
  int height{720};
  bool resizable{true};
  bool visible{true};
  bool high_dpi{false};
  bool prefer_wayland{true}; // Wayland-first; falls back to default driver
  bool vulkan{false};         // set SDL_WINDOW_VULKAN when needed (e.g. gfx integration)
};

struct InputState {
  // Efficient 256-byte VK snapshot (Win32 GetKeyboardState-style) and
  // 512-bit scancode bitset to avoid per-event hash allocations.
  std::array<bool, 256> vk_down{};
  std::bitset<512> scancodes_down;
  std::array<bool, 8> mouse_buttons_down{};
  int mouse_x{0};
  int mouse_y{0};
  float mouse_dx_accum{0.0f};
  float mouse_dy_accum{0.0f};
  bool focused{false};
  bool mouse_in_window{false};
  bool close_requested{false};
};

struct WindowCallbacks {
  std::function<void(const KeyEvent&)> on_key;
  std::function<void(const TextInputEvent&)> on_text;
  std::function<void(const MouseButtonEvent&)> on_mouse_button;
  std::function<void(const MouseMotionEvent&)> on_mouse_motion;
  std::function<void(const MouseWheelEvent&)> on_mouse_wheel;
  std::function<void(const ControllerButtonEvent&)> on_controller_button;
  std::function<void(const ControllerAxisEvent&)> on_controller_axis;
  std::function<void(const WindowEvent&)> on_window;
};

// Maps SDL keycode/scancode to a Win32-like virtual-key code for engine adapters.
std::uint32_t sdl_key_to_win_vk(int sdl_keycode, int sdl_scancode);

class Window {
 public:
  Window() = default;
  ~Window();

  Window(const Window&) = delete;
  Window& operator=(const Window&) = delete;
  Window(Window&& other) noexcept;
  Window& operator=(Window&& other) noexcept;

  bool create(const WindowConfig& config);
  void destroy();
  bool is_open() const;

  // Pumps SDL events, updates internal state, and dispatches callbacks.
  // Returns false when a close was requested (or window no longer open) and
  // the caller should stop the frame/update loop.
  bool pump_events(const WindowCallbacks& callbacks = {});
  bool pump_events() { return pump_events(WindowCallbacks{}); }

  // Convenience frame-loop helper: pump -> if running call tick(frame_index, dt_ms) -> repeat.
  // Stops on close request or if tick returns false. max_frames<=0 means unlimited.
  // yield_cpu: when false (default for rendering), no SDL_Delay is performed to avoid
  // frame-pacing micro-stutter. When true, yields ~1ms for harness/headless loops.
  void run_loop(const std::function<bool(std::uint64_t frame_index, double dt_ms)>& tick,
                const WindowCallbacks& callbacks = {}, int max_frames = 0, double max_seconds = 0.0,
                bool yield_cpu = false);

  // Lifecycle / focus / size
  void show();
  void hide();
  void raise();
  void set_title(const std::string& title);
  void get_size(int& w, int& h) const;
  void set_size(int w, int h);
  bool has_focus() const;
  WindowLifecycleEvent last_lifecycle_event() const;

  // Display/input modes (fullscreen/windowed).
  bool set_fullscreen(bool enabled);
  bool is_fullscreen() const { return fullscreen_; }
  void set_windowed() { set_fullscreen(false); }
  // Toggle between fullscreen and windowed.
  bool toggle_fullscreen() { return set_fullscreen(!fullscreen_); }

  // Cursor / grab handling
  void set_cursor_visible(bool visible);
  void set_relative_mouse_mode(bool enabled);
  void set_mouse_grab(bool grabbed);
  bool cursor_visible() const;
  bool relative_mouse_mode() const;
  bool mouse_grabbed() const;

  // State/adapters
  const InputState& input_state() const { return state_; }
  SDL_Window* sdl_window() const { return window_; }
  std::uint32_t sdl_window_id() const { return window_id_; }

  // Opaque adapter token for engine-facing HWND-style plumbing (not a Win32 HWND).
  // Stable for the lifetime of the Window; avoids fake-integer HWND assumptions.
  void* adapter_handle() const { return reinterpret_cast<void*>(window_); }

 private:
  void handle_event(const SDL_Event& ev, const WindowCallbacks& callbacks);
  void reset_transient_state();

  SDL_Window* window_{nullptr};
  std::uint32_t window_id_{0};
  InputState state_{};
  WindowLifecycleEvent last_lifecycle_{WindowLifecycleEvent::Shown};
  bool cursor_visible_{true};
  bool relative_mouse_{false};
  bool mouse_grabbed_{false};
  bool fullscreen_{false};
  std::vector<std::uint32_t> opened_gamepads_; // SDL_JoystickID list (tracked for cleanup)
  bool sdl_video_initialized_{false};
};

} // namespace fl_port
