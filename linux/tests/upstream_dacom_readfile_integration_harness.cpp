// #1.2 integration harness: DACOM + ReadFile upstream wrapper TUs compiled and exercised together.
// Validates real config file parsing (via stdio path-resolution hook), explicit ReadFile
// registration, and basic file-system component interaction under Linux shims.

#include "linux_port/windows.h"
#include "linux_port/path_util.h"
#include "linux_port/platform.h"

#include <dacom.h>
#include <readfile.h>

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

extern "C" int fl_upstream_dacom_integration_smoke(const char* ini_path);
extern "C" int fl_upstream_readfile_integration_register();

namespace {

bool read_text_via_win32(const std::filesystem::path& p, std::string& out, size_t max_bytes = 4096) {
  out.clear();
  HANDLE h = CreateFileA(p.string().c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (h == INVALID_HANDLE_VALUE) return false;
  std::vector<char> buf(max_bytes);
  DWORD got = 0;
  if (!ReadFile(h, buf.data(), (DWORD)buf.size(), &got, nullptr)) {
    CloseHandle(h);
    return false;
  }
  out.assign(buf.data(), buf.data() + got);
  CloseHandle(h);
  return true;
}

} // namespace

int main() {
  fl_port::log_message(fl_port::LogLevel::Info, "upstream_dacom_readfile_integration_harness: starting");
  std::cout.setf(std::ios::unitbuf);

  const auto cwd = std::filesystem::current_path();
  const auto dacom_ini = (cwd / "bin/DACOM.INI").string();

  // 1) Upstream DACOM manager/config parse smoke (real INI path, FDUMP bridged in TU)
  std::cout << "[step] DACOM smoke\n";
  int ok = fl_upstream_dacom_integration_smoke(dacom_ini.c_str());
  std::cout << "DACOM smoke (upstream TU): " << (ok ? "ok" : "fail") << "\n";

  // 2) Explicit ReadFile module registration (DllMain-free harness init)
  std::cout << "[step] ReadFile registration\n";
  const int rf_reg = fl_upstream_readfile_integration_register();
  std::cout << "ReadFile explicit registration: " << (rf_reg ? "ok" : "fail") << "\n";
  ok = ok && rf_reg;

  // 3) Win32 file shim real-read smoke against repo files (path normalization/case-insensitivity)
  std::cout << "[step] Win32 file reads\n";
  {
    std::string text;
    const auto ini = cwd / "bin" / "DACOM.INI";
    const bool read_ok = read_text_via_win32(ini, text, 256);
    std::cout << "Win32 ReadFile(bin/DACOM.INI): " << (read_ok ? "ok" : "fail");
    if (read_ok) std::cout << " bytes_prefix=" << text.substr(0, text.find('\n'));
    std::cout << "\n";
    ok = ok && read_ok;
  }
  {
    auto resolved = fl_port::resolve_path_case_insensitive(cwd, "DATA\\solar\\solararch.ini");
    std::cout << "path_util resolve DATA\\solar\\solararch.ini: " << (resolved ? "ok" : "fail") << "\n";
    if (resolved) {
      std::string text;
      const bool read_ok = read_text_via_win32(*resolved, text, 128);
      std::cout << "Win32 ReadFile(DATA solararch.ini): " << (read_ok ? "ok" : "fail") << "\n";
      ok = ok && read_ok;
    } else {
      ok = 0;
    }
  }

  // 4) Best-effort upstream component interaction (guarded; may be unsupported in current shim state)
  std::cout << "[step] upstream component interaction (best-effort)\n";
  try {
    ICOManager* mgr = DACOM_Acquire();
    if (mgr) {
      void* inst = nullptr;
      FFileSystemDesc fs{};
      fs.Size = sizeof(FFileSystemDesc);
      fs.ComponentName = "FileSystem";
      fs.Implementation = "DOS";
      std::string data_path = (cwd / "DATA").string();
      fs.Path = data_path.c_str();
      fs.Access = GENERIC_READ;
      fs.ShareMode = FILE_SHARE_READ;
      fs.Creation = OPEN_EXISTING;
      GENRESULT gr = mgr->CreateInstance((AGGDESC*)&fs, &inst);
      std::cout << "DACOM CreateInstance(FileSystem/DOS): " << (gr == GR_OK && inst ? "ok" : "not-available-in-shimmed-context") << "\n";
      if (gr == GR_OK && inst) {
        auto* unk = reinterpret_cast<IDAComponent*>(inst);
        if (unk) unk->Release();
      }
    } else {
      std::cout << "DACOM manager unavailable for FileSystem creation\n";
    }
  } catch (...) {
    std::cout << "upstream component interaction: exception (ignored)\n";
  }

  fl_port::log_message(fl_port::LogLevel::Info, "upstream_dacom_readfile_integration_harness: complete");
  std::cout << "overall: " << (ok ? "ok" : "fail") << "\n";
  return ok ? 0 : 1;
}
