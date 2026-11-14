#pragma once

// Minimal Win32 compatibility layer for Linux bring-up (Phase 0, #1).
// This header is intended for ported/shimmed compilation units; do NOT include
// it from unmodified upstream Windows-only TUs without guards. Definitions are
// intentionally conservative and will evolve as DACOM/core modules are ported.

#if defined(_WIN32)
#include <windows.h>
#else

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <dlfcn.h>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#if !defined(_WIN32)
#include <sys/types.h>
#include <unistd.h>
#endif

#include "linux_port/path_util.h"

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

#ifndef CALLBACK
#  define CALLBACK WINAPI
#endif

#ifndef CDECL
#  if defined(__i386__) || defined(__i386) || defined(i386)
#    if defined(__clang__) || defined(__GNUC__)
#      define CDECL __attribute__((cdecl))
#    else
#      define CDECL
#    endif
#  else
#    define CDECL
#  endif
#endif

#ifndef __cdecl
#  define __cdecl CDECL
#endif
#ifndef __stdcall
#  define __stdcall WINAPI
#endif
#ifndef FASTCALL
#  if defined(__i386__) || defined(__i386) || defined(i386)
#    if defined(__clang__) || defined(__GNUC__)
#      define FASTCALL __attribute__((fastcall))
#    else
#      define FASTCALL
#    endif
#  else
#    define FASTCALL
#  endif
#endif
#ifndef __fastcall
#  define __fastcall FASTCALL
#endif
// Best-effort no-op for __declspec attributes on non-MSVC toolchains.
#ifndef __declspec
#  define __declspec(...)
#endif
#ifndef __int64
#  define __int64 long long
#endif
#ifndef _int64
#  define _int64 long long
#endif

#ifndef DACOM_API
#  define DACOM_API __stdcall
#endif

#ifndef NOVTABLE
#  define NOVTABLE
#endif

// MSVC __thiscall differs from Linux i386 member-function ABI. GCC and Clang
// support __attribute__((thiscall)) on i386; still validate per-interface.
#ifndef THISCALL
#  if defined(__i386__) || defined(__i386) || defined(i386)
#    if defined(__clang__) || defined(__GNUC__)
#      define THISCALL __attribute__((thiscall))
#    else
#      define THISCALL
#    endif
#  else
#    define THISCALL
#  endif
#endif

#ifndef STDMETHODCALLTYPE
#  define STDMETHODCALLTYPE WINAPI
#endif

#ifndef STDMETHODVCALLTYPE
#  define STDMETHODVCALLTYPE CDECL
#endif

using BOOL = int;
using BYTE = std::uint8_t;
using WORD = std::uint16_t;
using DWORD = std::uint32_t;
using LONG = std::int32_t;
using ULONG = std::uint32_t;
using UINT = std::uint32_t;
using INT = std::int32_t;
using HRESULT = std::int32_t;
using HANDLE = void*;
using HMODULE = void*;
using HINSTANCE = void*;
using HWND = void*; // Opaque placeholder; engine-facing adapters must provide required behavior, not just an integer handle.
using LPVOID = void*;
using LPCVOID = const void*;
using LPSTR = char*;
using LPCSTR = const char*;
using FARPROC = void (*)();
using SIZE_T = std::size_t;

using DWORD_PTR = std::uintptr_t;
using ULONG_PTR = std::uintptr_t;
using LONG_PTR  = std::intptr_t;
using INT_PTR   = std::intptr_t;
using UINT_PTR  = std::uintptr_t;

using WPARAM = UINT_PTR;
using LPARAM = LONG_PTR;
using LRESULT = LONG_PTR;

using CHAR = char;
using WCHAR = wchar_t;
using SHORT = std::int16_t;
using USHORT = std::uint16_t;
using FLOAT = float;

using HDC = void*;
using HFONT = void*;
using HBRUSH = void*;
using HPEN = void*;
using HICON = void*;
using HCURSOR = void*;
using HMENU = void*;
using HRGN = void*;
using HGDIOBJ = void*;
using HBITMAP = void*;
using HPALETTE = void*;
using ATOM = WORD;

typedef struct tagPOINT {
  LONG x;
  LONG y;
} POINT, *PPOINT, *LPPOINT;

struct POINTS {
  SHORT x;
  SHORT y;
};

typedef struct tagSIZE {
  LONG cx;
  LONG cy;
} SIZE, *PSIZE, *LPSIZE;

typedef struct tagRECT {
  LONG left;
  LONG top;
  LONG right;
  LONG bottom;
} RECT, *PRECT, *LPRECT;

typedef struct tagMSG {
  HWND hwnd;
  UINT message;
  WPARAM wParam;
  LPARAM lParam;
  DWORD time;
  POINT pt;
#ifdef _MAC
  DWORD lPrivate;
#endif
} MSG, *PMSG, *LPMSG;

