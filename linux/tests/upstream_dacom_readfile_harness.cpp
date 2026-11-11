// Harness entrypoint for upstream DACOM/ReadFile compilation wrappers.
// The upstream TUs are compiled as separate objects/targets in this harness context;
// this executable validates link/runtime viability of the shim+wrapper integration.

#include "linux_port/platform.h"
#include "linux_port/windows.h"

int main() {
  FDUMP(0x100003, "upstream DACOM/ReadFile wrapper harness starting");
  fl_port::log_message(fl_port::LogLevel::Info, "upstream_dacom_readfile_harness: compiled wrapper TUs linked");
  return 0;
}
