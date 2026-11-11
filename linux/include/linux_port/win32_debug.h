#pragma once

// Debug output shim routed to platform logging.

#if defined(_WIN32)
#include <windows.h>
#else

#include "linux_port/platform.h"

inline void OutputDebugStringA(const char* msg) {
  if (msg) fl_port::debug_output(msg);
}

#endif // !_WIN32
