#pragma once

// Win32 file API/type shim skeleton for Linux bring-up (#1).
// This provides minimal types/constants commonly referenced by ReadFile/DACOM
// interfaces and path-handling code. Function implementations/mappings will be
// added incrementally as modules are ported/harnessed.

#if defined(_WIN32)
#include <windows.h>
#else

#include "linux_port/win32_compat.h"

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#ifndef INVALID_HANDLE_VALUE
#  define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)
#endif

#ifndef MAX_PATH
#  define MAX_PATH 260
#endif

// Access/share/create/flags (subset; values match Win32 for ABI/data compat where needed)
#ifndef GENERIC_READ
#  define GENERIC_READ 0x80000000u
#endif
#ifndef GENERIC_WRITE
#  define GENERIC_WRITE 0x40000000u
#endif
#ifndef FILE_SHARE_READ
#  define FILE_SHARE_READ 0x00000001u
#endif
#ifndef FILE_SHARE_WRITE
#  define FILE_SHARE_WRITE 0x00000002u
#endif
#ifndef OPEN_EXISTING
#  define OPEN_EXISTING 3u
#endif
#ifndef CREATE_ALWAYS
#  define CREATE_ALWAYS 2u
#endif
#ifndef FILE_ATTRIBUTE_DIRECTORY
#  define FILE_ATTRIBUTE_DIRECTORY 0x00000010u
#endif
#ifndef FILE_ATTRIBUTE_NORMAL
#  define FILE_ATTRIBUTE_NORMAL 0x00000080u
#endif
#ifndef FILE_ATTRIBUTE_ARCHIVE
#  define FILE_ATTRIBUTE_ARCHIVE 0x00000020u
#endif
#ifndef FILE_ATTRIBUTE_READONLY
#  define FILE_ATTRIBUTE_READONLY 0x00000001u
#endif
#ifndef INVALID_FILE_ATTRIBUTES
#  define INVALID_FILE_ATTRIBUTES ((DWORD)0xFFFFFFFFu)
#endif
#ifndef INVALID_FILE_SIZE
#  define INVALID_FILE_SIZE ((DWORD)0xFFFFFFFFu)
#endif

struct FILETIME {
  DWORD dwLowDateTime;
  DWORD dwHighDateTime;
};

struct WIN32_FIND_DATAA {
  DWORD    dwFileAttributes;
  FILETIME ftCreationTime;
  FILETIME ftLastAccessTime;
  FILETIME ftLastWriteTime;
  DWORD    nFileSizeHigh;
  DWORD    nFileSizeLow;
  DWORD    dwReserved0;
  DWORD    dwReserved1;
  char     cFileName[MAX_PATH];
  char     cAlternateFileName[14];
};

#ifndef FILE_BEGIN
#  define FILE_BEGIN 0u
#endif
#ifndef FILE_CURRENT
#  define FILE_CURRENT 1u
#endif
#ifndef FILE_END
#  define FILE_END 2u
#endif

#ifndef FILE_MAP_READ
#  define FILE_MAP_READ 0x0004u
#endif
#ifndef PAGE_READONLY
#  define PAGE_READONLY 0x02u
#endif

// Linux-backed Win32 file shim implementations for bring-up (#1.2).
// These provide real file IO for common paths (CreateFileA/ReadFile/CloseHandle/etc.)
// with Windows-path normalization and case-insensitive resolution via path_util.

HANDLE CreateFileA(const char* lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
                   void* lpSecurityAttributes, DWORD dwCreationDisposition,
                   DWORD dwFlagsAndAttributes, void* hTemplateFile);
BOOL ReadFile(HANDLE hFile, void* lpBuffer, DWORD nNumberOfBytesToRead,
              DWORD* lpNumberOfBytesRead, void* lpOverlapped);
BOOL WriteFile(HANDLE hFile, const void* lpBuffer, DWORD nNumberOfBytesToWrite,
               DWORD* lpNumberOfBytesWritten, void* lpOverlapped);
