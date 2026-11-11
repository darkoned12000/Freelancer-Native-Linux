# M4 — Windowing and input (status)

**Status:** completed in working tree (pending code review, not committed).  
**Acceptance target:** Native Linux window created with functioning input/event loop.

## Summary

M4 adds an SDL3-based, Wayland-first windowing and input abstraction for Linux bring-up, with engine-adapter-friendly handles (avoiding fake-`HWND` assumptions), keyboard/mouse/controller event mapping, focus/lifecycle/resize/cursor/grab handling, and event-loop integration hooks for engine frame/update flow. A harness proves native window creation and a functioning input/event loop.

## Implemented

### Window/event abstraction (SDL3, Wayland-first)
- `linux/include/linux_port/window_input.h`, `linux/src/window_input.cpp` (`fl_port::Window`, `fl_port::WindowConfig`, `fl_port::WindowCallbacks`, `fl_port::InputState`).
  - **Wayland-first:** prefers `wayland` video driver with fallback to default driver selection if unavailable (with video subsystem restart so driver hint actually takes effect).
  - **Lifecycle:** create/destroy/show/hide/raise/title/size APIs; window events mapped to `WindowLifecycleEvent` (shown/hidden/exposed/moved/resized/minimized/maximized/restored/mouse enter/leave/focus gained/lost/close requested/display changed).
  - **Engine adapter handle:** `adapter_handle()` returns a stable opaque token backed by the SDL window (not a Win32 `HWND` integer/behavior stub), suitable for engine-facing plumbing that must associate platform window state without assuming Win32 semantics.
  - SDL3 video/events/gamepad subsystem initialization scoped to window lifetime.
  - **Review-fix: gamepad handle leak fixed** — gamepads are now closed via `SDL_CloseGamepad` on disconnect and window destruction.
  - **Review-fix: sub-pixel mouse deltas** — `InputState` accumulates `mouse_dx_accum`/`mouse_dy_accum` as `float` to preserve high-DPI/Wayland sub-pixel deltas.
  - **Review-fix: generic modifier mirroring** — `VK_SHIFT`/`VK_CONTROL`/`VK_MENU` (0x10/0x11/0x12) are mirrored from specific L/R modifier states.
  - **Review-fix: efficient input state** — replaced `std::unordered_map` with `std::array<bool,256>` for VKs and `std::bitset<512>` for scancodes (no per-event hash allocations).
  - **Review-fix: frame pacing** — `run_loop` `SDL_Delay(1)` is now gated behind `yield_cpu` parameter (default false) to avoid micro-stutter during active rendering.

### Keyboard/mouse/controller mapping
- **Keyboard:** SDL key events translated to `KeyEvent` with SDL scancode/keycode, repeat/modifier state, and Win32-like virtual-key mapping via `sdl_key_to_win_vk` (letters/digits, function keys, navigation/editing/modifiers, keypad/common OEM keys) for engine/input-semantic adapters.
- **Text input:** UTF-8 text input events surfaced via callback.
- **Mouse:** button press/release (left/middle/right/X1/X2), motion (absolute position + relative delta accumulation), wheel, and per-frame/input-state tracking (position, button state).
- **Controller/gamepad (SDL gamepad API):** device open tracking (including hotplug add/remove handling) and mapped button/axis events; axis values normalized to `[-1,1]` (triggers normalized to `[0,1]`) for expected input semantics.

### Focus, lifecycle, resize, cursor/grab handling
- Focus/mouse-enter state tracked in `InputState` and dispatched via lifecycle callbacks.
- Resize/move/pixel-size changes surfaced as window events and observable via size APIs.
- Cursor visibility, mouse grab, and relative mouse mode APIs (`set_cursor_visible`, `set_mouse_grab`, `set_relative_mouse_mode`) with state accessors for engine/camera/input capture needs.

### Event loop integration with engine/frame/update flow
- `pump_events(...)`: polls/translates SDL events, updates `InputState`, dispatches callbacks, and returns continuation status (stops on close request/window loss).
- `run_loop(tick, callbacks, max_frames, max_seconds)`: frame/update-loop helper that pumps events and invokes per-frame tick callback with frame index and delta time, suitable for harnessing and as an integration pattern for engine frame/update flow.

## Harness

- **Target:** `m4_window_input_harness` (built when usable target SDL3 libraries are available)
- **Source:** `linux/tests/m4_window_input_harness.cpp`
- **CMake:** `fl_linux_port_window` static library + `m4_window_input_harness` added under `FL_LINUX_BUILD_TOOLS` when `FL_SDL3_USABLE`.
- **Behavior:** creates a native Wayland-first window, exercises title/resize/cursor/grab/relative-mouse APIs, runs a functioning event loop (auto-exits after ~2s for validation), reports lifecycle/input event counts and state, and validates keyboard mapping helper semantics.

## Validation

- **64-bit preset (`linux-x64`, Wayland session):** `m4_window_input_harness` → **PASS** (native window created, lifecycle/resize events observed, event loop pumped ~2s / ~1878 frames, clean shutdown).
- **32-bit preset (`linux-x86`):** target builds when 32-bit SDL3 is usable; runtime validation depends on host 32-bit SDL3/Wayland/X11 backend availability (same environment constraints as other SDL3 targets).

## Follow-on (M5+)

- Wire `fl_port::Window`/input callbacks into engine window/input consumers and Win32 message-pump expectations (`PeekMessage`/`DispatchMessage`/WndProc adapters) as engine subsystems are integrated.
- Extend mapping tables/semantics (DirectInput-era codes/buttons, raw/relative input nuances, controller rumble/haptics, IME/text composition) based on traced engine usage.
- Integrate Vulkan/swapchain presentation path with engine render pipeline (`IRenderPipeline`/RP8) per M5 scope.