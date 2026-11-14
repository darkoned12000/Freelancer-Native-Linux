#pragma once

// Win32 user/input/window helper declarations for Linux bring-up.
// Declared in extern "C" for ABI compatibility with upstream code including
// <windows.h> on non-Windows targets.

#include "linux_port/win32_compat.h"

#ifndef HACCEL
using HACCEL = void*;
#endif

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

// Existing stubs
BOOL WINAPI ShowCursor(BOOL bShow);
HWND WINAPI SetFocus(HWND hWnd);
BOOL WINAPI SetForegroundWindow(HWND hWnd);
BOOL WINAPI UpdateWindow(HWND hWnd);

// Window class registration (stub: accept and return a fake atom)
ATOM WINAPI RegisterClassExA(const WNDCLASSEXA* lpwcx);
ATOM WINAPI RegisterClassA(const WNDCLASSA* lpwc);

// Window creation (stub: return a fake HWND)
HWND WINAPI CreateWindowExA(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName,
                           DWORD dwStyle, int X, int Y, int nWidth, int nHeight,
                           HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
#define CreateWindowA(lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam) \
  CreateWindowExA(0, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam)

// Window procedure (default: return 0)
LRESULT WINAPI DefWindowProcA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

// Window management
BOOL WINAPI DestroyWindow(HWND hWnd);
BOOL WINAPI ShowWindow(HWND hWnd, int nCmdShow);
BOOL WINAPI IsWindow(HWND hWnd);
BOOL WINAPI IsWindowVisible(HWND hWnd);

// Cursor
HCURSOR WINAPI LoadCursorA(HINSTANCE hInstance, LPCSTR lpCursorName);
HCURSOR WINAPI SetCursor(HCURSOR hCursor);

// Icon
HICON WINAPI LoadIconA(HINSTANCE hInstance, LPCSTR lpIconName);

// Menu
HMENU WINAPI LoadMenuA(HINSTANCE hInstance, LPCSTR lpMenuName);
HMENU WINAPI GetMenu(HWND hWnd);

// Accelerator
HACCEL WINAPI LoadAcceleratorsA(HINSTANCE hInstance, LPCSTR lpTableName);

// String resources
int WINAPI LoadStringA(HINSTANCE hInstance, UINT uID, LPSTR lpBuffer, int cchBufferMax);

// Misc
int WINAPI GetSystemMetrics(int nIndex);

#if defined(__cplusplus)
} // extern "C"
#endif