DWORD GetFileSize(HANDLE hFile, DWORD* lpFileSizeHigh);
inline DWORD GetFileSize(HANDLE hFile, unsigned long* lpFileSizeHigh) {
  DWORD hi = 0;
  const DWORD lo = GetFileSize(hFile, &hi);
  if (lpFileSizeHigh) *lpFileSizeHigh = hi;
  return lo;
}
inline DWORD GetFileSize(HANDLE hFile, std::nullptr_t) { return GetFileSize(hFile, (DWORD*)nullptr); }
DWORD SetFilePointer(HANDLE hFile, LONG lDistanceToMove, LONG* lpDistanceToMoveHigh, DWORD dwMoveMethod);
BOOL SetEndOfFile(HANDLE hFile);
DWORD GetFileAttributesA(const char* lpFileName);
BOOL GetFileTime(HANDLE hFile, FILETIME* lpCreationTime, FILETIME* lpLastAccessTime, FILETIME* lpLastWriteTime);
HANDLE FindFirstFileA(const char* lpFileName, WIN32_FIND_DATAA* lpFindFileData);
BOOL FindNextFileA(HANDLE hFindFile, WIN32_FIND_DATAA* lpFindFileData);
BOOL FindClose(HANDLE hFindFile);
HANDLE CreateFileMappingA(HANDLE hFile, void* lpFileMappingAttributes, DWORD flProtect,
                          DWORD dwMaximumSizeHigh, DWORD dwMaximumSizeLow, const char* lpName);
void* MapViewOfFile(HANDLE hFileMappingObject, DWORD dwDesiredAccess, DWORD dwFileOffsetHigh,
                    DWORD dwFileOffsetLow, size_t dwNumberOfBytesToMap);
BOOL UnmapViewOfFile(const void* lpBaseAddress);
BOOL CloseHandle(HANDLE hObject);
DWORD GetLastError();
void SetLastError(DWORD err);

struct SECURITY_ATTRIBUTES {
  DWORD nLength;
  LPVOID lpSecurityDescriptor;
  BOOL bInheritHandle;
};
using LPSECURITY_ATTRIBUTES = SECURITY_ATTRIBUTES*;

inline BOOL DosDateTimeToFileTime(WORD, WORD, FILETIME* lpFileTime) {
  if (lpFileTime) { lpFileTime->dwLowDateTime = 0; lpFileTime->dwHighDateTime = 0; }
  return FALSE;
}

inline DWORD GetFullPathNameA(const char* lpFileName, DWORD nBufferLength, char* lpBuffer, char** lpFilePart) {
  if (!lpFileName || !lpBuffer || nBufferLength == 0) return 0;
  DWORD i = 0;
  while (lpFileName[i] && i + 1 < nBufferLength) { lpBuffer[i] = lpFileName[i]; ++i; }
  lpBuffer[i] = '\0';
  if (lpFilePart) {
    char* last = nullptr;
    for (char* p = lpBuffer; *p; ++p) if (*p == '\\' || *p == '/') last = p + 1;
    *lpFilePart = last ? last : lpBuffer;
  }
  return i;
}

inline char* _fullpath(char* absPath, const char* relPath, size_t maxLength) {
  if (!relPath) return nullptr;
  size_t sz = maxLength ? maxLength : MAX_PATH;
  char* target = absPath ? absPath : static_cast<char*>(std::malloc(sz));
  if (!target) return nullptr;
  GetFullPathNameA(relPath, static_cast<DWORD>(sz), target, nullptr);
  return target;
}

inline int _snprintf(char* buffer, size_t count, const char* format, ...) {
  if (!buffer || count == 0) return -1;
  va_list args;
  va_start(args, format);
  int r = vsnprintf(buffer, count, format, args);
  va_end(args);
  return r;
}

#endif // !_WIN32
