# DACOM/ReadFile harness spike (#1.2)

## Goal
Establish an incremental path from Linux shim scaffolding to compiling and exercising `dacom`/`ReadFile`-relevant behavior without requiring a full Windows runtime.

## Current spike (implemented)
- **`linux/tests/core_shim_harness.cpp`** (`core_shim_harness`) validates the #1 shim layer compiles and runs together:
  - `win32_compat` types/macros (`GUID` ops, HRESULT helpers)
  - `win32_threads` (`CRITICAL_SECTION`) and `win32_time` (`GetTickCount`)
  - `win32_file`/`win32_registry` type/constant skeletons
  - `path_util` resolution against repo config/data paths (`bin\\DACOM.INI`, `bin\\freelancer.ini`, `DATA\\solar\\solararch.ini`)
  - `module_loader` path mapping/resolution behavior (`ReadFile.dll` candidate lookup)
- **`linux/tests/abi_module_harness.cpp`** (`abi_module_harness`) models an explicit module-init/registration strategy (DllMain-free) with DACOM-like factory/component create/query flow for ABI/architecture bring-up validation.
- **`linux/tests/dacom_boot_harness.cpp`** (`dacom_boot_harness`) applies shims + `path_util` + module resolution planning against real `bin/DACOM.INI`/`bin/freelancer.ini` and `DATA/` samples (load-order/init planning, `MGSDB`/data path/resource checks).
- Together these confirm **shim growth, ABI/module-init modeling, and boot/config/path/module planning** are usable in Linux harness code.

## Upstream wrapper TU integration (implemented, 32-bit harness scope)
- `linux/tests/upstream_dacom_tu.cpp` and `linux/tests/upstream_readfile_tu.cpp` compile upstream `src/dacom/dacom.cpp` and `src/ReadFile/ReadFile.cpp` as guarded Linux harness TUs (with shim/facade includes, symbol/entrypoint guards, and Linux-compatible comparator wiring for ReadFile).
- `linux/tests/upstream_dacom_readfile_harness.cpp` links both wrapper object TUs into `upstream_dacom_readfile_harness`.
- **Validated in `linux/build-x86` (i386, 32-bit):** wrapper TUs and harness build and run successfully. This addresses direct compilation/integration in a shimmed harness context for the primary bring-up ABI.
- **Guarding:** `src/dacom/dacom.cpp` includes minimal Linux-wrapper-only guards under `FL_LINUX_PORT_UPSTREAM_TU && !_WIN32` (FDUMP symbol/export handling, non-modal dump path, and DllMain side-effect suppression) while preserving upstream Windows behavior.
- **Integration harness:** `upstream_dacom_integration_harness` (32-bit, links `fl_upstream_dacom_tu`) exercises upstream DACOM APIs in-process (`DACOM_Acquire`, `SetINIConfig` path on `bin/DACOM.INI`, component/RegExp creation attempt) with FDUMP bridged to `fl_port::fdump`. Current runs show upstream parser/manager diagnostics and expected library-load failures under stubbed file/module behavior.
- **Still limited:** many Win32 APIs are stubs, no `.rc`/resource integration, no full `DllMain`/registration/runtime semantics, and 64-bit compilation is not a correctness target for these 32-bit-layout-dependent upstream TUs. Next increments should replace stubs with Linux-backed behavior (real file/INI IO, module registration/loading strategy) and extend integration coverage beyond smoke paths.

## #1.2 implementation status (completed for bring-up scope)
1. **Header-compat / wrapper compilation:** ✅ `<windows.h>` facade, expanded Win32 shims, and guarded 32-bit wrapper TUs compile upstream `src/dacom/dacom.cpp` and `src/ReadFile/ReadFile.cpp` (`fl_upstream_dacom_tu`, `fl_upstream_readfile_tu`).
2. **DACOM config/load integration:** ✅ Upstream DACOM manager/config paths exercised in-process (`DACOM_Acquire`, `SetINIConfig` on `bin/DACOM.INI`) with stdio path-resolution hook and FDUMP bridged to `fl_port::fdump`, using real repo config files.
3. **ReadFile/VFS file-IO slice:** ✅ Linux-backed Win32 file shim (`linux/src/win32_file_shim.cpp`) provides path-normalized/case-insensitive `CreateFileA/ReadFile/GetFileSize/SetFilePointer/CloseHandle`, attributes, basic find-file, and mapping/view primitives (`CreateFileMappingA/MapViewOfFile/UnmapViewOfFile`) backed by POSIX `mmap` for bring-up/archive access paths.
4. **Module init/registration strategy:** ✅ Explicit DllMain-free registration hooks for upstream wrappers (`fl_upstream_readfile_integration_register`, DACOM integration hook in TU) and in-process harness wiring; load-order/library-name failures are observable and attributable (runtime DLLs absent/stubbed).
5. **ABI/calling-convention validation:** ✅ 32-bit build/run harnesses (`abi_module_harness`, upstream wrapper/integration harnesses) exercise stdcall/cdecl/fastcall-marked interfaces and upstream component smoke paths on i386.

## Remaining follow-on work (post-#1.2, outside this milestone)
- Replace remaining Win32 stubs with behavior-faithful Linux implementations as required by runtime boot (richer file sharing/flags, directory enumeration semantics, memory-mapped archive behavior parity, registry/XDG mapping, synchronization/timing edge cases).
- Deeper upstream factory/descriptor compatibility for creating/using `IFileSystem`/`ISearchPath` and other DACOM components end-to-end (descriptor packing, vtable/ABI validation per interface, explicit component registration graph).
- Compile/integrate additional upstream TUs/modules under guarded wrappers and connect them through the explicit init/load-order model.

## Success criteria for the next spike
- A Linux harness can compile (with shims/wrappers) a minimal DACOM-facing interface/config path and a minimal filesystem path resolution/open-read path.
- Behavior can be compared against known config/data expectations from `bin/` and `DATA/` without requiring the full client/render/audio stack.