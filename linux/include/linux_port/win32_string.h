#pragma once

// Minimal Win32/MSVC C string compatibility helpers for Linux bring-up (#1).

#if defined(_WIN32)
#include <windows.h>
#include <string.h>
#else

#include <cerrno>
#include <cstdarg>
#include <cstdlib>
#include <utility>
#include <cstdio>
#include <cstring>
#include <strings.h>

using errno_t = int;
#ifndef _TRUNCATE
#  define _TRUNCATE ((size_t)-1)
#endif
#ifndef EINVAL
#  define EINVAL 22
#endif
#ifndef ERANGE
#  define ERANGE 34
#endif
#ifndef STRUNCATE
#  define STRUNCATE 80
#endif

inline int _stricmp(const char* a, const char* b) {
  if (!a || !b) return (a == b) ? 0 : (a ? 1 : -1);
  return strcasecmp(a, b);
}

inline int stricmp(const char* a, const char* b) { return _stricmp(a, b); }

inline int _wcsnicmp(const wchar_t* a, const wchar_t* b, size_t count) {
  if (count == 0) return 0;
  if (!a || !b) return (a == b) ? 0 : (a ? 1 : -1);
  for (size_t i = 0; i < count; ++i) {
    wchar_t ca = a[i], cb = b[i];
    if (ca >= L'A' && ca <= L'Z') ca += 32;
    if (cb >= L'A' && cb <= L'Z') cb += 32;
    if (ca != cb) return (ca < cb) ? -1 : 1;
    if (ca == L'\0') return 0;
  }
  return 0;
}

inline char* _strdup(const char* s) {
  if (!s) return nullptr;
  size_t n = strlen(s) + 1;
  char* p = (char*)malloc(n);
  if (p) memcpy(p, s, n);
  return p;
}
inline char* _strlwr(char* s) {
  if (s) for (char* p = s; *p; ++p) if (*p >= 'A' && *p <= 'Z') *p += 32;
  return s;
}

inline int WideCharToMultiByte(unsigned int CodePage, DWORD dwFlags, const wchar_t* lpWideCharStr, int cchWideChar, char* lpMultiByteStr, int cbMultiByte, const char* lpDefaultChar, int* lpUsedDefaultChar) {
  (void)CodePage; (void)dwFlags; (void)lpDefaultChar; (void)lpUsedDefaultChar;
  if (!lpWideCharStr) return 0;
  int n = (cchWideChar < 0) ? (int)wcslen(lpWideCharStr) : cchWideChar;
  if (lpMultiByteStr && cbMultiByte > 0) {
    int i = 0;
    for (; i < n && i < cbMultiByte - 1; ++i) lpMultiByteStr[i] = (char)lpWideCharStr[i];
    lpMultiByteStr[i] = '\0';
  }
  return n;
}

inline int _strnicmp(const char* a, const char* b, size_t count) {
  if (count == 0) return 0;
  if (!a || !b) return (a == b) ? 0 : (a ? 1 : -1);
  return strncasecmp(a, b, count);
}
inline int strnicmp(const char* a, const char* b, size_t count) { return _strnicmp(a, b, count); }

inline int _vsnprintf(char* buffer, size_t count, const char* format, va_list argptr) {
  return vsnprintf(buffer, count, format, argptr);
}

inline errno_t strncpy_s(char* dest, size_t destsz, const char* src, size_t count) {
  if (!dest || destsz == 0) return EINVAL;
  if (!src) {
    dest[0] = '\0';
    return EINVAL;
  }
  size_t i = 0;
  while (i < count && src[i] != '\0') {
    if (i + 1 >= destsz) {
      if (count == _TRUNCATE) {
        dest[i] = '\0';
        return STRUNCATE;
      }
      dest[0] = '\0';
      return ERANGE;
    }
    dest[i] = src[i];
    ++i;
  }
  if (i >= destsz) {
    dest[0] = '\0';
    return ERANGE;
  }
  dest[i] = '\0';
  return 0;
}

#endif // !_WIN32

inline errno_t _ecvt_s(char* buffer, size_t sizeInBytes, double value, int count, int* dec, int* sign) {
  (void)value;(void)count;
  if (buffer && sizeInBytes) buffer[0]='\0';
  if (dec) *dec=0;
  if (sign) *sign=0;
  return 0;
}

