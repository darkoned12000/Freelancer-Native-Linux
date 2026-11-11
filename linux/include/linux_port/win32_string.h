#pragma once

// Minimal Win32/MSVC C string compatibility helpers for Linux bring-up (#1).

#if defined(_WIN32)
#include <windows.h>
#include <string.h>
#else

#include <cerrno>
#include <cstdarg>
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
