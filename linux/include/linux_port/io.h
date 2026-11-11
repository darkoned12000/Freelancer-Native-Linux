#pragma once
// Minimal <io.h> compatibility shim for compiling upstream Win32/MSVCRT-oriented
// sources in Linux harness TUs. Expand only as required by ported code.

#include "linux_port/path_util.h"

#include <cerrno>
#include <filesystem>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef _finddata_t
struct _finddata_t { int unused; };
#endif

inline intptr_t _findfirst(const char*, struct _finddata_t*) { return -1; }
inline int _findnext(intptr_t, struct _finddata_t*) { return -1; }
inline int _findclose(intptr_t) { return -1; }

#ifndef F_OK
#define F_OK 0
#endif
#ifndef R_OK
#define R_OK 4
#endif
#ifndef W_OK
#define W_OK 2
#endif
#ifndef X_OK
#define X_OK 1
#endif

inline std::string fl_port_io_resolve_existing(const char* path) {
  if (!path || !*path) return {};
  const std::string normalized = fl_port::normalize_path_separators(path);
  std::filesystem::path p(normalized);
  auto base = p.is_absolute() ? p.root_path() : std::filesystem::current_path();
  auto rel = p.is_absolute() ? p.relative_path().string() : p.string();
  fl_port::ResolveOptions opts;
  opts.allow_missing_tail = false;
  if (auto r = fl_port::resolve_path_case_insensitive(base, rel, opts)) return r->string();
  return normalized;
}

inline int _access(const char* path, int mode) {
  if (!path) { errno = ENOENT; return -1; }
  // mode 0 (existence), 2 (write), 4 (read), 6 (read/write) - map to POSIX access checks.
  const std::string resolved = fl_port_io_resolve_existing(path);
  int amode = F_OK;
  if (mode & 4) amode |= R_OK;
  if (mode & 2) amode |= W_OK;
  if (mode & 1) amode |= X_OK;
  if (mode == 0) amode = F_OK;
  return ::access(resolved.c_str(), amode);
}
inline int access(const char* path, int mode) { return _access(path, mode); }

inline int _mkdir(const char* path) {
  if (!path || !*path) { errno = EINVAL; return -1; }
  const std::string normalized = fl_port::normalize_path_separators(path);
  std::error_code ec;
  // Try to resolve existing parent case-insensitively for engine-created dirs under existing trees.
  std::filesystem::path p(normalized);
  std::filesystem::path target = p;
  if (!p.is_absolute()) {
    auto parent = p.parent_path();
    auto base = std::filesystem::current_path();
    if (!parent.empty()) {
      if (auto rp = fl_port::resolve_path_case_insensitive(base, parent.string(), fl_port::ResolveOptions{false})) {
        target = *rp / p.filename();
      } else {
        target = base / p;
      }
    } else {
      target = base / p;
    }
  }
  if (::mkdir(target.string().c_str(), 0777) == 0) return 0;
  if (errno == EEXIST) {
    ec.clear();
    if (std::filesystem::is_directory(target, ec)) return 0;
  }
  return -1;
}
