#include "linux_port/module_loader.h"
#include "linux_port/path_util.h"
#include "linux_port/platform.h"
#include "linux_port/win32_compat.h"
#include "linux_port/win32_file.h"
#include "linux_port/win32_registry.h"
#include "linux_port/win32_threads.h"
#include "linux_port/win32_time.h"

#include <filesystem>
#include <iostream>
#include <string>

namespace {

int g_counter = 0;
CRITICAL_SECTION g_cs;

void increment_locked() {
  EnterCriticalSection(&g_cs);
  ++g_counter;
  LeaveCriticalSection(&g_cs);
}

} // namespace

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;

  fl_port::log_message(fl_port::LogLevel::Info, "core_shim_harness starting");

  // win32_compat basic checks
  GUID a{0x12345678u, 0x1234u, 0x5678u, {1,2,3,4,5,6,7,8}};
  GUID b = a;
  GUID c{0x87654321u, 0x1234u, 0x5678u, {1,2,3,4,5,6,7,8}};
  std::cout << "GUID equality: " << ((a == b) ? "ok" : "fail") << "\n";
  std::cout << "GUID inequality: " << ((a != c) ? "ok" : "fail") << "\n";
  std::cout << "SUCCEEDED(S_OK): " << (SUCCEEDED(S_OK) ? "ok" : "fail") << "\n";
  std::cout << "FAILED(E_FAIL): " << (FAILED(E_FAIL) ? "ok" : "fail") << "\n";

  // threading/time stubs (recursive CS behavior is required by Win32 semantics)
  InitializeCriticalSection(&g_cs);
  for (int i = 0; i < 500; ++i) {
    EnterCriticalSection(&g_cs);
    EnterCriticalSection(&g_cs); // must not deadlock
    ++g_counter;
    LeaveCriticalSection(&g_cs);
    LeaveCriticalSection(&g_cs);
  }
  increment_locked();
  DeleteCriticalSection(&g_cs);
  std::cout << "CRITICAL_SECTION counter: " << g_counter << " (expected 501)\n";
  std::cout << "GetTickCount(): " << GetTickCount() << " ms\n";
  {
    LARGE_INTEGER freq{}, ctr{};
    if (QueryPerformanceFrequency(&freq) && QueryPerformanceCounter(&ctr)) {
      std::cout << "QPC/QPF: ok (" << freq.QuadPart << ")\n";
    } else {
      std::cout << "QPC/QPF: fail\n";
    }
  }

  // file/registry shim type checks (compile-time/runtime smoke)
  FILETIME ft{0, 0};
  WIN32_FIND_DATAA fd{};
  (void)ft; (void)fd;
  std::cout << "FILE_ATTRIBUTE_DIRECTORY: 0x" << std::hex << FILE_ATTRIBUTE_DIRECTORY << std::dec << "\n";
  HKEY hk = HKEY_CURRENT_USER;
  (void)hk;

  // path + module resolution smoke (repo-relative if run from repo root)
  const std::filesystem::path repo = std::filesystem::current_path();
  auto data_ini = fl_port::resolve_path_case_insensitive(repo, "bin\\freelancer.ini");
  auto dacom_ini = fl_port::resolve_path_case_insensitive(repo, "bin\\DACOM.INI");
  auto data_dir = fl_port::resolve_path_case_insensitive(repo, "DATA\\solar\\solararch.ini");
  std::cout << "resolve bin\\DACOM.INI: " << (dacom_ini ? dacom_ini->string() : "<not found>") << "\n";
  std::cout << "resolve bin\\freelancer.ini: " << (data_ini ? data_ini->string() : "<not found>") << "\n";
  std::cout << "resolve DATA\\solar\\solararch.ini: " << (data_dir ? data_dir->string() : "<not found>") << "\n";

  auto mod = fl_port::resolve_module_path("ReadFile.dll", repo / "bin");
  std::cout << "resolve_module_path(ReadFile.dll, bin): " << (mod ? mod->string() : "<not found, expected in source-only checkout>") << "\n";

  fl_port::log_message(fl_port::LogLevel::Info, "core_shim_harness complete");
  return (g_counter == 501 && (a == b) && (a != c)) ? 0 : 1;
}
