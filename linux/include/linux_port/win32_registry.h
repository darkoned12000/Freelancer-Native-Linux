#pragma once

// Win32 registry shim skeleton for Linux bring-up (#1).
// Runtime persistence/config should ultimately map to XDG-compliant storage;
// these stubs exist to unblock compilation/harnessing of registry-touching code.

#if defined(_WIN32)
#include <windows.h>
#else

#include "linux_port/win32_compat.h"

using HKEY = void*;
using REGSAM = DWORD;
using LSTATUS = LONG;

#ifndef HKEY_CLASSES_ROOT
#  define HKEY_CLASSES_ROOT ((HKEY)(intptr_t)0x80000000)
#endif
#ifndef HKEY_CURRENT_USER
#  define HKEY_CURRENT_USER ((HKEY)(intptr_t)0x80000001)
#endif
#ifndef HKEY_LOCAL_MACHINE
#  define HKEY_LOCAL_MACHINE ((HKEY)(intptr_t)0x80000002)
#endif
#ifndef HKEY_USERS
#  define HKEY_USERS ((HKEY)(intptr_t)0x80000003)
#endif

#ifndef ERROR_SUCCESS
#  define ERROR_SUCCESS 0L
#endif
#ifndef ERROR_FILE_NOT_FOUND
#  define ERROR_FILE_NOT_FOUND 2L
#endif
#ifndef ERROR_ACCESS_DENIED
#  define ERROR_ACCESS_DENIED 5L
#endif
#ifndef ERROR_NOT_SUPPORTED
#  define ERROR_NOT_SUPPORTED 50L
#endif

// Linkable fallback stubs (XDG-backed semantics TBD).
inline LSTATUS RegOpenKeyExA(HKEY, const char*, DWORD, REGSAM, HKEY*) { return ERROR_FILE_NOT_FOUND; }
inline LSTATUS RegCreateKeyExA(HKEY, const char*, DWORD, char*, DWORD, REGSAM, void*, HKEY*, DWORD*) { return ERROR_NOT_SUPPORTED; }
inline LSTATUS RegCloseKey(HKEY) { return ERROR_SUCCESS; }
inline LSTATUS RegQueryValueExA(HKEY, const char*, DWORD*, DWORD*, BYTE*, DWORD*) { return ERROR_FILE_NOT_FOUND; }
inline LSTATUS RegSetValueExA(HKEY, const char*, DWORD, DWORD, const BYTE*, DWORD) { return ERROR_NOT_SUPPORTED; }

#endif // !_WIN32
