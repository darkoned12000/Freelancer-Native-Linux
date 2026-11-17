#pragma once

// Win32 threading/synchronization shim skeleton for Linux bring-up (#1).
// Win32 CRITICAL_SECTION is recursive; use PTHREAD_MUTEX_RECURSIVE.

#if defined(_WIN32)
#include <windows.h>
#else

#include "linux_port/win32_compat.h"

#include <pthread.h>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>

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

using LPTHREAD_START_ROUTINE = DWORD (WINAPI*)(LPVOID);
#ifndef CREATE_SUSPENDED
#define CREATE_SUSPENDED 0x00000004
#endif
struct FL_PORT_THREAD_STATE {
  std::mutex mutex;
  std::condition_variable cv;
  bool done{false};
  DWORD exit_code{0};
};
namespace fl_port_detail {
inline std::mutex& thread_registry_mutex() { static std::mutex m; return m; }
inline std::unordered_map<void*, std::shared_ptr<FL_PORT_THREAD_STATE>>& thread_registry() {
  static std::unordered_map<void*, std::shared_ptr<FL_PORT_THREAD_STATE>> r;
  return r;
}
inline std::shared_ptr<FL_PORT_THREAD_STATE> find_thread(HANDLE h) {
  std::lock_guard<std::mutex> lock(thread_registry_mutex());
  auto it = thread_registry().find(h);
  return it == thread_registry().end() ? nullptr : it->second;
}
inline bool close_thread(HANDLE h) {
  std::lock_guard<std::mutex> lock(thread_registry_mutex());
  return thread_registry().erase(h) != 0;
}
}
inline HANDLE CreateThread(void*, SIZE_T, LPTHREAD_START_ROUTINE start, LPVOID param,
                           DWORD, DWORD* thread_id) {
  if (!start) return nullptr;
  auto state = std::make_shared<FL_PORT_THREAD_STATE>();
  void* token = state.get();
  {
    std::lock_guard<std::mutex> lock(fl_port_detail::thread_registry_mutex());
    fl_port_detail::thread_registry().emplace(token, state);
  }
  try {
    std::thread([state, start, param]() {
      const DWORD result = start(param);
      {
        std::lock_guard<std::mutex> lock(state->mutex);
        state->exit_code = result;
        state->done = true;
      }
      state->cv.notify_all();
    }).detach();
  } catch (...) {
    fl_port_detail::close_thread(token);
    return nullptr;
  }
  if (thread_id) *thread_id = static_cast<DWORD>(reinterpret_cast<std::uintptr_t>(token));
  return token;
}
inline HANDLE CreateThread(void* attrs, SIZE_T stack_size, LPTHREAD_START_ROUTINE start,
                           LPVOID param, DWORD flags, unsigned long* thread_id) {
  DWORD id = 0;
  HANDLE handle = CreateThread(attrs, stack_size, start, param, flags, &id);
  if (thread_id) *thread_id = id;
  return handle;
}
inline BOOL fl_port_close_handle_if_thread(HANDLE h) {
  return fl_port_detail::close_thread(h) ? TRUE : FALSE;
}
inline DWORD ResumeThread(HANDLE hThread) {
  return fl_port_detail::find_thread(hThread) ? 1u : 0xFFFFFFFFu;
}

#endif // !_WIN32
