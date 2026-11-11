#pragma once

// Win32 threading/synchronization shim skeleton for Linux bring-up (#1).
// Win32 CRITICAL_SECTION is recursive; use PTHREAD_MUTEX_RECURSIVE.

#if defined(_WIN32)
#include <windows.h>
#else

#include "linux_port/win32_compat.h"

#include <pthread.h>

using CRITICAL_SECTION = pthread_mutex_t;

inline void InitializeCriticalSection(CRITICAL_SECTION* cs) {
  if (!cs) return;
  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init(cs, &attr);
  pthread_mutexattr_destroy(&attr);
}

inline BOOL InitializeCriticalSectionAndSpinCount(CRITICAL_SECTION* cs, DWORD /*dwSpinCount*/) {
  InitializeCriticalSection(cs);
  return TRUE;
}

inline BOOL TryEnterCriticalSection(CRITICAL_SECTION* cs) {
  return (cs && pthread_mutex_trylock(cs) == 0) ? TRUE : FALSE;
}

inline void DeleteCriticalSection(CRITICAL_SECTION* cs) {
  if (cs) pthread_mutex_destroy(cs);
}

inline void EnterCriticalSection(CRITICAL_SECTION* cs) {
  if (cs) pthread_mutex_lock(cs);
}

inline void LeaveCriticalSection(CRITICAL_SECTION* cs) {
  if (cs) pthread_mutex_unlock(cs);
}

// TODO: thread creation APIs and Win32-specific semantics (timeouts, alertable waits, APCs).

#endif // !_WIN32
