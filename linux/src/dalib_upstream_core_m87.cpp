// M8.7 experimental: attempt to compile upstream src/DALib/DALib.cpp as a core TU
// for Linux by providing stub headers and excluding heavy Startup/Shutdown bodies.
// This target is used to drive "real upstream DALib core TU" integration progress;
// unresolved dependencies may require iterative stubbing/guarding.
#define FL_LINUX_PORT_DALIB_CORE_ONLY 1

#include "linux_port/windows.h"
#include "linux_port/platform.h"

// Include upstream DALib declarations before renaming excluded heavy functions so the
// DALib namespace/API signatures remain visible to this wrapper and dependents.
#include <dacom.h>
#include <readfile.h>
#include "dalib_core.h"
#include "DALib.h"

// Rename heavy functions defined in upstream DALib.cpp (excluded from core-only bring-up)
// before including the upstream source so we can provide Linux bring-up stubs below.
#define Startup fl_m87_upstream_DALib_Startup_excluded
#define Shutdown fl_m87_upstream_DALib_Shutdown_excluded
#define DllMain fl_m87_upstream_DALib_DllMain_excluded

// Note: do not macro-redefine OutputDebugStringA; linux_port provides a compatible inline.

#include "../../src/DALib/DALib.cpp"

#undef Startup
#undef Shutdown
#undef DllMain

namespace DALib {
bool __cdecl Startup(HWND /*Hwnd*/, const char* /*IniPath*/) {
  fl_port::log_message(fl_port::LogLevel::Warning,
                       "DALib::Startup (M8.7 upstream-core TU stub): full subsystem init pending");
  if (!Dacom) Dacom = DACOM_Acquire();
  return Dacom != nullptr;
}

void __cdecl Shutdown(HWND /*Hwnd*/) {
  fl_port::log_message(fl_port::LogLevel::Warning,
                       "DALib::Shutdown (M8.7 upstream-core TU stub): full subsystem teardown pending");
}
} // namespace DALib
