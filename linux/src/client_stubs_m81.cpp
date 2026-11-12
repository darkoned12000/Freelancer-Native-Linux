// M8.1 stub/no-op surface for not-yet-implemented client bring-up calls.
// Keep stubs minimal, deterministic, and logged so missing behavior is visible.
// Expand incrementally as upstream TUs are linked in M8.1+.

#include "linux_port/platform.h"

#if !defined(_WIN32)
#include "linux_port/windows.h"
#endif

#include <cstdint>

extern "C" {

// Movies / cutscenes (DirectShow/Bink/etc. placeholders)
int fl_stub_movie_play(const char* /*path*/) {
  fl_port::log_message(fl_port::LogLevel::Warning, "[stub] movie play requested (no-op)");
  return 0;
}
int fl_stub_movie_stop(void) {
  fl_port::log_message(fl_port::LogLevel::Debug, "[stub] movie stop (no-op)");
  return 0;
}

// EULA / UI gating placeholders
int fl_stub_eula_accepted(void) {
  fl_port::log_message(fl_port::LogLevel::Warning, "[stub] EULA gate bypassed/accepted for bring-up (no-op)");
  return 1;
}

// Registry write placeholders (persistence handled later via XDG-backed shims)
long fl_stub_registry_write_noop(void) {
  fl_port::log_message(fl_port::LogLevel::Debug, "[stub] registry write ignored (no-op)");
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

} // extern "C"

#endif
