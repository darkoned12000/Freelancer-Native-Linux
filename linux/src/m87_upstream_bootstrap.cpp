// M8.7 guarded upstream client/library bootstrap integration scaffold.
// Provides a hook point for wiring src/freelancer/Client/main.cpp and engine_init.cpp
// (and transitive upstream init paths) into freelancer_linux_client as they become
// portable. In the current bring-up build this returns "not available" so the client
// can fall back to validated M8.5-controlled flow with diagnostics.

#include "linux_port/platform.h"

#if defined(__GNUC__) || defined(__clang__)
#define FL_M87_EXPORT __attribute__((visibility("default")))
#else
#define FL_M87_EXPORT
#endif

extern "C" FL_M87_EXPORT int fl_m87_try_invoke_upstream_bootstrap(void* /*native_window_or_null*/, const char* /*ini_path_or_null*/) {
  // TODO(M8.7): when upstream client bootstrap/init TUs are compiled and linked for Linux,
  // invoke wrapped WinMain/InitializeEngine (or equivalent upstream entry points) here.
  fl_port::log_message(fl_port::LogLevel::Info,
                       "M8.7 bootstrap hook: upstream init TUs unavailable (scaffold only)");
  return 0;
}
