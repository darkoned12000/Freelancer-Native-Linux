// Compile-scope probe for incremental upstream DACOM/ReadFile integration (#1.2).
// This TU does NOT yet compile src/dacom/dacom.cpp or src/ReadFile/ReadFile.cpp
// directly (they still depend on Win32/MSVC-only constructs, resources, DllMain,
// and broader ABI/build assumptions). Instead it validates that our Linux
// <windows.h> facade + shims can satisfy common declarations/patterns used by
// those modules in a guarded harness context.

#include "linux_port/windows.h"
#include "linux_port/module_loader.h"
#include "linux_port/path_util.h"
#include "linux_port/platform.h"

#include <cstddef>
#include <cstdint>

namespace {

// Patterns commonly seen around DACOM/ReadFile bring-up surfaces.
using PFN_Startup = long (DACOM_API *)(void*);

struct IComponentStub {
  virtual long DACOM_API QueryInterface(REFIID, void**) = 0;
  virtual unsigned long DACOM_API AddRef() = 0;
  virtual unsigned long DACOM_API Release() = 0;
  virtual ~IComponentStub() = default;
};

static int volatile_sink = 0;

void touch_file_apis() {
  HANDLE h = CreateFileA("dummy", GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
  (void)h;
  WIN32_FIND_DATAA fd{};
  (void)fd;
  (void)GetFileAttributesA(".");
  (void)CreateFileMappingA(nullptr, nullptr, 0, 0, 0, nullptr);
  (void)MapViewOfFile(nullptr, 0, 0, 0, 0);
}

void touch_sync_time_debug() {
  CRITICAL_SECTION cs;
  InitializeCriticalSection(&cs);
  EnterCriticalSection(&cs);
  LeaveCriticalSection(&cs);
  DeleteCriticalSection(&cs);
  Sleep(0);
  LARGE_INTEGER li{};
  QueryPerformanceCounter(&li);
  QueryPerformanceFrequency(&li);
  OutputDebugStringA("compile probe\n");
  FDUMP(0x100002, "probe warning path %d", 1);
}

} // namespace

int main() {
  touch_file_apis();
  touch_sync_time_debug();
  auto p = fl_port::resolve_path_case_insensitive(std::filesystem::current_path(), "bin\\DACOM.INI");
  (void)p;
  (void)volatile_sink;
  fl_port::log_message(fl_port::LogLevel::Info, "dacom_upstream_compile_probe: ok");
  return 0;
}
