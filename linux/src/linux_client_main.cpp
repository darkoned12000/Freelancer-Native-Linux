#include "linux_port/platform.h"
#include "linux_port/path_util.h"
#include "linux_port/windows.h"

#include <dacom.h>
#include <readfile.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits.h>
#include <string>
#include <vector>

#include <unistd.h>

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
    char buf[PATH_MAX];
    const ssize_t n = ::readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (n > 0) {
      buf[n] = '\0';
      return std::filesystem::path(std::string(buf, static_cast<size_t>(n)));
    }
  }
#endif
  if (argv && argv[0]) {
    std::error_code ec;
    const std::filesystem::path arg_path(argv[0]);
    auto p = std::filesystem::weakly_canonical(arg_path, ec);
    if (!ec && !p.empty()) return p;
  }
  return std::filesystem::path{};
}

std::string trim_copy(std::string s) {
  size_t a = 0, b = s.size();
  while (a < b && (s[a] == ' ' || s[a] == '\t' || s[a] == '\r' || s[a] == '\n')) ++a;
  while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t' || s[b - 1] == '\r' || s[b - 1] == '\n')) --b;
  return s.substr(a, b - a);
}

std::string lower_ascii(std::string s) {
  for (char& c : s) if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
  return s;
}

std::string get_ini_first_value(const std::filesystem::path& ini_path,
                                const std::string& section_want,
                                const std::string& key_want) {
  std::ifstream in(ini_path);
  if (!in) return {};
  std::string line;
  std::string current;
  const std::string sec = lower_ascii(section_want);
  while (std::getline(in, line)) {
    auto sc = line.find(';');
    std::string s = trim_copy(sc == std::string::npos ? line : line.substr(0, sc));
    if (s.empty()) continue;
    if (s.front() == '[' && s.back() == ']') {
      current = lower_ascii(trim_copy(s.substr(1, s.size() - 2)));
      continue;
    }
    if (current != sec) continue;
    auto eq = s.find('=');
    if (eq == std::string::npos) continue;
    std::string k = trim_copy(s.substr(0, eq));
    if (lower_ascii(k) == lower_ascii(key_want)) return trim_copy(s.substr(eq + 1));
  }
  return {};
}

std::filesystem::path make_dacom_init_config_without_libraries(const std::filesystem::path& src_ini) {
  // Source-only checkout: external runtime module binaries in [Libraries] may be unavailable.
  // Use effective config with [Libraries] entries removed for explicit-registration bring-up.
  std::ifstream in(src_ini);
  if (!in) return {};
  const auto out_path = std::filesystem::temp_directory_path() /
      ("fl_m82_dacom_init_" + std::to_string(::getpid()) + ".ini");
  std::ofstream out(out_path, std::ios::trunc);
  if (!out) return {};
  std::string line;
  bool in_libraries = false;
  while (std::getline(in, line)) {
    std::string trimmed = trim_copy(line);
    if (!trimmed.empty() && trimmed.front() == '[' && trimmed.back() == ']') {
      in_libraries = (lower_ascii(trimmed) == "[libraries]");
      out << line << "\n";
      continue;
    }
    if (in_libraries) continue;
    out << line << "\n";
  }
  if (!out.good()) return {};
  return out_path;
}