using WNDPROC = LRESULT (CALLBACK*)(HWND, UINT, WPARAM, LPARAM);
struct WNDCLASSA {
  UINT      style;
  WNDPROC   lpfnWndProc;
  int       cbClsExtra;
  int       cbWndExtra;
  HINSTANCE hInstance;
  HICON     hIcon;
  HCURSOR   hCursor;
  HBRUSH    hbrBackground;
  LPCSTR    lpszMenuName;
  LPCSTR    lpszClassName;
};

struct WNDCLASSEXA {
  UINT      cbSize;
  UINT      style;
  WNDPROC   lpfnWndProc;
  int       cbClsExtra;
  int       cbWndExtra;
  HINSTANCE hInstance;
  HICON     hIcon;
  HCURSOR   hCursor;
  HBRUSH    hbrBackground;
  LPCSTR    lpszMenuName;
  LPCSTR    lpszClassName;
  HICON     hIconSm;
};

#ifndef LOGFONTA_DEFINED
#define LOGFONTA_DEFINED
struct LOGFONTA {
  LONG lfHeight;
  LONG lfWidth;
  LONG lfEscapement;
  LONG lfOrientation;
  LONG lfWeight;
  BYTE lfItalic;
  BYTE lfUnderline;
  BYTE lfStrikeOut;
  BYTE lfCharSet;
  BYTE lfOutPrecision;
  BYTE lfClipPrecision;
  BYTE lfQuality;
  BYTE lfPitchAndFamily;
  CHAR lfFaceName[32];
};
#endif


#ifndef _TEXTMETRICA_DEFINED
#define _TEXTMETRICA_DEFINED
typedef struct tagTEXTMETRICA {
  LONG tmHeight;
  LONG tmAscent;
  LONG tmDescent;
  LONG tmInternalLeading;
  LONG tmExternalLeading;
  LONG tmAveCharWidth;
  LONG tmMaxCharWidth;
  LONG tmWeight;
  LONG tmOverhang;
  LONG tmDigitizedAspectX;
  LONG tmDigitizedAspectY;
  BYTE tmFirstChar;
  BYTE tmLastChar;
  BYTE tmDefaultChar;
  BYTE tmBreakChar;
  BYTE tmItalic;
  BYTE tmUnderlined;
  BYTE tmStruckOut;
  BYTE tmPitchAndFamily;
  BYTE tmCharSet;
} TEXTMETRICA, *PTEXTMETRICA, *LPTEXTMETRICA;
typedef TEXTMETRICA TEXTMETRIC;
#endif

#ifndef MAKELONG
#  define MAKELONG(a, b) ((LONG)(((WORD)(((DWORD_PTR)(a)) & 0xffff)) | ((DWORD)((WORD)(((DWORD_PTR)(b)) & 0xffff))) << 16))
#endif
#ifndef MAKEWORD
#  define MAKEWORD(a, b) ((WORD)(((BYTE)(((DWORD_PTR)(a)) & 0xff)) | ((WORD)((BYTE)(((DWORD_PTR)(b)) & 0xff))) << 8))
#endif
#ifndef MAKELPARAM
#  define MAKELPARAM(l, h) ((LPARAM)(DWORD)MAKELONG(l, h))
#endif
#ifndef MAKEWPARAM
#  define MAKEWPARAM(l, h) ((WPARAM)(DWORD)MAKELONG(l, h))
#endif
#ifndef GET_X_LPARAM
#  define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#endif
#ifndef GET_Y_LPARAM
#  define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))
#endif

#ifndef TRUE
#  define TRUE 1
#endif
#ifndef FALSE
#  define FALSE 0
#endif
#ifndef MAX_PATH
#  define MAX_PATH 260
#endif

#ifndef LOBYTE
#  define LOBYTE(w) ((BYTE)(((DWORD_PTR)(w)) & 0xff))
#endif
#ifndef HIBYTE
#  define HIBYTE(w) ((BYTE)((((DWORD_PTR)(w)) >> 8) & 0xff))
#endif



// Win32 last-error accessors (implemented in linux file shim/platform shims).
DWORD GetLastError();
void SetLastError(DWORD err);

#ifndef APIENTRY
#  define APIENTRY
#endif

