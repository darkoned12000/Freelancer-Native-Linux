#pragma once

// Extended Win32 synchronization shim skeleton for Linux bring-up (#1).

#if defined(_WIN32)
#include <windows.h>
#else

#include "linux_port/win32_compat.h"
#include "linux_port/win32_threads.h"

#include <condition_variable>
#include <mutex>
#include <unordered_set>

// Minimal opaque event placeholder (not Win32-semantically complete).
struct FL_PORT_EVENT {
  std::mutex m;
  std::condition_variable cv;
  bool signaled = false;
  bool manual_reset = false;
};

namespace fl_port_detail {
inline std::mutex& event_registry_mutex() {
  static std::mutex m;
  return m;
}
inline std::unordered_set<void*>& event_registry() {
  static std::unordered_set<void*> s;
  return s;
}
inline bool register_event(void* e) {
  std::lock_guard<std::mutex> lock(event_registry_mutex());
  return event_registry().insert(e).second;
}
inline bool unregister_event(void* e) {
  std::lock_guard<std::mutex> lock(event_registry_mutex());
  auto it = event_registry().find(e);
  if (it == event_registry().end()) return false;
  event_registry().erase(it);
  return true;
}
inline bool is_event(void* e) {
  std::lock_guard<std::mutex> lock(event_registry_mutex());
  return event_registry().find(e) != event_registry().end();
}
} // namespace fl_port_detail

inline HANDLE CreateEventA(void* /*lpEventAttributes*/, BOOL bManualReset, BOOL bInitialState, const char* /*lpName*/) {
  auto* e = new FL_PORT_EVENT();
  e->manual_reset = (bManualReset != FALSE);
  e->signaled = (bInitialState != FALSE);
  (void)fl_port_detail::register_event(static_cast<void*>(e));
  return static_cast<HANDLE>(e);
}

inline BOOL SetEvent(HANDLE hEvent) {
  if (!fl_port_detail::is_event(hEvent)) return FALSE;
  auto* e = static_cast<FL_PORT_EVENT*>(hEvent);
  {
    std::lock_guard<std::mutex> lock(e->m);
    e->signaled = true;
  }
  e->cv.notify_all();
  return TRUE;
}

inline BOOL ResetEvent(HANDLE hEvent) {
  if (!fl_port_detail::is_event(hEvent)) return FALSE;
  auto* e = static_cast<FL_PORT_EVENT*>(hEvent);
  std::lock_guard<std::mutex> lock(e->m);
  e->signaled = false;
  return TRUE;
}

inline BOOL CloseEventHandle(HANDLE hObject) {
  if (!hObject) return FALSE;
  if (!fl_port_detail::unregister_event(hObject)) return FALSE;
  delete static_cast<FL_PORT_EVENT*>(hObject);
  return TRUE;
}

// Helper for unified CloseHandle implementations (e.g. win32 file shim) to
// close event handles without leaking when upstream code passes events to
// CloseHandle().
inline BOOL fl_port_close_handle_if_event(HANDLE hObject) {
  if (!fl_port_detail::is_event(hObject)) return FALSE;
  return CloseEventHandle(hObject);
}

inline BOOL fl_port_is_event_handle(HANDLE hObject) {
  return fl_port_detail::is_event(hObject) ? TRUE : FALSE;
}

#ifndef INFINITE
#  define INFINITE 0xFFFFFFFFu
#endif
#ifndef WAIT_OBJECT_0
#  define WAIT_OBJECT_0 0u
#endif
#ifndef WAIT_TIMEOUT
#  define WAIT_TIMEOUT 258u
#endif
#ifndef WAIT_FAILED
#  define WAIT_FAILED 0xFFFFFFFFu
#endif
#ifndef MAXIMUM_WAIT_OBJECTS
#  define MAXIMUM_WAIT_OBJECTS 64
#endif

inline DWORD WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds) {
  if (auto thread = fl_port_detail::find_thread(hHandle)) {
    std::unique_lock<std::mutex> lock(thread->mutex);
    if (dwMilliseconds == INFINITE) thread->cv.wait(lock, [&]() { return thread->done; });
    else if (!thread->cv.wait_for(lock, std::chrono::milliseconds(dwMilliseconds), [&]() { return thread->done; })) return WAIT_TIMEOUT;
    return WAIT_OBJECT_0;
  }
  if (!fl_port_detail::is_event(hHandle)) return WAIT_FAILED;
  auto* e = static_cast<FL_PORT_EVENT*>(hHandle);

  std::unique_lock<std::mutex> lock(e->m);
  if (dwMilliseconds == INFINITE) {
    e->cv.wait(lock, [e]() { return e->signaled; });
  } else {
    const bool acquired = e->cv.wait_for(lock, std::chrono::milliseconds(dwMilliseconds), [e]() {
      return e->signaled;
    });
    if (!acquired) return WAIT_TIMEOUT;
  }

  if (!e->manual_reset) {
    e->signaled = false; // auto-reset semantics
  }
  return WAIT_OBJECT_0;
}

inline DWORD WaitForMultipleObjects(DWORD count, const HANDLE* handles, BOOL waitAll, DWORD timeout) {
  if (count == 0 || !handles || count > MAXIMUM_WAIT_OBJECTS) return WAIT_FAILED;
  const auto start = std::chrono::steady_clock::now();
  for (;;) {
    if (waitAll) {
      bool all_ready = true;
      for (DWORD i = 0; i < count; ++i) {
        if (auto thread = fl_port_detail::find_thread(handles[i])) {
          std::lock_guard<std::mutex> lock(thread->mutex);
          if (!thread->done) all_ready = false;
        } else if (fl_port_detail::is_event(handles[i])) {
          auto* event = static_cast<FL_PORT_EVENT*>(handles[i]);
          std::lock_guard<std::mutex> lock(event->m);
          if (!event->signaled) all_ready = false;
        } else return WAIT_FAILED;
      }
      if (all_ready) {
        for (DWORD i = 0; i < count; ++i) {
          if (fl_port_detail::is_event(handles[i])) {
            auto* event = static_cast<FL_PORT_EVENT*>(handles[i]);
            std::lock_guard<std::mutex> lock(event->m);
            if (!event->manual_reset) event->signaled = false;
          }
        }
        return WAIT_OBJECT_0;
      }
    } else {
      for (DWORD i = 0; i < count; ++i) {
        if (auto thread = fl_port_detail::find_thread(handles[i])) {
          std::lock_guard<std::mutex> lock(thread->mutex);
          if (thread->done) return WAIT_OBJECT_0 + i;
        } else if (fl_port_detail::is_event(handles[i])) {
          auto* event = static_cast<FL_PORT_EVENT*>(handles[i]);
          std::lock_guard<std::mutex> lock(event->m);
          if (event->signaled) {
            if (!event->manual_reset) event->signaled = false;
            return WAIT_OBJECT_0 + i;
          }
        } else return WAIT_FAILED;
      }
    }
    if (timeout != INFINITE && std::chrono::steady_clock::now() - start >= std::chrono::milliseconds(timeout)) return WAIT_TIMEOUT;
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

#endif // !_WIN32
