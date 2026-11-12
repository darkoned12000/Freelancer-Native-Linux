#include "linux_port/platform.h"
#include "linux_port/path_util.h"

#include <dacom.h>
#include <readfile.h>

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#if defined(__i386__) || defined(__i386) || defined(i386) || defined(_M_IX86)
extern "C" void fl_upstream_dacom_install_fdump_bridge();
extern "C" void fl_upstream_dacom_register_builtin_components();
extern "C" int  fl_upstream_readfile_integration_register();
extern "C" int  fl_upstream_x86math_integration_register();
#endif

namespace {

std::filesystem::path resolve_exe_path(char** argv) {
#if defined(__linux__)
  {
    std::error_code ec;
    auto p = std::filesystem::read_symlink("/proc/self/exe", ec);
    if (!ec && !p.empty()) return p;
  }
#endif
  if (argv && argv[0]) {
    std::error_code ec;
    auto p = std::filesystem::weakly_canonical(std::filesystem::path(argv[0]), ec);
    if (!ec && !p.empty()) return p;
  }
  return {};
}

bool try_set_bin_cwd(const std::filesystem::path& start_cwd, const std::filesystem::path& exe_path) {
  const auto has_dacom = [](const std::filesystem::path& dir) {
    std::error_code ec;
    return !dir.empty() && std::filesystem::exists(dir / "DACOM.INI", ec) && !ec;
  };

  std::vector<std::filesystem::path> candidates;
  if (!start_cwd.empty()) {
    candidates.push_back(start_cwd / "bin");
    candidates.push_back(start_cwd);
  }
  if (!exe_path.empty()) {
    const auto ed = exe_path.parent_path();
    candidates.push_back(ed);
    candidates.push_back(ed / "bin");
    candidates.push_back(ed / ".." / "bin");
    candidates.push_back(ed / ".." / ".." / "bin");
  }

  for (const auto& cand : candidates) {
    std::error_code ec;
    auto norm = std::filesystem::weakly_canonical(cand, ec);
    const auto& dir = (!ec && !norm.empty()) ? norm : cand;
    if (!has_dacom(dir)) continue;
    ec.clear();
    std::filesystem::current_path(dir, ec);
    if (!ec) {
      fl_port::log_message(fl_port::LogLevel::Info,
                           std::string("set cwd to bin context: ") + dir.string());
      return true;
    }
  }
  return false;
}

} // namespace

int main(int argc, char** argv) {
  (void)argc;
  fl_port::log_message(fl_port::LogLevel::Info,
                       "freelancer_linux_client (M8.1): minimal linkable client skeleton starting");

#if defined(__i386__) || defined(__i386) || defined(i386) || defined(_M_IX86)
  // FDUMP/log bridge (DACOM diagnostics compatibility)
  fl_upstream_dacom_install_fdump_bridge();
  fl_port::log_message(fl_port::LogLevel::Info, "FDUMP bridge installed");

  // Basic path/cwd resolution for process start (best-effort; prefer bin/ context)
  std::error_code ec;
  const auto start_cwd = std::filesystem::current_path(ec);
  if (ec) {
    fl_port::log_message(fl_port::LogLevel::Warning,
                         "failed to query startup cwd (non-fatal)");
  } else {
    fl_port::log_message(fl_port::LogLevel::Info,
                         std::string("startup cwd=") + start_cwd.string());
  }

  const auto exe_path = resolve_exe_path(argv);
  if (!exe_path.empty()) {
    fl_port::log_message(fl_port::LogLevel::Info,
                         std::string("exe path=") + exe_path.string());
  }
  if (!try_set_bin_cwd(ec ? std::filesystem::path{} : start_cwd, exe_path)) {
    fl_port::log_message(fl_port::LogLevel::Info,
                         "no bin/DACOM.INI context found from startup cwd/exe path; continuing with current cwd");
  }

  // Minimal process start: DACOM acquisition and explicit registration hooks
  ICOManager* mgr = DACOM_Acquire();
  if (!mgr) {
    fl_port::log_message(fl_port::LogLevel::Error, "DACOM_Acquire() failed");
    std::cerr << "[FAIL] DACOM_Acquire() failed\n";
    return 1;
  }
  fl_port::log_message(fl_port::LogLevel::Info, "DACOM_Acquire() ok");

  // Explicit registration in dependency order (DllMain-free bring-up model)
  fl_upstream_dacom_register_builtin_components();
  fl_port::log_message(fl_port::LogLevel::Info, "DACOM built-in components registered");

  const int rf_ok = fl_upstream_readfile_integration_register();
  fl_port::log_message(rf_ok ? fl_port::LogLevel::Info : fl_port::LogLevel::Error,
                       rf_ok ? "ReadFile explicitly registered" : "ReadFile registration failed");

  const int math_ok = fl_upstream_x86math_integration_register();
  fl_port::log_message(math_ok ? fl_port::LogLevel::Info : fl_port::LogLevel::Error,
                       math_ok ? "x86math explicitly registered"
                              : "x86math explicit registration failed");

  // Smoke: core manager reachable after registrations
  (void)mgr;
  std::cout << "freelancer_linux_client (M8.1): reached entry point; DACOM core initialized (skeleton)\n";
  std::cout << "Status: linkable skeleton ok; upstream client WinMain/engine loop not wired yet (M8.2+)\n";
  if (!rf_ok) return 2;
  if (!math_ok) return 3;
  return 0;
#else
  fl_port::log_message(fl_port::LogLevel::Warning,
                       "M8.1 upstream DACOM skeleton requires a 32-bit (i386) build; running stub path");
  std::cout << "freelancer_linux_client: M8.1 skeleton requires linux-x86 build for upstream DACOM acquisition\n";
  return 0;
#endif
}