inline HMODULE LoadLibraryA(const char* lpLibFileName) {
  if (!lpLibFileName) return nullptr;
  // Try .so directly, then .dll→.so candidate.
  std::string name = lpLibFileName;
  // Strip path, try candidates.
  std::vector<std::string> cands;
  cands.push_back(name);
  // Replace .dll with .so
  size_t dllpos = name.rfind(".dll");
  if (dllpos != std::string::npos) {
    cands.push_back(name.substr(0, dllpos) + ".so");
  }
  // Try lib prefix.
  std::string base = name;
  size_t slash = base.find_last_of("/\\");
  if (slash != std::string::npos) base = base.substr(slash+1);
  if (base.find("lib") != 0) cands.push_back("lib" + base);
  for (auto& c : cands) {
    void* h = dlopen(c.c_str(), RTLD_NOW | RTLD_GLOBAL);
    if (h) return reinterpret_cast<HMODULE>(h);
  }
  return nullptr;
}
inline FARPROC GetProcAddress(HMODULE hModule, const char* lpProcName) {
  if (!hModule || !lpProcName) return nullptr;
  return reinterpret_cast<FARPROC>(dlsym(hModule, lpProcName));
}
inline BOOL FreeLibrary(HMODULE hLibModule) {
  if (!hLibModule) return FALSE;
  dlclose(hLibModule);
  return TRUE;
}
inline DWORD GetModuleFileNameA(HMODULE hModule, char* lpFilename, DWORD nSize) {
  if (!lpFilename || nSize == 0) return 0;
  if (hModule == nullptr) {
#if !defined(_WIN32)
    const ssize_t len = ::readlink("/proc/self/exe", lpFilename, static_cast<size_t>(nSize - 1));
    if (len > 0) {
      lpFilename[len] = '\0';
      return static_cast<DWORD>(len);
    }
#endif
  }
  lpFilename[0] = '\0';
  return 0;
}

struct OSVERSIONINFOA {
  DWORD dwOSVersionInfoSize;
  DWORD dwMajorVersion;
  DWORD dwMinorVersion;
  DWORD dwBuildNumber;
  DWORD dwPlatformId;
  char  szCSDVersion[128];
};
inline BOOL GetVersionExA(OSVERSIONINFOA* vi) {
  if (!vi) return 0;
  vi->dwMajorVersion = 5;
  vi->dwMinorVersion = 1;
  vi->dwBuildNumber = 2600;
  vi->dwPlatformId = 2; // VER_PLATFORM_WIN32_NT
  vi->szCSDVersion[0] = '\0';
  return 1;
}
inline HMODULE GetModuleHandleA(const char*) { return nullptr; }
inline DWORD SearchPathA(const char* lpPath, const char* lpFileName, const char* lpExtension,
                          DWORD nBufferLength, char* lpBuffer, char** lpFilePart) {
  // Engine-facing shim: perform minimal search with normalization/case-insensitive resolution.
  // Full Win32 SearchPath semantics (extension appending order, path list parsing nuances) are TODO.
  if (!lpFileName || !lpBuffer || nBufferLength == 0) return 0;
  auto try_resolve_copy = [&](const std::string& candidate) -> DWORD {
    const DWORD needed = static_cast<DWORD>(candidate.size() + 1);
    if (nBufferLength < needed) {
      if (nBufferLength > 0) {
        std::size_t i = 0;
        while (i < candidate.size() && i + 1 < nBufferLength) { lpBuffer[i] = candidate[i]; ++i; }
        lpBuffer[i] = '\0';
      }
      if (lpFilePart) {
        char* last = nullptr;
        if (nBufferLength > 0) {
          for (char* p = lpBuffer; *p; ++p) if (*p == '\\' || *p == '/') last = p + 1;
        }
        *lpFilePart = (last && *last != '\0') ? last : (nBufferLength > 0 ? lpBuffer : nullptr);
      }
      SetLastError(0);
      return needed;
    }
    std::memcpy(lpBuffer, candidate.c_str(), needed);
    if (lpFilePart) {
      char* last = nullptr;
      for (char* p = lpBuffer; *p; ++p) if (*p == '\\' || *p == '/') last = p + 1;
      *lpFilePart = (last && *last != '\0') ? last : lpBuffer;
    }
    SetLastError(0);
    return static_cast<DWORD>(candidate.size());
  };

  std::string name = lpFileName;
  std::string normalized = fl_port::normalize_path_separators(name);
  std::filesystem::path p(normalized);
  auto try_base = [&](const std::filesystem::path& base, const std::string& rel) -> std::optional<std::string> {
    fl_port::ResolveOptions opts; opts.allow_missing_tail = false;
    if (auto r = fl_port::resolve_path_case_insensitive(base, rel, opts)) return r->string();
    return std::nullopt;
  };

  if (p.is_absolute() || normalized.find('/') != std::string::npos) {
    if (auto r = try_base(p.is_absolute() ? p.root_path() : std::filesystem::current_path(),
                          p.is_absolute() ? p.relative_path().string() : normalized)) {
      return try_resolve_copy(*r);
    }
  } else {
    // Try explicit path dir first if provided, then current directory.
    if (lpPath && *lpPath) {
      std::filesystem::path base(fl_port::normalize_path_separators(lpPath));
      if (auto r = try_base(base, normalized)) return try_resolve_copy(*r);
    }
    if (auto r = try_base(std::filesystem::current_path(), normalized)) return try_resolve_copy(*r);
  }

  // Extension fallback (simple): append lpExtension if file has no extension and ext provided.
  if (lpExtension && *lpExtension && p.extension().empty()) {
    std::string ext = lpExtension;
    if (!ext.empty() && ext[0] != '.') ext = "." + ext;
    std::string with_ext = normalized + ext;
    std::filesystem::path pe(with_ext);
    if (pe.is_absolute() || with_ext.find('/') != std::string::npos) {
      if (auto r = try_base(pe.is_absolute() ? pe.root_path() : std::filesystem::current_path(),
                            pe.is_absolute() ? pe.relative_path().string() : with_ext)) {
        return try_resolve_copy(*r);
      }
    } else {
      if (lpPath && *lpPath) {
        if (auto r = try_base(std::filesystem::path(fl_port::normalize_path_separators(lpPath)), with_ext)) return try_resolve_copy(*r);
      }
      if (auto r = try_base(std::filesystem::current_path(), with_ext)) return try_resolve_copy(*r);
    }
  }

  // File not found in any searched location (Win32-compatible failure reporting).
  SetLastError(2); // ERROR_FILE_NOT_FOUND
  return 0;
}
inline void DisableThreadLibraryCalls(HMODULE) {}
inline void PostQuitMessage(int) {}

