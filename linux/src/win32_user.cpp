// Win32 user/window/message pump stubs for Linux bring-up.
#include "linux_port/win32_user.h"

#include <cstdio>
#include <cstdlib>

extern "C" {

static HWND next_hwnd = reinterpret_cast<HWND>(0x1000);

BOOL WINAPI ShowCursor(BOOL bShow) { (void)bShow; return 0; }
HWND WINAPI SetFocus(HWND hWnd) { return hWnd; }
BOOL WINAPI SetForegroundWindow(HWND hWnd) { (void)hWnd; return TRUE; }
BOOL WINAPI UpdateWindow(HWND hWnd) { (void)hWnd; return TRUE; }

ATOM WINAPI RegisterClassExA(const WNDCLASSEXA* lpwcx) { (void)lpwcx; return 1; }
ATOM WINAPI RegisterClassA(const WNDCLASSA* lpwc) { (void)lpwc; return 1; }

HWND WINAPI CreateWindowExA(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName,
                           DWORD dwStyle, int X, int Y, int nWidth, int nHeight,
                           HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam) {
  (void)dwExStyle; (void)lpClassName; (void)lpWindowName; (void)dwStyle;
  (void)X; (void)Y; (void)nWidth; (void)nHeight; (void)hWndParent;
  (void)hMenu; (void)hInstance; (void)lpParam;
  HWND h = next_hwnd;
  next_hwnd = reinterpret_cast<HWND>(reinterpret_cast<uintptr_t>(next_hwnd) + 1);
  return h;
}

LRESULT WINAPI DefWindowProcA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
  (void)hWnd; (void)Msg; (void)wParam; (void)lParam;
  return 0;
}





BOOL WINAPI DestroyWindow(HWND hWnd) { (void)hWnd; return TRUE; }
BOOL WINAPI ShowWindow(HWND hWnd, int nCmdShow) { (void)hWnd; (void)nCmdShow; return TRUE; }
BOOL WINAPI IsWindow(HWND hWnd) { (void)hWnd; return FALSE; }
BOOL WINAPI IsWindowVisible(HWND hWnd) { (void)hWnd; return FALSE; }

HCURSOR WINAPI LoadCursorA(HINSTANCE hInstance, LPCSTR lpCursorName) { (void)hInstance; (void)lpCursorName; return nullptr; }
HCURSOR WINAPI SetCursor(HCURSOR hCursor) { (void)hCursor; return nullptr; }

HICON WINAPI LoadIconA(HINSTANCE hInstance, LPCSTR lpIconName) { (void)hInstance; (void)lpIconName; return nullptr; }

HMENU WINAPI LoadMenuA(HINSTANCE hInstance, LPCSTR lpMenuName) { (void)hInstance; (void)lpMenuName; return nullptr; }
HMENU WINAPI GetMenu(HWND hWnd) { (void)hWnd; return nullptr; }

HACCEL WINAPI LoadAcceleratorsA(HINSTANCE hInstance, LPCSTR lpTableName) { (void)hInstance; (void)lpTableName; return nullptr; }

int WINAPI LoadStringA(HINSTANCE hInstance, UINT uID, LPSTR lpBuffer, int cchBufferMax) {
  (void)hInstance; (void)uID;
  if (lpBuffer && cchBufferMax > 0) lpBuffer[0] = '\0';
  return 0;
}

int WINAPI GetSystemMetrics(int nIndex) { (void)nIndex; return 0; }

} // extern "C"
