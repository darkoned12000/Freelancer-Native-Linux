#include "linux_port/module_loader.h"
#include "linux_port/path_util.h"
#include "linux_port/platform.h"
#include "linux_port/windows.h"

#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace {

int g_failures = 0;

void check(bool cond, const char* name) {
  if (!cond) {
    std::cerr << "[FAIL] " << name << "\n";
    ++g_failures;
  } else {
    std::cout << "[ok] " << name << "\n";
  }
}

std::filesystem::path make_temp_dir(const std::string& tag) {
  auto base = std::filesystem::temp_directory_path() / ("fl_m1_" + tag);
  std::error_code ec;
  std::filesystem::remove_all(base, ec);
  std::filesystem::create_directories(base / "SubDir", ec);
  return base;
}

void test_path_normalization_and_resolution() {
  const auto root = make_temp_dir("path");
  {
    std::ofstream(root / "SubDir" / "Config.INI") << "x=1\n";
    std::ofstream(root / "root.txt") << "abc";
  }

  // Separator normalization and drive-prefix stripping (engine-facing compatibility).
  check(fl_port::normalize_path_separators("C:\\Foo\\Bar\\..\\Baz.txt") == "Foo/Bar/../Baz.txt",
        "normalize strips drive and converts backslashes");

  fl_port::ResolveOptions opts;
  auto r1 = fl_port::resolve_path_case_insensitive(root, "subdir\\config.ini", opts);
  check(r1 && std::filesystem::exists(*r1), "resolve case-insensitive with backslashes");

  auto r2 = fl_port::resolve_path_case_insensitive(root, "SUBDIR/../SubDir/./config.ini", opts);
  check(r2 && std::filesystem::exists(*r2), "resolve with dot/dotdot segments");

  opts.allow_missing_tail = true;
  auto r3 = fl_port::resolve_path_case_insensitive(root, "SubDir/NewFolder\\newfile.dat", opts);
  check(r3.has_value() && r3->filename() == "newfile.dat", "resolve allow_missing_tail preserves tail");

  opts.allow_missing_tail = false;
  auto r4 = fl_port::resolve_path_case_insensitive(root, "SubDir/does_not_exist.ini", opts);
  check(!r4.has_value(), "resolve missing without allow_missing_tail fails");

  // Cache invalidation behavior.
  fl_port::clear_path_resolve_cache();
  auto r5 = fl_port::resolve_path_case_insensitive(root, "root.TXT", opts);
  check(r5 && std::filesystem::exists(*r5), "resolve after cache clear");

  // Engine-facing path APIs
  const auto old_cwd = std::filesystem::current_path();
  std::filesystem::current_path(root);
  char full[MAX_PATH]{};
  DWORD n = GetFullPathNameA("subdir\\config.ini", MAX_PATH, full, nullptr);
  check(n > 0 && std::strstr(full, "Config.INI") != nullptr, "GetFullPathNameA resolves existing case-insensitively");

  char* fp = _fullpath(nullptr, ".\\SubDir\\..\\SubDir\\config.ini", 0);
  check(fp != nullptr && std::filesystem::exists(std::filesystem::path(fp)), "_fullpath resolves normalized relative path");
  std::free(fp);

  // SearchPathA: found vs not-found behavior (Win32-compatible failure reporting).
  char spbuf[MAX_PATH]{};
  char* spfile = nullptr;
  SetLastError(0);
  DWORD spn = SearchPathA(root.string().c_str(), "root.txt", nullptr, MAX_PATH, spbuf, &spfile);
  check(spn != 0 && spfile != nullptr && std::string(spfile) == "root.txt", "SearchPathA finds existing file and sets file part");
  SetLastError(0);
  char spmiss[MAX_PATH]{};
  DWORD spm = SearchPathA(root.string().c_str(), "no_such_file_12345.txt", nullptr, MAX_PATH, spmiss, nullptr);
  check(spm == 0 && GetLastError() == 2, "SearchPathA returns 0 and ERROR_FILE_NOT_FOUND when missing");

  // GetFullPathNameA trailing-separator handling: lpFilePart should be NULL for directory-like paths.
  const std::string dir_with_sep = (root / "SubDir").string() + "/";
  char dirfull[MAX_PATH]{};
  char* dirpart = reinterpret_cast<char*>(0x1);
  DWORD dfn = GetFullPathNameA(dir_with_sep.c_str(), MAX_PATH, dirfull, &dirpart);
  check(dfn > 0 && dirpart == nullptr, "GetFullPathNameA sets lpFilePart NULL for trailing separator");

  std::filesystem::current_path(old_cwd);
  std::error_code ec;
  std::filesystem::remove_all(root, ec);
}

