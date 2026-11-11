// Guarded Linux harness TU: compile upstream ReadFile source in bring-up shim context.
// Integration/compilation wrapper only; runtime behavior remains partial/stubbed where
// Win32 APIs, mappings, and module semantics are not fully implemented.

#if !defined(_WIN32)
#include "linux_port/windows.h"
#include "linux_port/platform.h"
#include "linux_port/module_loader.h"
#include "linux_port/path_util.h"

#include <cstdio>
#include <filesystem>
#include <string>

namespace {
inline FILE* fl_port_fopen_resolved(const char* path, const char* mode) {
  if (!path || !mode) return nullptr;
  const std::string normalized = fl_port::normalize_path_separators(path);
  const std::filesystem::path base = std::filesystem::current_path();
  fl_port::ResolveOptions opts;
  opts.allow_missing_tail = (mode[0] == 'w' || mode[0] == 'a');
  if (auto resolved = fl_port::resolve_path_case_insensitive(base, normalized, opts)) {
    return std::fopen(resolved->string().c_str(), mode);
  }
  if (auto r2 = fl_port::resolve_path_case_insensitive(base, normalized, {})) {
    return std::fopen(r2->string().c_str(), mode);
  }
  return std::fopen(normalized.c_str(), mode);
}
}
#define fopen fl_port_fopen_resolved

// Neutralize Windows-specific directives/attributes not meaningful in this TU.
#ifndef __declspec
#define __declspec(x)
#endif
#define _declspec(x)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"

// Rename DllMain to avoid collision in harness linkage.
#define DllMain DllMain_readfile_upstream
#ifndef WINAPI
#define WINAPI
#endif

// Provide Linux-compatible comparator used by upstream ReadFile.cpp.
int CallDacomStricmp(const char* Left, const char* Right) {
  return _stricmp(Left, Right);
}

#endif

#include "../../src/ReadFile/ReadFile.cpp"

#if !defined(_WIN32)
// #1.2 integration hook: explicit module registration/init for ReadFile in harness context.
// Upstream DllMain was renamed to DllMain_readfile_upstream by this wrapper TU.
extern "C" int fl_upstream_readfile_integration_register() {
  // Simulate process attach so factories/components register with DACOM manager.
  const BOOL ok = DllMain_readfile_upstream((HINSTANCE)nullptr, (DWORD)1 /* DLL_PROCESS_ATTACH */, (LPVOID)nullptr);
  return ok ? 1 : 0;
}
#pragma GCC diagnostic pop
#endif