bool read_text_via_win32(const std::filesystem::path& p, std::string& out, size_t max_bytes = 512) {
  out.clear();
  HANDLE h = CreateFileA(p.string().c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                         OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (h == INVALID_HANDLE_VALUE) return false;
  std::vector<char> buf(max_bytes);
  DWORD got = 0;
  const bool ok = ReadFile(h, buf.data(), (DWORD)buf.size(), &got, nullptr) != 0;
  CloseHandle(h);
  if (!ok) return false;
  out.assign(buf.data(), buf.data() + got);
  return true;
}

bool try_set_bin_cwd(const std::filesystem::path& start_cwd, const std::filesystem::path& exe_path,
                     std::filesystem::path& out_bin_dir) {
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
      out_bin_dir = dir;
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
                       "freelancer_linux_client (M8.2): core engine initialization starting");

#if defined(__i386__) || defined(__i386) || defined(i386) || defined(_M_IX86)
  fl_upstream_dacom_install_fdump_bridge();
  fl_port::log_message(fl_port::LogLevel::Info, "FDUMP bridge installed");

  std::error_code ec;
  const auto start_cwd = std::filesystem::current_path(ec);
  if (ec) {
    fl_port::log_message(fl_port::LogLevel::Warning, "failed to query startup cwd (non-fatal)");
  } else {
    fl_port::log_message(fl_port::LogLevel::Info, std::string("startup cwd=") + start_cwd.string());
  }
  const auto exe_path = resolve_exe_path(argv);
  if (!exe_path.empty()) {
    fl_port::log_message(fl_port::LogLevel::Info, std::string("exe path=") + exe_path.string());
  }

  std::filesystem::path bin_dir;
  if (!try_set_bin_cwd(ec ? std::filesystem::path{} : start_cwd, exe_path, bin_dir)) {
    fl_port::log_message(fl_port::LogLevel::Warning,
                         "no bin/DACOM.INI context found from startup cwd/exe path; continuing with current cwd");
    bin_dir = ec ? std::filesystem::path{} : start_cwd;
  }

  // --- Early/core init sequence (client-oriented bring-up): DACOM + config + DATA ---
  ICOManager* mgr = DACOM_Acquire();
  if (!mgr) {
    fl_port::log_message(fl_port::LogLevel::Error, "DACOM_Acquire() failed");
    std::cerr << "[FAIL] DACOM_Acquire() failed\n";
    return 1;
  }
  fl_port::log_message(fl_port::LogLevel::Info, "DACOM_Acquire() ok");

  fl_upstream_dacom_register_builtin_components();
  fl_port::log_message(fl_port::LogLevel::Info, "DACOM built-in components registered");

  const auto dacom_ini_resolved = fl_port::resolve_path_case_insensitive(bin_dir.empty() ? std::filesystem::current_path() : bin_dir, "DACOM.INI");
  const auto freelancer_ini_resolved = fl_port::resolve_path_case_insensitive(bin_dir.empty() ? std::filesystem::current_path() : bin_dir, "freelancer.ini");
  if (dacom_ini_resolved) fl_port::log_message(fl_port::LogLevel::Info, "resolved DACOM.INI: " + dacom_ini_resolved->string());
  else fl_port::log_message(fl_port::LogLevel::Error, "failed to resolve DACOM.INI");
  if (freelancer_ini_resolved) fl_port::log_message(fl_port::LogLevel::Info, "resolved freelancer.ini: " + freelancer_ini_resolved->string());
  else fl_port::log_message(fl_port::LogLevel::Error, "failed to resolve freelancer.ini");

  if (mgr && dacom_ini_resolved) {
    const auto effective_dacom_ini = make_dacom_init_config_without_libraries(*dacom_ini_resolved);
    if (!effective_dacom_ini.empty()) {
      const GENRESULT gr = mgr->SetINIConfig(effective_dacom_ini.string().c_str(), 0);
      fl_port::log_message(gr == GR_OK ? fl_port::LogLevel::Info : fl_port::LogLevel::Warning,
                           std::string("DACOM SetINIConfig (effective config) gr=") + std::to_string((int)gr));
      std::error_code ec_rm;
      std::filesystem::remove(effective_dacom_ini, ec_rm);
    } else {
      fl_port::log_message(fl_port::LogLevel::Error, "failed to prepare effective DACOM init config");
    }
  }

  const int rf_ok = fl_upstream_readfile_integration_register();
  fl_port::log_message(rf_ok ? fl_port::LogLevel::Info : fl_port::LogLevel::Error,
                       rf_ok ? "ReadFile explicitly registered" : "ReadFile registration failed");

  const int math_ok = fl_upstream_x86math_integration_register();
  fl_port::log_message(math_ok ? fl_port::LogLevel::Info : fl_port::LogLevel::Error,
                       math_ok ? "x86math explicitly registered" : "x86math explicit registration failed");

  if (!rf_ok) return 2;
  if (!math_ok) return 3;
  if (!dacom_ini_resolved || !freelancer_ini_resolved) return 4;

  // Resolve freelancer.ini [Freelancer].data path and DATA root
  std::filesystem::path data_root;
  const std::string data_path_val = get_ini_first_value(*freelancer_ini_resolved, "Freelancer", "data path");
  if (data_path_val.empty()) {
    fl_port::log_message(fl_port::LogLevel::Error, "freelancer.ini missing [Freelancer].data path");
    return 5;
  }
  {
    const auto base = freelancer_ini_resolved->parent_path();
    auto dr = fl_port::resolve_path_case_insensitive(base, data_path_val);
    if (!dr) dr = fl_port::resolve_path_case_insensitive(base, std::string(data_path_val) + "\\");
    if (dr) {
      data_root = *dr;
      fl_port::log_message(fl_port::LogLevel::Info,
                           "resolved [Freelancer].data path: " + data_path_val + " -> " + data_root.string());
    } else {
      fl_port::log_message(fl_port::LogLevel::Error,
                           "failed to resolve [Freelancer].data path: " + data_path_val);
      return 6;
    }
  }

  // Core component creation proof (FileSystem/DOS over DATA root) + IFileSystem exercise
  bool fs_ok = false;
  IFileSystem* data_fs = nullptr;
  {
    void* inst = nullptr;
    FFileSystemDesc fs{};
    static_assert(offsetof(FFileSystemDesc, Field30) == 0x30, "DOS/UTF desc prefix is 0x34");
    fs.Size = 0x34; // DOS/UTF descriptor size (prefix) for FileSystem/DOS instance
    fs.ComponentName = "FileSystem";
    fs.Implementation = "DOS";
    const std::string data_path_str = data_root.string();
    fs.Path = data_path_str.c_str();
    fs.Access = GENERIC_READ;
    fs.ShareMode = FILE_SHARE_READ;
    fs.Creation = OPEN_EXISTING;
    fs.Flags = 0;
    fs.Extra = 0;
    fs.Field30 = 0xFFFFFFFFu;
    const GENRESULT gr = mgr->CreateInstance((AGGDESC*)&fs, &inst);
    if (gr == GR_OK && inst) {
      data_fs = reinterpret_cast<IFileSystem*>(inst);
      fs_ok = (data_fs != nullptr);
      if (fs_ok) fl_port::log_message(fl_port::LogLevel::Info, "CreateInstance(FileSystem/DOS) ok");
      else {
        fl_port::log_message(fl_port::LogLevel::Error, "CreateInstance(FileSystem/DOS) returned null IFileSystem");
        if (auto* comp = reinterpret_cast<IDAComponent*>(inst)) comp->Release();
      }
    } else {
      fl_port::log_message(fl_port::LogLevel::Error,
                           std::string("CreateInstance(FileSystem/DOS) failed gr=") + std::to_string((int)gr));
    }
  }
  if (!fs_ok || !data_fs) return 7;

  // Exercise IFileSystem interface directly (path/search behavior in ReadFile component)
  {
    char cur[260] = {};
    const int cur_ok = data_fs->GetCurrentPath(sizeof(cur), cur);
    fl_port::log_message(cur_ok ? fl_port::LogLevel::Info : fl_port::LogLevel::Warning,
                         std::string("IFileSystem GetCurrentPath: ") + (cur_ok ? cur : "(failed)"));
    const char* probe_paths[] = {
      "solar\\solararch.ini",
      "UNIVERSE\\universe.ini",
      "EQUIPMENT\\select_equip.ini",
      "FONTS\\fonts.ini",
      "InitialWorld.ini",
    };
    for (const char* p : probe_paths) {
      const int attr = data_fs->GetPathAttributes(p);
      fl_port::log_message(attr != 0 ? fl_port::LogLevel::Info : fl_port::LogLevel::Warning,
                           std::string("IFileSystem GetPathAttributes(") + p + ") => " + std::to_string(attr));
    }
  }

  // Config/DATA loading checks via Linux-compatible path resolution and Win32 file APIs
  auto check_read = [&](const std::filesystem::path& base, const char* rel, const char* label, size_t nbytes) -> bool {
    auto resolved = fl_port::resolve_path_case_insensitive(base, rel);
    if (!resolved) {
      fl_port::log_message(fl_port::LogLevel::Error, std::string("resolve failed: ") + label + " (" + rel + ")");
      return false;
    }
    std::string text;
    if (!read_text_via_win32(*resolved, text, nbytes)) {
      fl_port::log_message(fl_port::LogLevel::Error, std::string("read failed: ") + label + " -> " + resolved->string());
      return false;
    }
    fl_port::log_message(fl_port::LogLevel::Info, std::string("loaded: ") + label + " (" + std::to_string(text.size()) + " bytes)");
    return true;
  };

  bool data_ok = true;
  data_ok = check_read(freelancer_ini_resolved->parent_path(), "freelancer.ini", "freelancer.ini", 256) && data_ok;
  data_ok = check_read(freelancer_ini_resolved->parent_path(), "DACOM.INI", "DACOM.INI", 256) && data_ok;
  // Key DATA files (case-insensitive/backslash-safe resolution on Linux)
  data_ok = check_read(data_root, "solar\\solararch.ini", "DATA solararch.ini", 256) && data_ok;
  data_ok = check_read(data_root, "UNIVERSE\\universe.ini", "DATA universe.ini", 256) && data_ok;
  data_ok = check_read(data_root, "EQUIPMENT\\select_equip.ini", "DATA select_equip.ini", 256) && data_ok;
  data_ok = check_read(data_root, "FONTS\\fonts.ini", "DATA fonts.ini", 256) && data_ok;

  // BINI/specialized content presence check (resolved path) - full BINI decode is deeper client-path work
  {
    auto bini = fl_port::resolve_path_case_insensitive(data_root, "INITIALWORLD.INI");
    if (bini) fl_port::log_message(fl_port::LogLevel::Info, "resolved DATA INITIALWORLD.INI: " + bini->string());
    else fl_port::log_message(fl_port::LogLevel::Warning, "DATA INITIALWORLD.INI not resolved (non-fatal in source-only checkout)");
  }

  if (data_fs) {
    data_fs->Release();
    data_fs = nullptr;
  }

  if (!data_ok) {
    std::cerr << "[FAIL] M8.2 core config/DATA loading checks failed\n";
    return 8;
  }

  std::cout << "freelancer_linux_client (M8.2 bring-up): DACOM component system initialized; core components created; config/DATA loading checks passed\n";
  std::cout << "Status: upstream client/DALib/Common TU linkage pending before full M8.2 acceptance (window/render/input/engine loop pending M8.3+)\n";
  return 0;
#else
  fl_port::log_message(fl_port::LogLevel::Warning,
                       "M8.2 core engine initialization requires a 32-bit (i386) build; running stub path");
  std::cout << "freelancer_linux_client: M8.2 requires linux-x86 build for upstream DACOM acquisition\n";
  return 0;
#endif
}
