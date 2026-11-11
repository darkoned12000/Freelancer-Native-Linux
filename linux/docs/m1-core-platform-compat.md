# M1 — Core platform compatibility (status)

**Status:** completed in working tree (pending code review, not committed).  
**Acceptance target:** `DACOM` and `ReadFile` compile or run in harnessed form and pass standalone tests for targeted behavior.

## Summary

M1 focused on making the Linux bring-up platform layer more behavior-faithful and engine-facing (beyond probe-only use), while preserving the explicit-registration/static-link-first strategy for source-only checkouts.

## Implemented

### Filesystem/path normalization (engine-facing)
- Integrated normalization and case-insensitive resolution into engine-facing Win32/MSVCRT path APIs:
  - `GetFullPathNameA`, `_fullpath` (implemented in `linux/src/win32_file_shim.cpp`)
  - `SearchPathA` (improved minimal search/resolve behavior in `linux/include/linux_port/win32_compat.h`)
  - `GetCurrentDirectoryA`, `SetCurrentDirectoryA`
  - `_access`, `_mkdir` (path-resolving shims in `linux/include/linux_port/io.h`)
- Existing Linux-backed file APIs (`CreateFileA`, `ReadFile`/`WriteFile`, `GetFileAttributesA`, `FindFirstFileA`, mapping helpers, etc.) continue to normalize/resolve inputs via `fl_port::path_util`.
- Covered edge cases in tests: Windows separators, drive-prefix stripping behavior, mixed case, `.`/`..`, missing-tail resolution, and cache reset.

### Win32 shim fidelity, timing, threading/sync
- Timing: monotonic `GetTickCount`, `GetTickCount64`, `timeGetTime`, `Sleep`, `SleepEx` (non-alertable completion), `QueryPerformanceCounter`/`QueryPerformanceFrequency` (`linux/include/linux_port/win32_time.h`).
- Threading/sync: recursive critical sections, basic event create/set/reset/wait/close semantics and unified event-handle close handling (`linux/include/linux_port/win32_threads.h`, `win32_sync.h`, file shim `CloseHandle`).
- Common primitives: `InterlockedIncrement/Decrement/Exchange/CompareExchange`, improved `DosDateTimeToFileTime`, directory helpers, and related compatibility behavior (`linux/include/linux_port/win32_compat.h`, `win32_file.h`, shim implementation).

### Logging/diagnostics and error mapping
- Preserved FDUMP/platform logging bridge (`fl_port::fdump`, `fl_port::log_message`) and improved/retained Win32-style `GetLastError` behavior for common file/path shim success/failure cases.
- Harness diagnostics clarified for expected source-only absence of runtime module binaries (e.g. `dacom_boot_harness` resolve messaging).

### Module loading strategy
- Retained explicit/static registration as the primary bring-up path for harnessed upstream integration.
- Continued/refined dynamic loading primitives in `fl_port::module_loader`: case-insensitive module path resolution, `.dll`/no-ext/`.so` candidate mapping, `dlopen`/`dlsym`/`dlclose`, and thread-local last-error reporting.

### DACOM/ReadFile harnessed validation and targeted tests
- 32-bit upstream DACOM/ReadFile wrapper integration remains buildable/runnable (`upstream_dacom_readfile_integration_harness` reports `overall: ok` in source-only checkout).
  - DACOM smoke step uses a filtered config without `[Libraries]` entries for this harness context to avoid incorrect auto-load attempts when runtime module binaries are not provided; modules are exercised via explicit registration/integration paths.
- Added `linux/tests/m1_platform_tests.cpp` (`m1_platform_tests` target) covering:
  - path normalization/resolution edge cases and engine-facing path API behavior
  - Linux-backed Win32 file I/O, attributes, wildcard find, and failure/error mapping
  - config variant/failure-mode read behavior
  - timing, synchronization, event wait behavior, and interlocked primitives
  - module path resolution candidates
  - ABI-sensitive type/layout checks (including 32-bit pointer/handle sizes)

## Validation (host)

- **32-bit (`linux/build-x86`)**: `m1_platform_tests` PASS; `core_shim_harness` ok; `dacom_boot_harness` ok; `upstream_dacom_readfile_integration_harness` `overall: ok`.
- **64-bit (`linux/linux-x64`)**: `m1_platform_tests` PASS and core bring-up targets build/run as applicable.

> Note: runtime module DLL/SO artifacts listed in `DACOM.INI [Libraries]` are not present in this source-only checkout; `resolve=<missing: ...>` and missing-resource reports in boot/config probes are expected.

## Follow-on (M2+)
- Integrate path normalization/module resolution into actual engine boot/runtime call paths as more upstream modules are brought up.
- Replace harness-scoped DACOM init assumptions (e.g. filtered `[Libraries]` smoke config) with behavior-faithful Linux module initialization/registration flows as component loading is implemented.
- Expand failure-mode/config-variant fixtures and Windows-vs-Linux behavior comparisons for boot-critical systems.
- Implement/validate more Win32 APIs and semantics on demand (registry/XDG persistence, fuller SearchPath semantics, alertable waits, etc.) based on real usage.