void test_file_io_and_find() {
  const auto root = make_temp_dir("file");
  const auto file_path = root / "SubDir" / "DataFile.BIN";

  HANDLE h = CreateFileA((root.string() + "\\subdir\\datafile.bin").c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, 4u /*OPEN_ALWAYS*/, FILE_ATTRIBUTE_NORMAL, nullptr);
  check(h != INVALID_HANDLE_VALUE, "CreateFileA case-insensitive path (write/create)");
  const char payload[] = "HELLO_M1";
  DWORD written = 0;
  check(WriteFile(h, payload, (DWORD)(sizeof(payload) - 1), &written, nullptr) == TRUE && written == sizeof(payload) - 1, "WriteFile writes payload");
  check(SetFilePointer(h, 0, nullptr, FILE_BEGIN) == 0, "SetFilePointer seeks to begin");
  char buf[32]{};
  DWORD read = 0;
  check(ReadFile(h, buf, 5, &read, nullptr) == TRUE && read == 5 && std::memcmp(buf, "HELLO", 5) == 0, "ReadFile reads data");
  DWORD sz = GetFileSize(h, nullptr);
  check(sz == sizeof(payload) - 1, "GetFileSize reports expected size");
  FILETIME ctime{}, atime{}, wtime{};
  check(GetFileTime(h, &ctime, &atime, &wtime) == TRUE && (wtime.dwLowDateTime != 0 || wtime.dwHighDateTime != 0),
        "GetFileTime returns timestamps for open file");
  check(CloseHandle(h) == TRUE, "CloseHandle closes file");

  DWORD attrs = GetFileAttributesA((root.string() + "/SUBDIR").c_str());
  check(attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY), "GetFileAttributesA detects directory");

  WIN32_FIND_DATAA fd{};
  HANDLE ffind = FindFirstFileA((root.string() + "/SubDir/*.bin").c_str(), &fd);
  check(ffind != INVALID_HANDLE_VALUE, "FindFirstFileA wildcard match");
  bool found = false;
  if (ffind != INVALID_HANDLE_VALUE) {
    do {
      if (std::string(fd.cFileName) == "DataFile.BIN" || std::string(fd.cFileName) == "datafile.bin") {
        found = true;
        check(fd.ftLastWriteTime.dwLowDateTime != 0 || fd.ftLastWriteTime.dwHighDateTime != 0,
              "FindFirstFileA reports file write time");
      }
    } while (FindNextFileA(ffind, &fd));
    FindClose(ffind);
  }
  check(found, "FindFirstFileA enumerates created file");

  // Anonymous file mapping (pagefile-backed) support
  HANDLE map = CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, 4096, nullptr);
  check(map != nullptr, "CreateFileMappingA(INVALID_HANDLE_VALUE) creates anonymous mapping");
  if (map) {
    void* view = MapViewOfFile(map, 0x000F /* FILE_MAP_ALL_ACCESS */, 0, 0, 0);
    check(view != nullptr, "MapViewOfFile maps anonymous mapping");
    if (view) {
      std::memcpy(view, "MAPOK", 5);
      check(std::memcmp(view, "MAPOK", 5) == 0, "anonymous mapping view is readable/writable");
      check(UnmapViewOfFile(view) == TRUE, "UnmapViewOfFile unmaps anonymous view");
    }
    check(CloseHandle(map) == TRUE, "CloseHandle closes mapping handle");
  }

  // Failure mode / error mapping
  SetLastError(0);
  HANDLE bad = CreateFileA((root.string() + "/SubDir/nope.txt").c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  check(bad == INVALID_HANDLE_VALUE && GetLastError() != 0, "CreateFileA missing file fails with last error");

  std::error_code ec;
  std::filesystem::remove_all(root, ec);
}

