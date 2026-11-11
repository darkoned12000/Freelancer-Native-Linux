#pragma once

// Win32 timing shim skeleton for Linux bring-up (#1).

#if defined(_WIN32)
#include <windows.h>
#else

#include "linux_port/win32_compat.h"

#include <chrono>
#include <cstdint>
#include <thread>

inline DWORD GetTickCount() {
  using namespace std::chrono;
  static const auto start = steady_clock::now();
  const auto now = steady_clock::now();
  return static_cast<DWORD>(duration_cast<milliseconds>(now - start).count());
}

inline void Sleep(DWORD dwMilliseconds) {
  std::this_thread::sleep_for(std::chrono::milliseconds(dwMilliseconds));
}

union LARGE_INTEGER {
  struct {
    DWORD LowPart;
    LONG  HighPart;
  } u;
  std::int64_t QuadPart;
};

inline BOOL QueryPerformanceFrequency(LARGE_INTEGER* lpFrequency) {
  if (!lpFrequency) return FALSE;
  lpFrequency->QuadPart = 1000000000LL; // nanosecond ticks
  return TRUE;
}

inline BOOL QueryPerformanceCounter(LARGE_INTEGER* lpPerformanceCount) {
  if (!lpPerformanceCount) return FALSE;
  static const auto start = std::chrono::steady_clock::now();
  const auto now = std::chrono::steady_clock::now();
  lpPerformanceCount->QuadPart =
      std::chrono::duration_cast<std::chrono::nanoseconds>(now - start).count();
  return TRUE;
}

#endif // !_WIN32
