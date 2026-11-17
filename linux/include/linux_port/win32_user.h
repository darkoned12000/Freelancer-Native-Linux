#pragma once

// Win32 user/input/window helper declarations for Linux bring-up.
// Declared in extern "C" for ABI compatibility with upstream code including
// <windows.h> on non-Windows targets.

#include "linux_port/win32_compat.h"

#ifndef HACCEL
using HACCEL = void*;
#endif
#ifndef HLOCAL
using HLOCAL = void*;
#endif

#ifndef _CURSORINFO_DEFINED
#define _CURSORINFO_DEFINED
typedef struct tagCURSORINFO {
  DWORD cbSize;
  DWORD flags;
  HCURSOR hCursor;
  POINT ptScreenPos;
} CURSORINFO, *PCURSORINFO, *LPCURSORINFO;
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
HCURSOR WINAPI GetCursor(void);
BOOL WINAPI IsBadWritePtr(LPVOID lp, UINT_PTR ucb);
UINT WINAPI MapVirtualKeyA(UINT uCode, UINT uMapType);
int WINAPI GetKeyNameTextW(LONG lParam, LPWSTR lpString, int cchSize);
LONG WINAPI RegDeleteKeyA(HKEY hKey, LPCSTR lpSubKey);
int WINAPI GetObjectA(HANDLE h, int c, LPVOID pv);
HANDLE WINAPI LoadImageA(HINSTANCE hInst, LPCSTR name, UINT type, int cx, int cy, UINT fuLoad);
DWORD WINAPI SizeofResource(HINSTANCE hModule, HRSRC hResInfo);
HGLOBAL WINAPI LoadResource(HINSTANCE hModule, HRSRC hResInfo);
LPVOID WINAPI LockResource(HGLOBAL hResData);
HRSRC WINAPI FindResourceExA(HINSTANCE hModule, LPCSTR lpType, LPCSTR lpName, WORD wLanguage);
SHORT WINAPI GetKeyState(int nVirtKey);
int WINAPI SelectClipRgn(HDC hdc, HRGN hrgn);
HBRUSH WINAPI CreateSolidBrush(COLORREF crColor);
int WINAPI FillRect(HDC hDC, const RECT* lprc, HBRUSH hbr);
UINT WINAPI SetTextAlign(HDC hdc, UINT fMode);
COLORREF WINAPI SetBkColor(HDC hdc, COLORREF crColor);
int WINAPI SetBkMode(HDC hdc, int mode);
COLORREF WINAPI SetTextColor(HDC hdc, COLORREF crColor);
BOOL WINAPI ExtTextOutW(HDC hdc, int X, int Y, UINT fuOptions, const RECT* lprc, LPCWSTR lpString, UINT cch, const int* lpDx);
BOOL WINAPI GetTextMetricsA(HDC hdc, LPTEXTMETRIC lptm);
BOOL WINAPI GetCharWidthW(HDC hdc, UINT iFirstChar, UINT iLastChar, LPINT lpBuffer);
BOOL WINAPI GetTextExtentPointW(HDC hdc, LPCWSTR lpString, int cbString, LPSIZE lpSize);
HDC WINAPI ReleaseDC(HWND hWnd, HDC hDC);
HFONT WINAPI CreateFontIndirectA(const void* lplf);
HDC WINAPI GetDC(HWND hWnd);
HDC WINAPI CreateCompatibleDC(HDC hdc);
HBITMAP WINAPI CreateCompatibleBitmap(HDC hdc, int cx, int cy);
HGDIOBJ WINAPI SelectObject(HDC hdc, HGDIOBJ h);
BOOL WINAPI DeleteDC(HDC hdc);
BOOL WINAPI DeleteObject(HGDIOBJ ho);
int WINAPI GetDIBits(HDC hdc, HBITMAP hbmp, UINT uStartScan, UINT cScanLines, LPVOID lpvBits, LPBITMAPINFO lpbi, UINT uUsage);
DWORD WINAPI FormatMessageA(DWORD dwFlags, LPCVOID lpSource, DWORD dwMessageId, DWORD dwLanguageId, LPSTR lpBuffer, DWORD nSize, va_list* Arguments);
HLOCAL WINAPI LocalFree(HLOCAL hMem);
HANDLE WINAPI GetProcessHeap(void);
LPVOID WINAPI HeapAlloc(HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes);
BOOL WINAPI HeapFree(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem);
HWND WINAPI GetFocus(void);
HWND WINAPI GetForegroundWindow(void);
BOOL WINAPI ClientToScreen(HWND hWnd, LPPOINT lpPoint);
BOOL WINAPI ScreenToClient(HWND hWnd, LPPOINT lpPoint);
BOOL WINAPI GetCursorPos(LPPOINT lpPoint);
BOOL WINAPI GetCursorInfo(PCURSORINFO pci);
BOOL WINAPI SetCursorPos(int X, int Y);
UINT WINAPI GetDoubleClickTime(void);
HWND WINAPI GetDesktopWindow(void);
BOOL WINAPI GetWindowRect(HWND hWnd, LPRECT lpRect);
LONG WINAPI GetWindowLongA(HWND hWnd, int nIndex);
LONG WINAPI SetWindowLongA(HWND hWnd, int nIndex, LONG dwNewLong);
BOOL WINAPI AdjustWindowRect(LPRECT lpRect, DWORD dwStyle, BOOL bMenu);
BOOL WINAPI AdjustWindowRectEx(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle);
BOOL WINAPI InvalidateRect(HWND hWnd, const RECT* lpRect, BOOL bErase);
BOOL WINAPI SetWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags);

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
