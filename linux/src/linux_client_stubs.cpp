// M8.1/M8.4 stub/no-op surface for not-yet-implemented client bring-up calls.
// Keep stubs minimal, deterministic, and logged so missing behavior is visible.
// Expand incrementally as upstream TUs are linked.

#include "linux_port/platform.h"

#if !defined(_WIN32)
#include "linux_port/windows.h"
#endif

#include <cstdint>
#include <string>

extern "C" {

// Movies / cutscenes (DirectShow/Bink/etc. placeholders)
int fl_stub_movie_play(const char* path) {
  fl_port::log_message(fl_port::LogLevel::Warning,
                       std::string("[stub] movie play requested (bypassed/no-op): ") + (path ? path : "(null)"));
  return 0;
}
int fl_stub_movie_stop(void) {
  fl_port::log_message(fl_port::LogLevel::Debug, "[stub] movie stop (no-op)");
  return 0;
}
int fl_stub_movie_is_playing(void) {
  return 0;
}

// DirectShow/WMV bring-up blockers: deterministic bypass stubs (no playback)
int fl_stub_directshow_playback(const char* path) {
  fl_port::log_message(fl_port::LogLevel::Warning,
                       std::string("[stub] DirectShow playback bypassed (no-op): ") + (path ? path : "(null)"));
  return 0;
}

// IME blockers: stub minimal enable/disable/focus hooks for boot/menu bring-up
int fl_stub_ime_enable(int enable) {
  fl_port::log_message(fl_port::LogLevel::Debug,
                       std::string("[stub] IME ") + (enable ? "enabled" : "disabled") + " (no-op)");
  return 1;
}
int fl_stub_ime_set_focus(int focused) {
  fl_port::log_message(fl_port::LogLevel::Debug,
                       std::string("[stub] IME focus ") + (focused ? "gained" : "lost") + " (no-op)");
  return 1;
}
void* fl_stub_ime_get_context(void* hwnd) {
  (void)hwnd;
  return nullptr;
}
int fl_stub_ime_release_context(void* hwnd, void* himc) {
  (void)hwnd; (void)himc;
  return 1;
}

// EULA / UI gating placeholders
int fl_stub_eula_accepted(void) {
  fl_port::log_message(fl_port::LogLevel::Warning, "[stub] EULA gate bypassed/accepted for bring-up (no-op)");
  return 1;
}

// Registry write placeholders (legacy marker; Win32 Reg* APIs are XDG-backed in win32_registry.h)
long fl_stub_registry_write_noop(void) {
  fl_port::log_message(fl_port::LogLevel::Debug, "[stub] legacy registry write marker (no-op)");
  return 0; // ERROR_SUCCESS-like
}

// Generic not-yet-implemented marker for diagnostics
void fl_stub_not_implemented(const char* what) {
  if (!what) what = "(unknown)";
  fl_port::log_message(fl_port::LogLevel::Warning, std::string("[stub] not implemented: ") + what);
}

} // extern "C"

#if !defined(_WIN32)
// Win32 API stubs commonly needed by early client/bootstrap paths.
// Declarations provided by linux_port/win32_user.h (included via windows.h).
// Use extern "C" to match Win32 C linkage expected by upstream code.

extern "C" {

BOOL WINAPI ShowCursor(BOOL bShow) {
  static int display_count = 0;
  display_count += bShow ? 1 : -1;
  return display_count;
}

BOOL WINAPI SetForegroundWindow(HWND /*hWnd*/) { return TRUE; }

HWND WINAPI SetFocus(HWND hWnd) { return hWnd; }

BOOL WINAPI UpdateWindow(HWND /*hWnd*/) { return TRUE; }

// Minimal Win32 calls often hit during boot/menu bring-up (no real window manager semantics)
HWND WINAPI GetFocus(void) { return nullptr; }
HWND WINAPI GetForegroundWindow(void) { return nullptr; }
BOOL WINAPI IsWindow(HWND /*hWnd*/) { return TRUE; }
BOOL WINAPI ShowWindow(HWND /*hWnd*/, int /*nCmdShow*/) { return TRUE; }
BOOL WINAPI EnableWindow(HWND /*hWnd*/, BOOL /*bEnable*/) { return TRUE; }
BOOL WINAPI DestroyWindow(HWND /*hWnd*/) { return TRUE; }

} // extern "C"

#endif
