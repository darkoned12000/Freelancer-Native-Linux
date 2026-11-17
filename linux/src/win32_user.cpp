#include <cstdlib>
// Win32 user/window/message pump stubs for Linux bring-up.
#include "linux_port/win32_user.h"

#include <cstdio>
#include <cstring>
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

BOOL WINAPI AdjustWindowRect(LPRECT lpRect, DWORD dwStyle, BOOL bMenu) { (void)dwStyle; (void)bMenu; return lpRect ? TRUE : FALSE; }
BOOL WINAPI AdjustWindowRectEx(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle) { (void)dwStyle; (void)bMenu; (void)dwExStyle; return lpRect ? TRUE : FALSE; }
BOOL WINAPI InvalidateRect(HWND hWnd, const RECT* lpRect, BOOL bErase) { (void)hWnd; (void)lpRect; (void)bErase; return TRUE; }
BOOL WINAPI SetWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags) { (void)hWnd; (void)hWndInsertAfter; (void)X; (void)Y; (void)cx; (void)cy; (void)uFlags; return TRUE; }
LONG WINAPI GetWindowLongA(HWND hWnd, int nIndex) { (void)hWnd; (void)nIndex; return 0; }
LONG WINAPI SetWindowLongA(HWND hWnd, int nIndex, LONG dwNewLong) { (void)hWnd; (void)nIndex; (void)dwNewLong; return 0; }
HWND WINAPI GetDesktopWindow(void) { return nullptr; }
BOOL WINAPI GetWindowRect(HWND hWnd, LPRECT lpRect) { (void)hWnd; if (lpRect) { lpRect->left=lpRect->top=lpRect->right=lpRect->bottom=0; } return TRUE; }
BOOL WINAPI ClientToScreen(HWND hWnd, LPPOINT lpPoint) { (void)hWnd; if (lpPoint) { lpPoint->x=lpPoint->y=0; } return TRUE; }
BOOL WINAPI ScreenToClient(HWND hWnd, LPPOINT lpPoint) { (void)hWnd; if (lpPoint) { lpPoint->x=lpPoint->y=0; } return TRUE; }
BOOL WINAPI GetCursorPos(LPPOINT lpPoint) { if (lpPoint) { lpPoint->x=lpPoint->y=0; } return TRUE; }
BOOL WINAPI SetCursorPos(int X, int Y) { (void)X; (void)Y; return TRUE; }
UINT WINAPI GetDoubleClickTime(void) { return 500; }
HWND WINAPI GetFocus(void) { return nullptr; }
HWND WINAPI GetForegroundWindow(void) { return nullptr; }
BOOL WINAPI GetCursorInfo(PCURSORINFO pci) { if (pci) { pci->cbSize=sizeof(CURSORINFO); pci->flags=0; pci->hCursor=nullptr; pci->ptScreenPos.x=pci->ptScreenPos.y=0; } return TRUE; }
HANDLE WINAPI GetProcessHeap(void) { return (HANDLE)1; }
LPVOID WINAPI HeapAlloc(HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes) { (void)hHeap;(void)dwFlags; return malloc(dwBytes ? dwBytes : 1); }
BOOL WINAPI HeapFree(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem) { (void)hHeap;(void)dwFlags; free(lpMem); return TRUE; }
HLOCAL WINAPI LocalFree(HLOCAL hMem) { free(hMem); return nullptr; }
DWORD WINAPI FormatMessageA(DWORD dwFlags, LPCVOID lpSource, DWORD dwMessageId, DWORD dwLanguageId, LPSTR lpBuffer, DWORD nSize, va_list* Arguments) { (void)dwFlags;(void)lpSource;(void)dwMessageId;(void)dwLanguageId;(void)Arguments; if (lpBuffer && nSize) lpBuffer[0]='\0'; return 0; }
HDC WINAPI GetDC(HWND hWnd) { (void)hWnd; return nullptr; }
HDC WINAPI CreateCompatibleDC(HDC hdc) { (void)hdc; return nullptr; }
HBITMAP WINAPI CreateCompatibleBitmap(HDC hdc, int cx, int cy) { (void)hdc;(void)cx;(void)cy; return nullptr; }
HGDIOBJ WINAPI SelectObject(HDC hdc, HGDIOBJ h) { (void)hdc;(void)h; return nullptr; }
BOOL WINAPI DeleteDC(HDC hdc) { (void)hdc; return TRUE; }
BOOL WINAPI DeleteObject(HGDIOBJ ho) { (void)ho; return TRUE; }
int WINAPI GetDIBits(HDC hdc, HBITMAP hbmp, UINT uStartScan, UINT cScanLines, LPVOID lpvBits, LPBITMAPINFO lpbi, UINT uUsage) { (void)hdc;(void)hbmp;(void)uStartScan;(void)cScanLines;(void)lpvBits;(void)lpbi;(void)uUsage; return 0; }
HDC WINAPI ReleaseDC(HWND hWnd, HDC hDC) { (void)hWnd; (void)hDC; return nullptr; }
HFONT WINAPI CreateFontIndirectA(const void* lplf) { (void)lplf; return nullptr; }
UINT WINAPI SetTextAlign(HDC hdc, UINT fMode) { (void)hdc;(void)fMode; return 0; }
COLORREF WINAPI SetBkColor(HDC hdc, COLORREF crColor) { (void)hdc;(void)crColor; return 0; }
int WINAPI SetBkMode(HDC hdc, int mode) { (void)hdc;(void)mode; return 0; }
COLORREF WINAPI SetTextColor(HDC hdc, COLORREF crColor) { (void)hdc;(void)crColor; return 0; }
BOOL WINAPI ExtTextOutW(HDC hdc, int X, int Y, UINT fuOptions, const RECT* lprc, LPCWSTR lpString, UINT cch, const int* lpDx) { (void)hdc;(void)X;(void)Y;(void)fuOptions;(void)lprc;(void)lpString;(void)cch;(void)lpDx; return TRUE; }
BOOL WINAPI GetTextMetricsA(HDC hdc, LPTEXTMETRIC lptm) { (void)hdc; if (lptm) memset(lptm, 0, sizeof(TEXTMETRIC)); return TRUE; }
BOOL WINAPI GetCharWidthW(HDC hdc, UINT iFirstChar, UINT iLastChar, LPINT lpBuffer) { (void)hdc;(void)iFirstChar;(void)iLastChar;(void)lpBuffer; return TRUE; }
BOOL WINAPI GetTextExtentPointW(HDC hdc, LPCWSTR lpString, int cbString, LPSIZE lpSize) { (void)hdc;(void)lpString;(void)cbString; if (lpSize) { lpSize->cx=0; lpSize->cy=0; } return TRUE; }
int WINAPI SelectClipRgn(HDC hdc, HRGN hrgn) { (void)hdc;(void)hrgn; return 0; }
HBRUSH WINAPI CreateSolidBrush(COLORREF crColor) { (void)crColor; return nullptr; }
int WINAPI FillRect(HDC hDC, const RECT* lprc, HBRUSH hbr) { (void)hDC;(void)lprc;(void)hbr; return 0; }
SHORT WINAPI GetKeyState(int nVirtKey) { (void)nVirtKey; return 0; }
HRSRC WINAPI FindResourceExA(HINSTANCE hModule, LPCSTR lpType, LPCSTR lpName, WORD wLanguage) { (void)hModule;(void)lpType;(void)lpName;(void)wLanguage; return nullptr; }
DWORD WINAPI SizeofResource(HINSTANCE hModule, HRSRC hResInfo) { (void)hModule;(void)hResInfo; return 0; }
HGLOBAL WINAPI LoadResource(HINSTANCE hModule, HRSRC hResInfo) { (void)hModule;(void)hResInfo; return nullptr; }
LPVOID WINAPI LockResource(HGLOBAL hResData) { (void)hResData; return nullptr; }
int WINAPI GetObjectA(HANDLE h, int c, LPVOID pv) { (void)h;(void)c;(void)pv; return 0; }
HANDLE WINAPI LoadImageA(HINSTANCE hInst, LPCSTR name, UINT type, int cx, int cy, UINT fuLoad) { (void)hInst;(void)name;(void)type;(void)cx;(void)cy;(void)fuLoad; return nullptr; }
LONG WINAPI RegDeleteKeyA(HKEY hKey, LPCSTR lpSubKey) { (void)hKey;(void)lpSubKey; return 0; }
int WINAPI GetKeyNameTextW(LONG lParam, LPWSTR lpString, int cchSize) { (void)lParam; if (lpString && cchSize > 0) lpString[0]=L'\0'; return 0; }
UINT WINAPI MapVirtualKeyA(UINT uCode, UINT uMapType) { (void)uCode;(void)uMapType; return 0; }
BOOL WINAPI IsBadWritePtr(LPVOID lp, UINT_PTR ucb) { (void)lp;(void)ucb; return FALSE; }
HCURSOR WINAPI GetCursor(void) { return nullptr; }
