#include "linux_port/module_loader.h"

#include "linux_port/path_util.h"

#include <cctype>

#if !defined(_WIN32)
#include <dlfcn.h>
#endif

namespace fl_port {
namespace {
#if !defined(_WIN32)
thread_local std::string g_last_loader_error;
#endif

std::string ascii_lower_copy(std::string s) {
  for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return s;
}
} // namespace

std::optional<std::filesystem::path> resolve_module_path(
    std::string_view requested_name,
    const std::filesystem::path& search_dir) {
  // Skeleton policy:
  // 1) Try requested name as-is (normalized/case-insensitively resolved).
  // 2) Try Linux-mapped candidates:
  //    - if ".dll": <dir>/lib<stem>.so
  //    - if no extension: <dir>/<name>.so and <dir>/lib<name>.so
  // Actual DACOM-compatible search semantics (DllPath, order, registration side
  // effects) will be implemented during module bring-up.
  if (auto direct = resolve_path_case_insensitive(search_dir, requested_name)) {
    return direct;
  }

  const std::filesystem::path req(requested_name);
  const std::filesystem::path parent = req.parent_path();
  const std::string ext = ascii_lower_copy(req.extension().string());
  const std::string stem = req.stem().string();

  auto try_candidate = [&](const std::filesystem::path& candidate) -> std::optional<std::filesystem::path> {
    if (auto r = resolve_path_case_insensitive(search_dir, candidate.string())) return r;
    return std::nullopt;
  };

  if (ext == ".dll") {
    if (!stem.empty()) {
      const auto c = parent / ("lib" + stem + ".so");
      if (auto r = try_candidate(c)) return r;
    }
  } else if (ext.empty()) {
    const std::string name = req.filename().string();
    if (!name.empty()) {
      if (auto r = try_candidate(parent / (name + ".so"))) return r;
      if (auto r = try_candidate(parent / ("lib" + name + ".so"))) return r;
    }
  } else if (ext == ".so") {
    // Some callers may pass a plain .so name; if a "lib" prefixed variant exists
    // in the search dir, accept it.
    if (!stem.empty() && stem.rfind("lib", 0) != 0) {
      if (auto r = try_candidate(parent / ("lib" + req.filename().string()))) return r;
    }
  }

  return std::nullopt;
}

std::optional<ModuleHandle> load_module(const std::filesystem::path& path) {
#if defined(_WIN32)
  (void)path;
  return std::nullopt;
#else
  dlerror();
  void* handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
  if (!handle) {
    const char* err = dlerror();
    g_last_loader_error = err ? std::string(err) : "unknown dlopen failure";
    return std::nullopt;
  }
  g_last_loader_error.clear();
  ModuleHandle out;
  out.native = handle;
  out.path = path;
  return out;
#endif
}

void* get_module_symbol(const ModuleHandle& module, const char* symbol_name) {
#if defined(_WIN32)
  (void)module; (void)symbol_name;
  return nullptr;
#else
  if (!module.native || !symbol_name) return nullptr;
  dlerror();
  void* sym = dlsym(module.native, symbol_name);
  const char* err = dlerror();
  if (err) g_last_loader_error = err;
  return sym;
#endif
}

void unload_module(ModuleHandle& module) {
#if defined(_WIN32)
  module.native = nullptr;
  module.path.clear();
#else
  if (module.native) {
    dlclose(module.native);
  }
  module.native = nullptr;
  module.path.clear();
#endif
}

std::string module_loader_last_error() {
#if defined(_WIN32)
  return "module loading not implemented for Windows in linux_port skeleton";
#else
  return g_last_loader_error;
#endif
}

} // namespace fl_port
