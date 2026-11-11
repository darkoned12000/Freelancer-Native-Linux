#pragma once
// Minimal MSVC path helper compatibility for Linux harness/upstream-wrapper TUs.

#if defined(_WIN32)
#include <stdlib.h>
#else

#include <algorithm>
#include <cstddef>
#include <cstring>

#ifndef _MAX_DRIVE
#  define _MAX_DRIVE 3
#endif
#ifndef _MAX_DIR
#  define _MAX_DIR 256
#endif
#ifndef _MAX_FNAME
#  define _MAX_FNAME 256
#endif
#ifndef _MAX_EXT
#  define _MAX_EXT 256
#endif

inline void _splitpath(const char* path, char* drive, char* dir, char* fname, char* ext) {
  if (drive) drive[0] = '\0';
  if (dir) dir[0] = '\0';
  if (fname) fname[0] = '\0';
  if (ext) ext[0] = '\0';
  if (!path) return;

  const char* p = path;
  // Drive letter (C:)
  if (p[0] && p[1] == ':') {
    if (drive) { drive[0] = p[0]; drive[1] = ':'; drive[2] = '\0'; }
    p += 2;
  }

  const char* last_slash = nullptr;
  for (const char* s = p; *s; ++s) if (*s == '/' || *s == '\\') last_slash = s;
  const char* name_begin = last_slash ? (last_slash + 1) : p;

  if (dir) {
    const std::size_t n = static_cast<std::size_t>(name_begin - p);
    if (n) std::memcpy(dir, p, n);
    dir[n] = '\0';
  }

  const char* last_dot = nullptr;
  for (const char* s = name_begin; *s; ++s) if (*s == '.') last_dot = s;
  if (!last_dot || last_dot == name_begin) {
    if (fname) std::strcpy(fname, name_begin);
  } else {
    if (fname) {
      const std::size_t n = static_cast<std::size_t>(last_dot - name_begin);
      std::memcpy(fname, name_begin, n);
      fname[n] = '\0';
    }
    if (ext) std::strcpy(ext, last_dot);
  }
}

inline void _makepath(char* path, const char* drive, const char* dir, const char* fname, const char* ext) {
  if (!path) return;
  path[0] = '\0';
  if (drive && *drive) std::strcat(path, drive);
  if (dir && *dir) {
    std::strcat(path, dir);
    const std::size_t len = std::strlen(path);
    if (len > 0) {
      const char c = path[len - 1];
      if (c != '/' && c != '\\') {
        std::strcat(path, (std::strchr(dir, '/') != nullptr) ? "/" : "\\");
      }
    }
  }
  if (fname && *fname) std::strcat(path, fname);
  if (ext && *ext) {
    if (ext[0] != '.') std::strcat(path, ".");
    std::strcat(path, ext);
  }
}

#endif // !_WIN32