#ifndef DLL_PROCESS_ATTACH
#  define DLL_PROCESS_ATTACH 1
#endif
#ifndef DLL_PROCESS_DETACH
#  define DLL_PROCESS_DETACH 0
#endif
#ifndef DLL_THREAD_ATTACH
#  define DLL_THREAD_ATTACH 2
#endif
#ifndef DLL_THREAD_DETACH
#  define DLL_THREAD_DETACH 3
#endif

#ifndef HIWORD
#  define HIWORD(l) ((WORD)((((DWORD)(l)) >> 16) & 0xFFFF))
#endif
#ifndef LOWORD
#  define LOWORD(l) ((WORD)(((DWORD)(l)) & 0xFFFF))
#endif

// MessageBox / debug-break compatibility
#ifndef IDOK
#  define IDOK 1
#endif
#ifndef IDCANCEL
#  define IDCANCEL 2
#endif
#ifndef IDABORT
#  define IDABORT 3
#endif
#ifndef IDRETRY
#  define IDRETRY 4
#endif
#ifndef IDIGNORE
#  define IDIGNORE 5
#endif
#ifndef MB_OK
#  define MB_OK 0x00000000L
#endif
inline int MessageBoxA(void*, const char*, const char*, unsigned int) { return IDOK; }

#ifndef __debugbreak
#  if defined(__clang__) || defined(__GNUC__)
#    define __debugbreak() __builtin_trap()
#  else
#    include <cstdlib>
#    define __debugbreak() std::abort()
#  endif
#endif

// Version API stubs (dacom.cpp version query paths; guarded behavior may bypass in harness TU)
using VS_FIXEDFILEINFO = struct tagVS_FIXEDFILEINFO {
  DWORD dwSignature; DWORD dwStrucVersion; DWORD dwFileVersionMS; DWORD dwFileVersionLS;
  DWORD dwProductVersionMS; DWORD dwProductVersionLS; DWORD dwFileFlagsMask; DWORD dwFileFlags;
  DWORD dwFileOS; DWORD dwFileType; DWORD dwFileSubtype; DWORD dwFileDateMS; DWORD dwFileDateLS;
};
inline DWORD GetFileVersionInfoSizeA(const char*, DWORD*) { return 0; }
inline BOOL GetFileVersionInfoA(const char*, DWORD, DWORD, void*) { return 0; }
inline BOOL VerQueryValueA(const void*, const char*, void**, UINT*) { return 0; }

// Virtual memory stubs for upstream wrapper compilation (bring-up only).
#ifndef MEM_COMMIT
#  define MEM_COMMIT 0x1000
#endif
#ifndef MEM_RESERVE
#  define MEM_RESERVE 0x2000
#endif
#ifndef MEM_RELEASE
#  define MEM_RELEASE 0x8000
#endif
#ifndef PAGE_READWRITE
#  define PAGE_READWRITE 0x04
#endif

inline LPVOID VirtualAlloc(LPVOID, SIZE_T, DWORD, DWORD) { return nullptr; }
inline BOOL VirtualFree(LPVOID, SIZE_T, DWORD) { return 0; }

#ifndef TRUE
#  define TRUE 1
#endif
#ifndef FALSE
#  define FALSE 0
#endif

