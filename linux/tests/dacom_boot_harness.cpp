#include "linux_port/module_loader.h"
#include "linux_port/path_util.h"
#include "linux_port/platform.h"
#include "linux_port/win32_compat.h"
#include "linux_port/win32_file.h"
#include "linux_port/win32_threads.h"
#include "linux_port/win32_time.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

// #1.2 deepened DACOM/ReadFile boot harness spike:
// - validates shim+path+module primitives against real boot config/data
// - models Linux module resolution/init planning from DACOM.INI [Libraries]
// - summarizes key freelancer.ini boot dependencies (data path/resources/startup/data)
// This does not execute upstream DACOM/ReadFile code; it prepares/validates the
// contracts and filesystem/module assumptions needed for guarded port/harness TUs.

namespace {

std::string trim(const std::string& s) {
  size_t a = 0, b = s.size();
  while (a < b && (s[a] == ' ' || s[a] == '\t' || s[a] == '\r' || s[a] == '\n')) ++a;
  while (b > a && (s[b-1] == ' ' || s[b-1] == '\t' || s[b-1] == '\r' || s[b-1] == '\n')) --b;
  return s.substr(a, b - a);
}

std::string to_lower_ascii_copy(std::string s) {
  for (char& c : s) c = (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
  return s;
}

std::map<std::string, std::vector<std::pair<std::string,std::string>>> parse_ini_simple(const std::filesystem::path& path) {
  std::ifstream in(path);
  std::map<std::string, std::vector<std::pair<std::string,std::string>>> out;
  std::string current_key;
  std::string raw;
  while (std::getline(in, raw)) {
    auto c = raw.find(';');
    std::string line = trim(c == std::string::npos ? raw : raw.substr(0, c));
    if (line.empty()) continue;
    if (line.front() == '[' && line.back() == ']') {
      current_key = to_lower_ascii_copy(trim(line.substr(1, line.size() - 2)));
      (void)out[current_key];
      continue;
    }
    if (current_key.empty()) continue;
    auto eq = line.find('=');
    if (eq == std::string::npos) out[current_key].push_back({line, ""});
    else out[current_key].push_back({trim(line.substr(0, eq)), trim(line.substr(eq + 1))});
  }
  return out;
}

std::string get_first(const std::map<std::string, std::vector<std::pair<std::string,std::string>>>& ini,
                      const std::string& sec, const std::string& key) {
  auto it = ini.find(to_lower_ascii_copy(sec));
  if (it == ini.end()) return {};
  for (const auto& kv : it->second) if (kv.first == key) return kv.second;
  return {};
}

} // namespace

int main(int argc, char** argv) {
  const std::filesystem::path repo = (argc > 1) ? std::filesystem::path(argv[1]) : std::filesystem::current_path();
  const std::filesystem::path bin = repo / "bin";
  const std::filesystem::path dacom_ini = bin / "DACOM.INI";
  const std::filesystem::path fl_ini = bin / "freelancer.ini";

  fl_port::log_message(fl_port::LogLevel::Info, "dacom_boot_harness starting");
  std::cout << "repo: " << repo.string() << "\n";

  if (!std::filesystem::exists(dacom_ini)) {
    std::cerr << "missing: " << dacom_ini << "\n";
    return 1;
  }
  auto dacom = parse_ini_simple(dacom_ini);
  auto libs_it = dacom.find("libraries");
  std::cout << "[DACOM] Libraries load/init planning (explicit init strategy placeholder):\n";
  if (libs_it != dacom.end()) {
    int i = 0;
    for (const auto& e : libs_it->second) {
      if (!e.second.empty()) continue;
      ++i;
      auto resolved = fl_port::resolve_module_path(e.first, bin);
      std::cout << "  " << i << ". " << e.first
                << " | resolve=" << (resolved ? resolved->string() : "<missing>")
                << " | init=TODO(explicit/static registration, preserve order)\n";
    }
  }
  auto mgsdb = get_first(dacom, "renderpipeline", "MGSDB");
  if (!mgsdb.empty()) {
    auto r = fl_port::resolve_path_case_insensitive(bin, mgsdb);
    std::cout << "MGSDB: " << mgsdb << " => " << (r ? r->string() : "<not found>") << "\n";
  }

  if (std::filesystem::exists(fl_ini)) {
    auto fl = parse_ini_simple(fl_ini);
    const std::string data_path = get_first(fl, "freelancer", "data path");
    auto data_resolved = fl_port::resolve_path_case_insensitive(bin, data_path);
    std::cout << "freelancer.ini data path: " << data_path
              << " => " << (data_resolved ? data_resolved->string() : "<not found>") << "\n";

    if (data_resolved) {
      const char* samples[] = {"solar\\solararch.ini", "universe\\universe.ini", "fonts\\fonts.ini"};
      for (const char* s : samples) {
        auto r = fl_port::resolve_path_case_insensitive(*data_resolved, s);
        std::cout << "  data sample: " << s << " => " << (r ? r->string() : "<missing>") << "\n";
      }
    }
    auto res_it = fl.find("resources");
    if (res_it != fl.end()) {
      int missing = 0;
      for (const auto& kv : res_it->second) if (kv.first == "DLL") {
        auto r = fl_port::resolve_module_path(kv.second, bin);
        if (!r) ++missing;
      }
      std::cout << "Resources DLLs unresolved in runtime dir: " << missing << "\n";
    }
  }

  // Sync/time shim smoke in harness context
  CRITICAL_SECTION cs;
  InitializeCriticalSection(&cs);
  EnterCriticalSection(&cs);
  LeaveCriticalSection(&cs);
  DeleteCriticalSection(&cs);
  std::cout << "tick: " << GetTickCount() << "\n";
  std::cout << "dacom_boot_harness: ok\n";
  return 0;
}
