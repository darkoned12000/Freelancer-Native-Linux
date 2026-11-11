# Platform abstraction/shim skeleton (#1)

Initial Linux bring-up scaffolding for porting/harnessing core modules (`dacom`, `ReadFile`, etc.) without depending on `windows.h` or Win32 runtime APIs.

## Added (skeleton)

- `linux/include/linux_port/win32_compat.h`
  - Minimal Win32-compatible types/macros for Linux builds (`BOOL`, `DWORD`, `HRESULT`, `HANDLE`/`HMODULE`/`HWND` placeholders, `GUID` with equality operators, `REFGUID`/`REFIID`/`REFCLSID` aliases, common result macros).
  - Calling-convention macro mappings/placeholders (`WINAPI`/`STDMETHODCALLTYPE`, `CDECL`, `THISCALL` attributes on i386 GCC/Clang) for bring-up; **ABI validation required** per interface/signature.
  - Intentionally minimal; COM method macros and broader API shims are deferred until concrete porting needs are known.

- `linux/include/linux_port/platform.h`, `linux/src/platform.cpp`
  - Minimal logging/debug-output hooks (`fl_port::log_message`, `fl_port::debug_output`) to replace `OutputDebugStringA`-style diagnostics in harnessed code.

- `linux/include/linux_port/module_loader.h`, `linux/src/module_loader.cpp`
  - Module loading abstraction skeleton (`resolve_module_path`, `load_module`, `get_module_symbol`, `unload_module`, `module_loader_last_error`).
  - Uses `path_util` for normalized/case-insensitive name resolution and basic `.dll`/extensionless/`.so` candidate mapping (including `parent_path/lib<stem>.so` handling).
  - Captures `dlerror()` into a thread-local last-error string on load/symbol failures.
  - Actual DACOM-compatible search/load/registration semantics are **not** implemented yet.

- `linux/include/linux_port/win32_threads.h`, `linux/include/linux_port/win32_time.h`
  - Initial stubs/mappings: `CRITICAL_SECTION` (`pthread_mutex_t`) init/enter/leave/delete; `GetTickCount()` via `std::chrono::steady_clock`.

- `linux/include/linux_port/win32_threads.h`, `linux/include/linux_port/win32_time.h`
  - Initial stubs/mappings: `CRITICAL_SECTION` (`pthread_mutex_t`) init/enter/leave/delete; `GetTickCount()` via `std::chrono::steady_clock`.

- `linux/include/linux_port/win32_file.h`, `linux/include/linux_port/win32_registry.h`
  - Minimal file/find/time-structure and registry handle/key type/constant skeletons for compilation/harness bring-up.

- `linux/cmake/toolchain-linux-x86.cmake`
  - i686/Linux CMake toolchain skeleton (`-m32` flags) for 32-bit bring-up and dependency configuration consistency.

- `linux/tests/core_shim_harness.cpp` (`core_shim_harness`)
  - Compile/run smoke harness exercising core shims, `path_util` config/data resolution, and module path mapping behavior.

## Integration intent (next steps)
1. Introduce guarded usage in Linux-port/harness translation units (avoid breaking upstream Windows builds).
2. Expand shims incrementally with only the APIs/types needed by `dacom`/`ReadFile` and early harnesses (file/time/sync/module/calling-convention specifics).
3. Define a DACOM module initialization strategy for Linux (static registration vs exported init entrypoints via `dlopen`), preserving load/registration order semantics.
4. Add compile checks/tests for shimmed code paths and ABI-sensitive boundaries (especially calling conventions and struct layout/returns on i386).

## Cautions
- `HWND` and other handle placeholders are opaque adapters, not behavioral Win32 objects.
- Calling-convention compatibility on 32-bit x86 is a correctness risk; validate per interface/function signature.
- Do not assume `dlopen` module loading is sufficient without explicit handling of DACOM `DllMain`/registration side effects.