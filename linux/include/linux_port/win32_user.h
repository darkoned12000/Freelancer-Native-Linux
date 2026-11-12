#pragma once

// Minimal Win32 user/input/window helper declarations for Linux bring-up.
// Declared in extern "C" for ABI compatibility with upstream code including
// <windows.h> on non-Windows targets.

#include "linux_port/win32_compat.h"

#if defined(__cplusplus)
extern "C" {
#endif

#ifndef WINAPI
#  if defined(__i386__) || defined(__i386) || defined(i386)
#    if defined(__clang__) || defined(__GNUC__)
#      define WINAPI __attribute__((stdcall))
#    else
#      define WINAPI
#    endif
#  else
#    define WINAPI
#  endif
#endif

BOOL WINAPI ShowCursor(BOOL bShow);
HWND WINAPI SetFocus(HWND hWnd);
BOOL WINAPI SetForegroundWindow(HWND hWnd);
BOOL WINAPI UpdateWindow(HWND hWnd);

#if defined(__cplusplus)
} // extern "C"
#endif
