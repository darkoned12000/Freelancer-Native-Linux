#pragma once
// Minimal <io.h> compatibility shim for compiling upstream Win32/MSVCRT-oriented
// sources in Linux harness TUs. Expand only as required by ported code.

#include <sys/types.h>
#include <unistd.h>

#ifndef _finddata_t
struct _finddata_t { int unused; };
#endif

inline intptr_t _findfirst(const char*, struct _finddata_t*) { return -1; }
inline int _findnext(intptr_t, struct _finddata_t*) { return -1; }
inline int _findclose(intptr_t) { return -1; }
inline int _access(const char* path, int mode) {
  if (!path) return -1;
  return ::access(path, mode);
}

#ifndef F_OK
#define F_OK 0
#endif
#ifndef R_OK
#define R_OK 4
#endif
#ifndef W_OK
#define W_OK 2
#endif
