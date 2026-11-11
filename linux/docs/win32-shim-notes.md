# Win32 shim notes (#1, ongoing)

This document tracks incremental decisions for the Linux platform/shim layer.

## Calling conventions / ABI (32-bit)
- Upstream x86 Win32 code may rely on `__stdcall`, `__cdecl`, `__fastcall`, and MSVC `__thiscall` semantics (including COM-style/DACOM interface vtables).
- Linux GCC/Clang i386 defaults differ from MSVC member-function ABI. Current `win32_compat.h` provides attribute-based placeholders/mappings; **each interface boundary must be validated** (stack cleanup, `this` passing, struct returns, vtable layout assumptions).
- Prefer compiling port/harness TUs with Clang and `-fms-extensions` where helpful, and add targeted tests for ABI-sensitive calls.

## `HWND` and windowing
- `HWND` is treated as an opaque handle placeholder, not a behavioral Win32 window object.
- Window/message operations required by engine subsystems (sync/async behavior, client/screen coords, focus/lifecycle, presentation target) must be adapted to SDL3/Wayland-backed implementations rather than integer stubs.

## Module loading and initialization
- `module_loader` currently wraps `dlopen`/`dlsym` with path/name resolution helpers.
- DACOM `DllMain`-style registration side effects and load order semantics are not preserved by dynamic loading alone; early bring-up may require static linking or explicit init entrypoints before restoring plugin-style loading.

## Implemented skeletons
- `win32_compat.h`: basic types/result codes, GUID compare/sort/`IsEqual*` helpers, 32-bit `__fastcall` attribute mapping, and SEH placeholders (`__try`/`__except`) for non-Windows bring-up.
- `win32_threads.h`: recursive `CRITICAL_SECTION` (`PTHREAD_MUTEX_RECURSIVE`), spin/try helpers.
- `win32_sync.h`: minimal auto-reset/manual-reset event object + `WaitForSingleObject`/timeout semantics.
- `win32_time.h`: `GetTickCount`, `Sleep`, `LARGE_INTEGER`, `QueryPerformanceCounter/Frequency`.
- `win32_string.h`: `_stricmp`/`_strnicmp`, `strncpy_s` with `errno_t`/`_TRUNCATE` semantics.
- `win32_file.h`: file constants/types and incremental stubs (including mapping APIs placeholders).
- `win32_debug.h`: `OutputDebugStringA` routed to `fl_port::debug_output`.
- `win32_registry.h`: linkable stub registry APIs returning standard Win32-style error codes (XDG-backed behavior TBD).
- `platform.h/.cpp`: thread-safe logging with timestamps/thread IDs and debug-output hook.
- `module_loader.h/.cpp`: move-only `ModuleHandle`, candidate resolution (`.dll`/lib*.so), `dlopen`/`dlsym`, and thread-local last error.

## SEH / exceptions
- Upstream code may rely on MSVC SEH (`__try`/`__except`). On Linux targets we currently provide no-op placeholders in `win32_compat.h`; memory-safety probing and plugin-isolation behavior must be replaced case-by-case with explicit checks, C++ exceptions, or signal handling as modules are ported.

## Next increments
- File/path APIs and structures needed by `ReadFile`/config loading (real implementations, including memory-mapped archive access as required).
- Additional synchronization primitives/wait dispatch used by audio/movie/server code.
- Registry/config storage shim boundary (XDG-backed) for Linux runtime.
- Error/reporting conventions aligned with DACOM `FDUMP`/debug paths.