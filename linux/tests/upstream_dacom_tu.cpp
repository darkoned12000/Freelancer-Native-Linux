// Guarded Linux harness TU: compile upstream DACOM source in bring-up shim context.
// This is an integration/compilation wrapper, not a drop-in runtime replacement for
// the original Windows DLL build (resources, full Win32 behavior, and all original
// side effects are not reproduced).

#if !defined(_WIN32)
#include "linux_port/windows.h"
#include "linux_port/platform.h"
#include "linux_port/module_loader.h"
#include "linux_port/path_util.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <unistd.h>

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
#pragma GCC diagnostic ignored "-Wformat"
#pragma GCC diagnostic ignored "-Wwrite-strings"

// Let upstream DACOM define/use its own FDUMP symbol in this TU.
#ifdef FDUMP
#undef FDUMP
#endif
#ifdef FDUMP_ERROR
#undef FDUMP_ERROR
#endif
#ifdef FDUMP_WARNING
#undef FDUMP_WARNING
#endif

// Rename DllMain so the harness can provide its own entrypoint if linked as exe.
#define DllMain DllMain_dacom_upstream
#ifndef APIENTRY
#define APIENTRY
#endif

#endif

#include "../../src/dacom/dacom.cpp"

#if !defined(_WIN32)
namespace {
void __cdecl fl_port_fdump_bridge(int flags, const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  const std::string msg = ::fl_port::format_printf(fmt, args);
  va_end(args);
  const unsigned int code = static_cast<unsigned int>(flags);
  ::fl_port::fdump(static_cast<std::uint32_t>(code), "%s", msg.c_str());
}
}

extern "C" void fl_upstream_dacom_install_fdump_bridge() {
  if (!FDUMP) {
    FDUMP = &fl_port_fdump_bridge;
  }
}

// #1.2 integration hooks: exercise upstream DACOM APIs compiled in this TU.
// These are harness-only helpers (not part of upstream ABI).
extern "C" int fl_upstream_dacom_integration_smoke(const char* ini_path) {
  fl_upstream_dacom_install_fdump_bridge();

  ICOManager* mgr = DACOM_Acquire();
  if (!mgr) return 0;

  // Basic manager/config surface (SetINIConfig may partially succeed/fail depending on file IO shims).
  // Source-only checkouts do not ship runtime DLL modules listed in [Libraries]. For this smoke harness
  // we explicitly register needed components later, so use a filtered config copy without [Libraries]
  // entries to avoid incorrect auto-load attempts (and resulting load errors).
  std::string effective_ini = ini_path ? ini_path : "bin/DACOM.INI";
  std::string temp_ini_path;
  {
    std::ifstream in(effective_ini);
    if (in) {
      temp_ini_path = (std::filesystem::temp_directory_path() /
                       ("fl_dacom_smoke_no_libraries_" + std::to_string(::getpid()) + ".ini")).string();
      std::ofstream out(temp_ini_path, std::ios::trunc);
      if (out) {
        std::string line;
        bool in_libraries = false;
        while (std::getline(in, line)) {
          std::string trimmed = line;
          while (!trimmed.empty() && (trimmed.front() == ' ' || trimmed.front() == '\t')) trimmed.erase(trimmed.begin());
          while (!trimmed.empty() && (trimmed.back() == ' ' || trimmed.back() == '\t' || trimmed.back() == '\r')) trimmed.pop_back();
          if (!trimmed.empty() && trimmed.front() == '[' && trimmed.back() == ']') {
            in_libraries = (trimmed == "[Libraries]" || trimmed == "[libraries]" || trimmed == "[LIBRARIES]");
            out << line << "\n";
            continue;
          }
          if (in_libraries) {
            continue;
          }
          out << line << "\n";
        }
        if (out.good()) effective_ini = temp_ini_path;
      }
    }
  }

  (void)mgr->SetINIConfig(effective_ini.c_str(), 0);

  if (!temp_ini_path.empty()) {
    std::error_code ec;
    std::filesystem::remove(temp_ini_path, ec);
  }

  // Exercise component registration/creation path compiled from upstream source.
  AGGDESC desc{};
  desc.size = sizeof(AGGDESC);
  desc.interface_name = "RegExp";
  void* inst = nullptr;
  const GENRESULT gr = mgr->CreateInstance(&desc, &inst);
  if (gr == GR_OK && inst) {
    auto* re = reinterpret_cast<IRegExp*>(inst);
    if (re) {
      (void)re->CompilePattern("abc");
      (void)re->Search("xxabcxx");
      re->Release();
    }
  }

  // Avoid forcing full process-global shutdown in smoke harness; upstream manager
  // lifetime/registration state is process-scoped and still under active bring-up.
  return 1;
}

extern "C" void fl_upstream_dacom_register_builtin_components() {
  fl_upstream_dacom_install_fdump_bridge();
  // DllMain was renamed in this wrapper TU; explicitly register built-ins that
  // upstream DACOM would register during DLL_PROCESS_ATTACH.
  RegisterRegExpComponent();
}
#pragma GCC diagnostic pop
#endif
