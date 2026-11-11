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

#if !defined(_WIN32)
#include <sys/types.h>
#include <unistd.h>
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

#ifndef APIENTRY
#  define APIENTRY
#endif

inline HMODULE LoadLibraryA(const char*) { return nullptr; }
inline FARPROC GetProcAddress(HMODULE, const char*) { return nullptr; }
inline BOOL FreeLibrary(HMODULE) { return 0; }
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
inline DWORD SearchPathA(const char*, const char* lpFileName, const char*, DWORD nBufferLength, char* lpBuffer, char** lpFilePart) {
  if (!lpFileName || !lpBuffer || nBufferLength == 0) return 0;
  // Minimal stub: copy filename as-is (no path search). Callers may treat 0 as failure.
  std::size_t i = 0;
  while (lpFileName[i] && i + 1 < nBufferLength) { lpBuffer[i] = lpFileName[i]; ++i; }
  lpBuffer[i] = '\0';
  if (lpFilePart) {
    char* last = nullptr;
    for (char* p = lpBuffer; *p; ++p) if (*p == '\\' || *p == '/') last = p + 1;
    *lpFilePart = last ? last : lpBuffer;
  }
  return static_cast<DWORD>(i);
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

// SEH placeholders for non-Windows bring-up (no structured exception semantics).
#ifndef _WIN32
#  ifndef __try
#    define __try if (true)
#  endif
#  ifndef __except
#    define __except(x) if (false)
#  endif
#endif

#ifndef interface
#  define interface struct
#endif

// NOTE: Do not define COM method macros (STDMETHOD/IFACEMETHOD) here yet;
// introduce them alongside a concrete DACOM/COM-interface porting strategy to
// avoid conflicts with upstream headers and calling-convention requirements.

#endif // !_WIN32