#ifndef S_OK
#  define S_OK ((HRESULT)0x00000000L)
#endif
#ifndef S_FALSE
#  define S_FALSE ((HRESULT)0x00000001L)
#endif
#ifndef E_FAIL
#  define E_FAIL ((HRESULT)0x80004005L)
#endif
#ifndef E_NOINTERFACE
#  define E_NOINTERFACE ((HRESULT)0x80004002L)
#endif
#ifndef E_OUTOFMEMORY
#  define E_OUTOFMEMORY ((HRESULT)0x8007000EL)
#endif
#ifndef E_INVALIDARG
#  define E_INVALIDARG ((HRESULT)0x80070057L)
#endif

#ifndef SUCCEEDED
#  define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
#endif
#ifndef FAILED
#  define FAILED(hr) (((HRESULT)(hr)) < 0)
#endif

struct GUID {
  std::uint32_t Data1;
  std::uint16_t Data2;
  std::uint16_t Data3;
  std::uint8_t  Data4[8];
};

inline bool operator==(const GUID& a, const GUID& b) {
  return a.Data1 == b.Data1 && a.Data2 == b.Data2 && a.Data3 == b.Data3 &&
         a.Data4[0] == b.Data4[0] && a.Data4[1] == b.Data4[1] &&
         a.Data4[2] == b.Data4[2] && a.Data4[3] == b.Data4[3] &&
         a.Data4[4] == b.Data4[4] && a.Data4[5] == b.Data4[5] &&
         a.Data4[6] == b.Data4[6] && a.Data4[7] == b.Data4[7];
}
inline bool operator!=(const GUID& a, const GUID& b) { return !(a == b); }
inline bool operator<(const GUID& a, const GUID& b) {
  if (a.Data1 != b.Data1) return a.Data1 < b.Data1;
  if (a.Data2 != b.Data2) return a.Data2 < b.Data2;
  if (a.Data3 != b.Data3) return a.Data3 < b.Data3;
  for (int i = 0; i < 8; ++i) if (a.Data4[i] != b.Data4[i]) return a.Data4[i] < b.Data4[i];
  return false;
}

using REFGUID = const GUID&;
using REFIID = const GUID&;
using REFCLSID = const GUID&;
using IID = GUID;
using CLSID = GUID;

inline bool IsEqualGUID(REFGUID rguid1, REFGUID rguid2) { return rguid1 == rguid2; }
#ifndef IsEqualIID
#  define IsEqualIID(riid1, riid2) IsEqualGUID(riid1, riid2)
#endif
#ifndef IsEqualCLSID
#  define IsEqualCLSID(rclsid1, rclsid2) IsEqualGUID(rclsid1, rclsid2)
#endif

