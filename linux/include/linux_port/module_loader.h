#pragma once

// Module loading abstraction skeleton for DACOM/Linux bring-up (#1).
// Early bring-up may prefer static-linked modules or explicit init functions;
// dynamic loading via dlopen/dlsym is the eventual compatibility path.

#include "linux_port/win32_compat.h"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace fl_port {

struct ModuleHandle;
void unload_module(ModuleHandle& module);

struct ModuleHandle {
  void* native = nullptr; // dlopen handle on Linux
  std::filesystem::path path;

  ModuleHandle() = default;
  ~ModuleHandle() = default;
  ModuleHandle(const ModuleHandle&) = delete;
  ModuleHandle& operator=(const ModuleHandle&) = delete;
  ModuleHandle(ModuleHandle&& o) noexcept : native(o.native), path(std::move(o.path)) {
    o.native = nullptr;
  }
  ModuleHandle& operator=(ModuleHandle&& o) noexcept {
    if (this != &o) {
      if (native) {
        unload_module(*this);
      }
      native = o.native;
      path = std::move(o.path);
      o.native = nullptr;
    }
    return *this;
  }

  explicit operator bool() const { return native != nullptr; }
};

// Resolve/map a requested module name (e.g. "ReadFile.dll", "ReadFile") to a
// Linux candidate path/name according to policy (TBD: .so mapping, search dirs,
// case-insensitive resolution, static-module registry).
std::optional<std::filesystem::path> resolve_module_path(
    std::string_view requested_name,
    const std::filesystem::path& search_dir);

// Load a module from an explicit path. Returns nullopt on failure.
std::optional<ModuleHandle> load_module(const std::filesystem::path& path);

// Lookup a symbol/export from a loaded module.
void* get_module_symbol(const ModuleHandle& module, const char* symbol_name);

// Unload a module. Safe to call with empty handle.
void unload_module(ModuleHandle& module);

// Return last loader error message (thread-local or global per implementation; TBD).
std::string module_loader_last_error();

} // namespace fl_port