void test_timing_and_sync() {
  const DWORD t1 = GetTickCount();
  Sleep(2);
  const DWORD t2 = GetTickCount();
  check(t2 >= t1, "GetTickCount monotonic/non-decreasing over short interval");

  LARGE_INTEGER freq{}, c1{}, c2{};
  check(QueryPerformanceFrequency(&freq) == TRUE && freq.QuadPart > 0, "QPF returns positive frequency");
  check(QueryPerformanceCounter(&c1) == TRUE && QueryPerformanceCounter(&c2) == TRUE && c2.QuadPart >= c1.QuadPart,
        "QPC monotonic/non-decreasing");

  HANDLE ev = CreateEventA(nullptr, FALSE, FALSE, nullptr);
  check(ev != nullptr, "CreateEventA returns handle");
  check(WaitForSingleObject(ev, 1) == WAIT_TIMEOUT, "WaitForSingleObject times out when not signaled");
  check(SetEvent(ev) == TRUE, "SetEvent succeeds");
  check(WaitForSingleObject(ev, 50) == WAIT_OBJECT_0, "WaitForSingleObject acquires signaled auto-reset event");
  check(WaitForSingleObject(ev, 1) == WAIT_TIMEOUT, "auto-reset event resets after wait");
  check(CloseHandle(ev) == TRUE, "CloseHandle closes event handle");

  CRITICAL_SECTION cs;
  InitializeCriticalSection(&cs);
  check(TryEnterCriticalSection(&cs) == TRUE, "TryEnterCriticalSection acquires unlocked CS");
  LeaveCriticalSection(&cs);
  EnterCriticalSection(&cs);
  EnterCriticalSection(&cs);
  LeaveCriticalSection(&cs);
  LeaveCriticalSection(&cs);
  DeleteCriticalSection(&cs);
  check(true, "recursive critical section enter/leave sequence");

  volatile long v = 0;
  check(InterlockedIncrement(&v) == 1 && v == 1, "InterlockedIncrement");
  check(InterlockedDecrement(&v) == 0 && v == 0, "InterlockedDecrement");
  check(InterlockedCompareExchange(&v, 5, 0) == 0 && v == 5, "InterlockedCompareExchange success");
  check(InterlockedCompareExchange(&v, 7, 0) == 5 && v == 5, "InterlockedCompareExchange failure preserves value");
  check(InterlockedExchange(&v, 9) == 5 && v == 9, "InterlockedExchange");

  void* volatile pv = nullptr;
  int a = 1, b = 2;
  void* oldp = InterlockedCompareExchangePointer(&pv, &a, nullptr);
  check(oldp == nullptr && pv == &a, "InterlockedCompareExchangePointer");
  void* xp = InterlockedExchangePointer(&pv, &b);
  check(xp == &a && pv == &b, "InterlockedExchangePointer");
}