inline int _swprintf(wchar_t* buffer, const wchar_t* format, ...) {
  va_list args;
  va_start(args, format);
  int r = vswprintf(buffer, 1024, format, args);
  va_end(args);
  return r;
}

inline int _wcsicmp(const wchar_t* a, const wchar_t* b) {
  if (!a || !b) return (a == b) ? 0 : (a ? 1 : -1);
  while (*a && *b) {
    wchar_t ca = *a, cb = *b;
    if (ca >= L'A' && ca <= L'Z') ca += 32;
    if (cb >= L'A' && cb <= L'Z') cb += 32;
    if (ca != cb) return (ca < cb) ? -1 : 1;
    ++a; ++b;
  }
  return (int)(*a - *b);
}

inline int _snwprintf(wchar_t* buffer, size_t count, const wchar_t* format, ...) {
  va_list args;
  va_start(args, format);
  int r = vswprintf(buffer, count, format, args);
  va_end(args);
  return r;
}

inline wchar_t* CharUpperW(wchar_t* s) {
  if (s) for (wchar_t* p = s; *p; ++p) if (*p >= L'a' && *p <= L'z') *p -= 32;
  return s;
}

inline int _isnan(double x) { return __builtin_isnan(x); }

inline wchar_t* _ltow(long value, wchar_t* buffer, int radix) {
  if (!buffer) return nullptr;
  if (radix == 10) swprintf(buffer, 64, L"%ld", value);
  else if (radix == 16) swprintf(buffer, 64, L"%lx", value);
  else swprintf(buffer, 64, L"%ld", value);
  return buffer;
}

inline wchar_t* _itow(int value, wchar_t* buffer, int radix) {
  if (!buffer) return nullptr;
  if (radix == 10) swprintf(buffer, 64, L"%d", value);
  else if (radix == 16) swprintf(buffer, 64, L"%x", value);
  else swprintf(buffer, 64, L"%d", value);
  return buffer;
}
inline wchar_t* _wcsdup(const wchar_t* s) {
  if (!s) return nullptr;
  size_t n = (wcslen(s) + 1) * sizeof(wchar_t);
  wchar_t* p = (wchar_t*)malloc(n);
  if (p) memcpy(p, s, n);
  return p;
}

inline int _wcsicoll(const wchar_t* a, const wchar_t* b) {
  // case-insensitive collation = case-insensitive compare for bring-up
  if (!a || !b) return (a == b) ? 0 : (a ? 1 : -1);
  while (*a && *b) {
    wchar_t ca = *a, cb = *b;
    if (ca >= L'A' && ca <= L'Z') ca += 32;
    if (cb >= L'A' && cb <= L'Z') cb += 32;
    if (ca != cb) return (ca < cb) ? -1 : 1;
    ++a; ++b;
  }
  return (int)(*a - *b);
}

inline wchar_t* _ultow(unsigned long value, wchar_t* buffer, int radix) {
  if (!buffer) return nullptr;
  if (radix == 10) swprintf(buffer, 64, L"%lu", value);
  else if (radix == 16) swprintf(buffer, 64, L"%lx", value);
  else swprintf(buffer, 64, L"%lu", value);
  return buffer;
}

template <size_t N, typename... Args>
int sprintf_s(char (&buffer)[N], const char* format, Args&&... args) {
  return snprintf(buffer, N, format, std::forward<Args>(args)...);
}
template <size_t N, typename... Args>
int sprintf_s(char (&buffer)[N], size_t sizeOfBuffer, const char* format, Args&&... args) {
  return snprintf(buffer, sizeOfBuffer, format, std::forward<Args>(args)...);
}

inline char* _strupr(char* s) {
  if (s) for (char* p = s; *p; ++p) if (*p >= 'a' && *p <= 'z') *p -= 32;
  return s;
}

inline char* _ltoa(long value, char* buffer, int radix) {
  if (!buffer) return nullptr;
  if (radix == 10) sprintf(buffer, "%ld", value);
  else if (radix == 16) sprintf(buffer, "%lx", value);
  else sprintf(buffer, "%ld", value);
  return buffer;
}
