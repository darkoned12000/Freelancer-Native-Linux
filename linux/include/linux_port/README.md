# linux_port headers

Shim/compat headers for Linux bring-up (#1).

- `win32_compat.h`: core Win32/DACOM-adjacent types, HRESULT/GUID/COM aliases, calling-convention macros.
- `win32_file.h`: file/find/mapping-related types, constants, and stub declarations.
- `win32_registry.h`: registry handle/key stubs (future XDG-backed mapping).
- `win32_threads.h`, `win32_sync.h`: critical sections/events/sync stubs.
- `win32_time.h`: timing stubs.
- `win32_string.h`: MSVC/Win32 string helper shims.
- `platform.h`, `module_loader.h`, `path_util.h`: Linux port platform utilities.

These are incremental and behavior-targeted; validate ABI/semantics at integration boundaries.