// Common atomic intrinsics used by upstream code (MSVC Interlocked* API family).
#if defined(__clang__) || defined(__GNUC__)
inline long InterlockedIncrement(volatile long* value) {
  return __atomic_add_fetch(value, 1L, __ATOMIC_SEQ_CST);
}
inline long InterlockedDecrement(volatile long* value) {
  return __atomic_sub_fetch(value, 1L, __ATOMIC_SEQ_CST);
}
inline long InterlockedExchange(volatile long* target, long value) {
  return __atomic_exchange_n(target, value, __ATOMIC_SEQ_CST);
}
inline long InterlockedCompareExchange(volatile long* destination, long exchange, long comparand) {
  long expected = comparand;
  __atomic_compare_exchange_n(destination, &expected, exchange, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  return expected;
}
inline void* InterlockedCompareExchangePointer(void* volatile* destination, void* exchange, void* comparand) {
  void* expected = comparand;
  __atomic_compare_exchange_n(destination, &expected, exchange, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  return expected;
}
inline void* InterlockedExchangePointer(void* volatile* target, void* value) {
  return __atomic_exchange_n(target, value, __ATOMIC_SEQ_CST);
}
#else
inline long InterlockedIncrement(volatile long* value) { return ++(*value); }
inline long InterlockedDecrement(volatile long* value) { return --(*value); }
inline long InterlockedExchange(volatile long* target, long value) { long old = *target; *target = value; return old; }
inline long InterlockedCompareExchange(volatile long* destination, long exchange, long comparand) {
  long old = *destination;
  if (old == comparand) *destination = exchange;
  return old;
}
inline void* InterlockedCompareExchangePointer(void* volatile* destination, void* exchange, void* comparand) {
  void* old = *destination;
  if (old == comparand) *destination = exchange;
  return old;
}
inline void* InterlockedExchangePointer(void* volatile* target, void* value) { void* old = *target; *target = value; return old; }
#endif

// SEH placeholders for non-Windows bring-up (no structured exception semantics).
#ifndef _WIN32
#  ifndef __try
#    define __try
#  endif
#  ifndef __except
#    define __except(x) if (0)
#  endif
#endif

#ifndef interface
#  define interface struct
#endif

// NOTE: Do not define COM method macros (STDMETHOD/IFACEMETHOD) here yet;
// introduce them alongside a concrete DACOM/COM-interface porting strategy to
// avoid conflicts with upstream headers and calling-convention requirements.


// --- Probe/port compatibility additions (Linux bring-up) ---

// Additional probe/port compatibility stubs (M8.7 path B incremental)
#ifndef CP_UTF8
#define CP_UTF8 65001
#endif
#ifndef FILE_MAP_READ
#define FILE_MAP_READ 0x0004
#endif
#ifndef FILE_MAP_WRITE
#define FILE_MAP_WRITE 0x0002
#endif
#ifndef KEY_QUERY_VALUE
#define KEY_QUERY_VALUE 0x0001
#endif
#ifndef EXCEPTION_EXECUTE_HANDLER
#define EXCEPTION_EXECUTE_HANDLER 1
#endif

#ifndef _STARTUPINFOA_DEFINED
#define _STARTUPINFOA_DEFINED
typedef struct _STARTUPINFOA {
  DWORD cb;
  LPSTR lpReserved;
  LPSTR lpDesktop;
  LPSTR lpTitle;
  DWORD dwX, dwY, dwXSize, dwYSize;
  DWORD dwXCountChars, dwYCountChars;
  DWORD dwFillAttribute;
  DWORD dwFlags;
  WORD wShowWindow;
  WORD cbReserved2;
  BYTE* lpReserved2;
  HANDLE hStdInput, hStdOutput, hStdError;
} STARTUPINFOA, *LPSTARTUPINFOA;
#endif
#ifndef _PROCESS_INFORMATION_DEFINED
#define _PROCESS_INFORMATION_DEFINED
typedef struct _PROCESS_INFORMATION {
  HANDLE hProcess;
  HANDLE hThread;
  DWORD dwProcessId;
  DWORD dwThreadId;
} PROCESS_INFORMATION, *LPPROCESS_INFORMATION;
#endif
#ifndef _EXCEPTION_POINTERS_DEFINED
#define _EXCEPTION_POINTERS_DEFINED
typedef struct _EXCEPTION_POINTERS { void* ExceptionRecord; void* ContextRecord; } EXCEPTION_POINTERS, *PEXCEPTION_POINTERS;
#endif

inline DWORD GetTempPathA(DWORD nBufferLength, LPSTR lpBuffer) {
  const char* tmp = "/tmp/";
  if (!lpBuffer || nBufferLength == 0) return 5;
  strncpy(lpBuffer, tmp, nBufferLength - 1);
  lpBuffer[nBufferLength - 1] = '\0';
  return (DWORD)strlen(lpBuffer);
}
inline HANDLE GetCurrentProcess() { return (HANDLE)(intptr_t)-1; }
inline DWORD GetCurrentProcessId() { return 1; }
inline DWORD GetCurrentThreadId() { return 1; }
inline BOOL DuplicateHandle(HANDLE, HANDLE, HANDLE, HANDLE*, DWORD, BOOL, DWORD) { return FALSE; }
inline HANDLE CreateMutexA(void*, BOOL, LPCSTR) { return nullptr; }
#ifndef wsprintfA
#define wsprintfA sprintf
#endif


// Keep these inside #if !defined(_WIN32) so Windows builds use the native SDK.

#ifndef PM_NOREMOVE
#define PM_NOREMOVE 0x0000
#define PM_REMOVE   0x0001
#define PM_NOYIELD  0x0002
#endif
#ifndef WM_NULL
#define WM_NULL 0x0000
#define WM_CREATE 0x0001
#define WM_DESTROY 0x0002
#define WM_MOVE 0x0003
#define WM_SIZE 0x0005
#define WM_ACTIVATE 0x0006
#define WM_SETFOCUS 0x0007
#define WM_KILLFOCUS 0x0008
#define WM_ENABLE 0x000A
#define WM_PAINT 0x000F
#define WM_CLOSE 0x0010
#define WM_QUIT 0x0012
#define WM_SHOWWINDOW 0x0018
#define WM_ACTIVATEAPP 0x001C
#define WM_SETCURSOR 0x0020
#define WM_MOUSEACTIVATE 0x0021
#define WM_GETMINMAXINFO 0x0024
#define WM_WINDOWPOSCHANGING 0x0046
#define WM_WINDOWPOSCHANGED 0x0047
#define WM_NCCREATE 0x0081
#define WM_NCDESTROY 0x0082
#define WM_NCACTIVATE 0x0086
#define WM_KEYDOWN 0x0100
#define WM_KEYUP 0x0101
#define WM_CHAR 0x0102
#define WM_SYSKEYDOWN 0x0104
#define WM_SYSKEYUP 0x0105
#define WM_INITDIALOG 0x0110
#define WM_COMMAND 0x0111
#define WM_SYSCOMMAND 0x0112
#define WM_TIMER 0x0113
#define WM_MOUSEMOVE 0x0200
#define WM_LBUTTONDOWN 0x0201
#define WM_LBUTTONUP 0x0202
#define WM_RBUTTONDOWN 0x0204
#define WM_RBUTTONUP 0x0205
#define WM_MBUTTONDOWN 0x0207
#define WM_MBUTTONUP 0x0208
#define WM_MOUSEWHEEL 0x020A
#endif

#ifndef MB_OK
#define MB_OK 0x00000000L
#define MB_OKCANCEL 0x00000001L
#define MB_YESNO 0x00000004L
#define MB_ICONERROR 0x00000010L
#define MB_ICONWARNING 0x00000030L
#define MB_ICONINFORMATION 0x00000040L
#define IDOK 1
#define IDCANCEL 2
#define IDYES 6
#define IDNO 7
#endif

#ifndef _MEMORYSTATUS_DEFINED
#define _MEMORYSTATUS_DEFINED
typedef struct _MEMORYSTATUS {
  DWORD dwLength;
  DWORD dwMemoryLoad;
  SIZE_T dwTotalPhys;
  SIZE_T dwAvailPhys;
  SIZE_T dwTotalPageFile;
  SIZE_T dwAvailPageFile;
  SIZE_T dwTotalVirtual;
  SIZE_T dwAvailVirtual;
} MEMORYSTATUS, *LPMEMORYSTATUS;
#endif

#if defined(__cplusplus)
extern "C" {
#endif
inline BOOL WINAPI PeekMessageA(MSG* lpMsg, HWND, UINT, UINT, UINT) { if (lpMsg) *lpMsg = MSG{}; return FALSE; }
inline BOOL WINAPI GetMessageA(MSG* lpMsg, HWND, UINT, UINT) { if (lpMsg) *lpMsg = MSG{}; return FALSE; }
inline BOOL WINAPI TranslateMessage(const MSG*) { return FALSE; }
inline LRESULT WINAPI DispatchMessageA(const MSG*) { return 0; }
inline int WINAPI MessageBoxW(void*, const wchar_t*, const wchar_t*, unsigned int) { return IDOK; }
inline BOOL WINAPI SetWindowTextA(HWND, LPCSTR) { return TRUE; }
inline BOOL WINAPI SetWindowTextW(HWND, const wchar_t*) { return TRUE; }
inline HWND WINAPI GetDlgItem(HWND, int) { return nullptr; }
inline BOOL WINAPI EndDialog(HWND, INT_PTR) { return TRUE; }
inline INT_PTR WINAPI DialogBoxParamA(HINSTANCE, LPCSTR, HWND, INT_PTR (CALLBACK*)(HWND, UINT, WPARAM, LPARAM), LPARAM) { return 0; }
inline INT_PTR WINAPI DialogBoxParamW(HINSTANCE, const wchar_t*, HWND, INT_PTR (CALLBACK*)(HWND, UINT, WPARAM, LPARAM), LPARAM) { return 0; }
inline void WINAPI GlobalMemoryStatus(LPMEMORYSTATUS lpBuffer) {
  if (!lpBuffer) return;
  lpBuffer->dwLength = sizeof(MEMORYSTATUS);
  lpBuffer->dwMemoryLoad = 0;
  lpBuffer->dwTotalPhys = lpBuffer->dwAvailPhys = 512u * 1024u * 1024u;
  lpBuffer->dwTotalPageFile = lpBuffer->dwAvailPageFile = 512u * 1024u * 1024u;
  lpBuffer->dwTotalVirtual = lpBuffer->dwAvailVirtual = 512u * 1024u * 1024u;
}
BOOL WINAPI EnableWindow(HWND, BOOL);
HWND WINAPI GetDesktopWindow(void);
BOOL WINAPI GetWindowRect(HWND, LPRECT);
BOOL WINAPI GetClientRect(HWND, LPRECT);
LRESULT WINAPI SendMessageA(HWND, UINT, WPARAM, LPARAM);
LRESULT WINAPI SendMessageW(HWND, UINT, WPARAM, LPARAM);
#if defined(__cplusplus)
} // extern "C"
#endif

#ifndef BM_GETCHECK
#define BM_GETCHECK 0x00F0
#define BM_SETCHECK 0x00F1
#endif
#ifndef MAKEINTRESOURCEA
#define MAKEINTRESOURCEA(i) ((LPSTR)((ULONG_PTR)((WORD)(i))))
#define MAKEINTRESOURCEW(i) ((wchar_t*)((ULONG_PTR)((WORD)(i))))
#define MAKEINTRESOURCE MAKEINTRESOURCEA
#endif
#ifndef _control87
inline unsigned int _control87(unsigned int, unsigned int) { return 0; }
#endif

#endif // !_WIN32

// COM stubs for Linux bring-up.
inline HRESULT CoInitialize(void* pvReserved) { (void)pvReserved; return 0; /* S_OK */ }
inline void CoUninitialize() {}
// Forward declare for COM adapter.
inline void* g_render_pipeline_instance = nullptr;

inline HRESULT CoCreateInstance(const GUID& rclsid, void* pUnkOuter, DWORD dwClsContext, const GUID& riid, void** ppv) {
  (void)rclsid; (void)pUnkOuter; (void)dwClsContext; (void)riid;
  if (ppv) *ppv = nullptr;
  // Best-effort: if a RenderPipeline instance was registered, return it.
  if (g_render_pipeline_instance && ppv) {
    *ppv = g_render_pipeline_instance;
    return 0; // S_OK
  }
  return -2147467259; // E_NOINTERFACE
}

// Resource shims
inline HBITMAP LoadBitmapA(HINSTANCE, LPCSTR) { return nullptr; }
inline HBITMAP LoadBitmapW(HINSTANCE, const wchar_t*) { return nullptr; }

// IME shims
inline void* ImmGetContext(HWND) { return nullptr; }
inline BOOL ImmReleaseContext(HWND, void*) { return TRUE; }
inline BOOL ImmSetCompositionStringW(void*, DWORD, const void*, DWORD, const void*, DWORD) { return FALSE; }
inline BOOL ImmGetCompositionStringW(void*, DWORD, void*, DWORD) { return 0; }

// DirectPlay stubs
struct IDirectPlay8Peer;
struct IDirectPlay8Server;
struct IDirectPlay8Client;


// Additional Win32/CRT constants for upstream TU compilation.
#ifndef CP_ACP
#define CP_ACP 0
#endif
#ifndef CP_UTF8
#define CP_UTF8 65001
#endif
#ifndef IDYES
#define IDYES 6
#endif
#ifndef IDNO
#define IDNO 7
#endif
#ifndef IDOK
#define IDOK 1
#endif
#ifndef EXCEPTION_CONTINUE_SEARCH
#define EXCEPTION_CONTINUE_SEARCH 0
#endif
#ifndef EXCEPTION_EXECUTE_HANDLER
#define EXCEPTION_EXECUTE_HANDLER 1
#endif
#ifndef EXCEPTION_CONTINUE_EXECUTION
#define EXCEPTION_CONTINUE_EXECUTION (-1)
#endif

// CRT math stubs
inline void _clearfp() {}
inline unsigned int _controlfp(unsigned int, unsigned int) { return 0; }

inline void* SetUnhandledExceptionFilter(void* filter) { (void)filter; return nullptr; }
// Overload for typed function pointer
template<typename T>
inline T SetUnhandledExceptionFilter(T filter) { (void)filter; return nullptr; }

inline int MultiByteToWideChar(unsigned int CodePage, DWORD dwFlags, const char* lpMultiByteStr, int cbMultiByte, wchar_t* lpWideCharStr, int cchWideChar) {
  (void)CodePage; (void)dwFlags; (void)lpMultiByteStr; (void)cbMultiByte; (void)lpWideCharStr; (void)cchWideChar;
  if (lpWideCharStr && cchWideChar > 0) lpWideCharStr[0] = L'\0';
  return 0;
}


// Window class styles
#ifndef CS_DBLCLKS
#define CS_DBLCLKS 0x0008
#endif
#ifndef CS_HREDRAW
#define CS_HREDRAW 0x0002
#endif
#ifndef CS_VREDRAW
#define CS_VREDRAW 0x0001
#endif
#ifndef CS_OWNDC
#define CS_OWNDC 0x0020
#endif

// Standard cursors
#ifndef IDC_ARROW
#define IDC_ARROW (LPCSTR)32512
#endif
#ifndef IDC_IBEAM
#define IDC_IBEAM (LPCSTR)32513
#endif

// Stock objects
#ifndef BLACK_BRUSH
#define BLACK_BRUSH 4
#endif
#ifndef WHITE_BRUSH
#define WHITE_BRUSH 0
#endif
#ifndef NULL_BRUSH
#define NULL_BRUSH 5
#endif

// GetStockObject stub
inline void* GetStockObject(int /*fnObject*/) { return nullptr; }

// __try/__except SEH stubs for non-MSVC
#if !defined(_MSC_VER)
#  if !defined(__try)
#    define __try
#  endif
#  if !defined(__except)
#    define __except(x) if (false)
#  endif
#  if !defined(__finally)
#    define __finally if (true)
#  endif
#endif

inline BOOL CreateProcessA(LPCSTR, LPSTR, void*, void*, BOOL, DWORD, void*, LPCSTR, void*, void*) { return FALSE; }

#define WAIT_ABANDONED 0x00000080

inline BOOL ReleaseMutex(void*) { return TRUE; }
