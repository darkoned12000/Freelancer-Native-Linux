// #1.2 integration harness: exercises upstream DACOM TU compiled into the 32-bit wrapper build.
// Calls harness-only hooks defined in upstream_dacom_tu.cpp after including upstream source.

#include "linux_port/platform.h"

#include <filesystem>
#include <iostream>

#if defined(FL_LINUX_PORT_UPSTREAM_TU_HARNESS_LINKED)
// not used; linkage is via object TU in 32-bit build
#endif

extern "C" int fl_upstream_dacom_integration_smoke(const char* ini_path);

int main() {
  const std::filesystem::path ini = std::filesystem::current_path() / "bin" / "DACOM.INI";
  const std::string ini_str = ini.string();
  fl_port::log_message(fl_port::LogLevel::Info, "upstream_dacom_integration_harness: starting");
  const int ok = fl_upstream_dacom_integration_smoke(ini_str.c_str());
  std::cout << "DACOM_Acquire/SetINIConfig/CreateInstance(RegExp)/Shutdown smoke: " << (ok ? "ok" : "fail") << "\n";
  fl_port::log_message(fl_port::LogLevel::Info, "upstream_dacom_integration_harness: complete");
  return ok ? 0 : 1;
}