void test_module_resolution_and_config_variants() {
  const auto root = make_temp_dir("modcfg");
  std::error_code ec;
  std::filesystem::create_directories(root / "mods", ec);
  // Module candidates
  {
    std::ofstream(root / "mods" / "libExample.so") << "";
    std::ofstream(root / "mods" / "Plain.so") << "";
  }
  auto m1 = fl_port::resolve_module_path("Example.dll", root / "mods");
  check(m1 && m1->filename() == "libExample.so", "resolve_module_path maps .dll to lib<stem>.so");
  auto m2 = fl_port::resolve_module_path("Plain", root / "mods");
  check(m2.has_value(), "resolve_module_path resolves extensionless module candidate");

  // Config/INI parsing variants and failure modes via probe-like behavior expectations:
  // - missing file: resolve/read should fail gracefully
  // - empty/odd content: file APIs should still behave predictably
  const auto ini = root / "mods" / "variant.ini";
  {
    std::ofstream out(ini);
    out << "; comment\r\n";
    out << "[Libraries]\r\n";
    out << "ReadFile.dll\r\n";
    out << "\r\n";
    out << "[Section With Spaces]\r\n";
    out << "Key = Value\r\n";
    out << "BareEntry\r\n";
  }
  HANDLE h = CreateFileA(ini.string().c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  check(h != INVALID_HANDLE_VALUE, "CreateFileA opens config variant");
  char buf[256]{};
  DWORD got = 0;
  check(ReadFile(h, buf, sizeof(buf) - 1, &got, nullptr) == TRUE && got > 0, "ReadFile reads config variant");
  CloseHandle(h);

  auto missing = fl_port::resolve_path_case_insensitive(root, "mods\\no_such_config.ini", fl_port::ResolveOptions{false});
  check(!missing.has_value(), "config path resolution failure mode (missing file)");

  std::filesystem::remove_all(root, ec);
}

void test_abi_sensitive_layout() {
  // ABI-sensitive checks (most critical in 32-bit builds; still validated in 64-bit for regressions in shim types).
  check(sizeof(BYTE) == 1, "sizeof(BYTE)==1");
  check(sizeof(WORD) == 2, "sizeof(WORD)==2");
  check(sizeof(DWORD) == 4, "sizeof(DWORD)==4");
  check(sizeof(LONG) == 4, "sizeof(LONG)==4");
  check(sizeof(BOOL) == 4, "sizeof(BOOL)==4");
  check(sizeof(GUID) == 16, "sizeof(GUID)==16");
  check(sizeof(FILETIME) == 8, "sizeof(FILETIME)==8");
  check(sizeof(LARGE_INTEGER) == 8, "sizeof(LARGE_INTEGER)==8");
  check(sizeof(ULARGE_INTEGER) == 8, "sizeof(ULARGE_INTEGER)==8");
  {
    LARGE_INTEGER li{};
    li.LowPart = 0x11223344u;
    li.HighPart = 0x55667788;
    check(li.u.LowPart == 0x11223344u && li.u.HighPart == 0x55667788, "LARGE_INTEGER anonymous/u fields alias");
  }
#if defined(__i386__) || defined(__i386) || defined(i386)
  check(sizeof(void*) == 4, "32-bit pointer size");
  check(sizeof(SIZE_T) == 4, "32-bit SIZE_T size");
  check(sizeof(HWND) == 4, "32-bit HWND placeholder size");
  check(sizeof(DWORD_PTR) == 4, "32-bit DWORD_PTR size");
#else
  check(sizeof(void*) == 8, "64-bit pointer size");
  check(sizeof(DWORD_PTR) == 8, "64-bit DWORD_PTR size");
#endif
  std::cout << "[info] ABI/layout summary: ptr=" << sizeof(void*) << " size_t=" << sizeof(size_t)
            << " HANDLE=" << sizeof(HANDLE) << " HWND=" << sizeof(HWND) << "\n";
}

} // namespace

int main() {
  fl_port::log_message(fl_port::LogLevel::Info, "m1_platform_tests starting");
  test_path_normalization_and_resolution();
  test_file_io_and_find();
  test_timing_and_sync();
  test_module_resolution_and_config_variants();
  test_abi_sensitive_layout();

  if (g_failures == 0) {
    std::cout << "m1_platform_tests: PASS\n";
    return 0;
  }
  std::cerr << "m1_platform_tests: FAIL (" << g_failures << ")\n";
  return 1;
}
