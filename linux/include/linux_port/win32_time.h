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
  // DWORD wraparound is intentional (Win32 semantics, ~49.7 days).
  return static_cast<DWORD>(duration_cast<milliseconds>(now - start).count());
}

inline unsigned long long GetTickCount64() {
  using namespace std::chrono;
  static const auto start = steady_clock::now();
  const auto now = steady_clock::now();
  return static_cast<unsigned long long>(duration_cast<milliseconds>(now - start).count());
}

inline DWORD timeGetTime() { return GetTickCount(); }

inline void Sleep(DWORD dwMilliseconds) {
  std::this_thread::sleep_for(std::chrono::milliseconds(dwMilliseconds));
}

inline DWORD SleepEx(DWORD dwMilliseconds, BOOL /*bAlertable*/) {
  Sleep(dwMilliseconds);
  return 0; // 0 = timeout/wait completed (alertable APC semantics not modeled)
}

union LARGE_INTEGER {
  struct {
    DWORD LowPart;
    LONG  HighPart;
  };
  struct {
    DWORD LowPart;
    LONG  HighPart;
  } u;
  std::int64_t QuadPart;
};

union ULARGE_INTEGER {
  struct {
    DWORD LowPart;
    DWORD HighPart;
  };
  struct {
    DWORD LowPart;
    DWORD HighPart;
  } u;
  std::uint64_t QuadPart;
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
