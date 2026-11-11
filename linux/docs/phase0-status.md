# Phase 0 status

Snapshot of initial bring-up work for native Linux (Wayland) target (plan refactored after external review).

## Done
- Created and refactored **`assessment.md`**:
  - clarified Level 1 (native Linux app + compatible D3D8 behavior, no Wine/Proton) vs Level 2 (fully native renderer),
  - adopted SDL3 as preferred initial window/input abstraction, null-audio-first staging, XDG registry strategy,
  - added measurable milestones (M0–M7), testing/behavior-comparison goals, 32-bit dependency validation, dependency-graph milestone, and source-separation guidance.
- Normalized data directory layout: `DATA/` is the canonical real directory at repo root; `data -> DATA` compatibility symlink exists.
- Added Linux bring-up scaffolding: `linux/README.md`, `linux/CMakeLists.txt`, `linux/docs/`.
- **#3 Path normalization (prototyped + refined):**
  - `linux/docs/path-normalization.md` updated with deterministic/ASCII-only policy, collision handling, cache invalidation, and planned tests.
  - `linux/include/linux_port/path_util.h`, `linux/src/path_util.cpp` refined (Gemini review follow-ups applied): locale-independent ASCII lowercasing, deterministic sorted directory caching with reduced lock contention, cache-first component resolution (avoid per-component `stat()` misses), single-pass `allow_missing_tail` handling, and drive-letter prefix stripping for bring-up compatibility.
  - `linux/tests/path_resolve_main.cpp` + CMake target `fl_path_resolve` for manual validation.

## Known immediate normalization items
- `bin/freelancer.ini`: `data path = ..\\DATA` (backslashes + Windows-style relative path); runtime integration needs normalization/resolution and compatible working-directory assumptions.
- Mixed-case asset/config lookups and `DACOM.INI` Windows module names/`DllPath` require VFS/module-mapping integration later.
- Resolver path support is intentionally scoped (game data/config paths), not full Win32 path emulation.

## Build/environment follow-up (Phase 0 bring-up scope closed)
- DACOM/FDUMP-aligned logging support added in `linux_port/platform` (`fdump` + `FDUMP*` compatibility macros, timestamp/thread-id logging retained).
- 32-bit compile/link and **32-bit runtime execution** validated in `linux/build-x86` on this host.
- **#4 graphics de-risk spike (completed for Phase 0 deliverables):** `gfx_env_probe` and `gfx_wayland_poc` implemented (SDL3+Vulkan; swapchain + 120-frame clear-color present) with CMake arch-aware dependency probing and `linux/docs/irenderpipeline-rp8-scope.md` findings. 64-bit Wayland/RADV validation on `AMD Radeon RX 7900 XTX (RADV NAVI31)` succeeded (`presented frames: 120/120`). 32-bit PoC build/run is pending **external host installation** of 32-bit SDL3 dev/runtime (Vulkan/Wayland 32-bit components observed); in-repo implementation/scaffolding is complete.
- **#1/#1.2 upstream integration (completed for bring-up scope):** 32-bit guarded upstream DACOM/ReadFile wrapper TUs compile/link/run with real file-backed config/IO integration and explicit registration hooks (`upstream_dacom_integration_harness`, `upstream_dacom_readfile_integration_harness`, `RC=0`). Runtime behavior remains partial/stub-backed and tracked as post-Phase-0 follow-on work.

## Core modules inspected (early)
- `src/dacom/dacom.cpp` (~2.8k LOC): DACOM manager/config/library loading, Win32 module APIs, `DllMain`, `HMODULE`.
- `src/ReadFile/ReadFile.cpp` (~3.5k LOC): filesystem/search-path core, Win32-oriented APIs/patterns, central to portability.

## #2 Config/DACOM parsing probe (completed for Phase 0 scope)
- Added **`linux/tests/dacom_ini_probe.cpp`** and CMake target **`dacom_ini_probe`** (builds under `linux/build`, links `fl_linux_port_path`).
- Added **`linux/docs/dacom-config-probe.md`** documenting purpose, usage, reports, and limitations.
- Verified against `bin/DACOM.INI` (+ auto-detected `bin/freelancer.ini`).
- Probe provides:
  - **DACOM.INI boot views:** sections/counts and `[DACOM]`, `[Libraries]` (14, ordered), `[System]`, `[Engine]`, `[RenderManager]`, `[RenderPipeline]`.
  - **Linux boot report:** load order → candidate module names/classes/`src/` hints, component→provider hints, suggested Tier 0–3 boot subset.
  - **Path/module resolution checks:** `DllPath`/`MGSDB` normalized+resolved; runtime binary presence checks for `[Libraries]`.
  - **`.vcxproj` dependency hints:** `ProjectReference` + `AdditionalDependencies` for `[Libraries]` modules and key runtime chains (`freelancer`, `rpclocal`, `Content`, `Server`, `DALib`, `Common`).
  - **Parse notes:** basic duplicate-section observations (none in current `DACOM.INI`).
  - **`freelancer.ini` checks:** resolved `data path` (`..\DATA` → repo `DATA/`), `local_server`, `[Initial SP/MP DLLs]`, `[Resources]` presence, `[Startup]` movie resolution, `[Data]` mappings validation (sampled 20 of 95, 95 resolved in checkout).
- Notes: parser/mappings/deps are simplified/heuristic (sufficient for boot planning); authoritative behavior remains in `src/dacom/dacom.cpp` and project/build files. Incremental fidelity can continue during #1 integration.

## #1 Platform abstraction/shim skeleton / #1.2 integration (completed for bring-up scope)
- Added **`linux/cmake/toolchain-linux-x86.cmake`** (i686 `-m32` CMake toolchain skeleton; includes `-msse2 -mfpmath=sse` for FP consistency).
- Added **`linux/include/linux_port/windows.h`** compatibility facade, expanded Win32 shims (compat/threads/sync/time/string/file/debug/registry/module/platform/path), and Linux-backed file IO shim (`linux/src/win32_file_shim.cpp`, path-normalized/case-insensitive, basic `mmap` view support).
- Added compile/integration probes and harnesses: `dacom_upstream_compile_probe`, guarded upstream wrapper TUs (`fl_upstream_dacom_tu`, `fl_upstream_readfile_tu`), `upstream_dacom_readfile_harness`, `upstream_dacom_integration_harness`, and `upstream_dacom_readfile_integration_harness` (32-bit `linux/build-x86`).
- **Upstream integration validated (32-bit):** upstream `src/dacom/dacom.cpp` and `src/ReadFile/ReadFile.cpp` compile in guarded wrapper TUs; DACOM config parsing/manager APIs exercised with real `bin/DACOM.INI` and bridged FDUMP diagnostics; explicit ReadFile registration hook and real file-backed reads (`bin/DACOM.INI`, `DATA\\solar\\solararch.ini`) succeed in integration harness. Remaining runtime behavior is stub/compat-limited and tracked as post-#1.2 follow-on work.
- Added/expanded shims, libs, and docs:
  - `linux/include/linux_port/win32_compat.h` (minimal Win32 types/result macros, `GUID` ops/COM aliases, calling-convention mappings/placeholders)
  - `linux/include/linux_port/platform.h`, `linux/src/platform.cpp` (logging/debug-output hooks)
  - `linux/include/linux_port/module_loader.h`, `linux/src/module_loader.cpp` (`dlopen`/`dlsym` skeleton with `path_util`-based resolution, `.dll`/no-ext/`.so` candidate mapping fixes, thread-local `dlerror` capture)
  - `linux/include/linux_port/win32_threads.h` (`CRITICAL_SECTION` stub mapping)
  - `linux/include/linux_port/win32_time.h` (`GetTickCount` stub)
  - `linux/include/linux_port/win32_file.h` (FILETIME/WIN32_FIND_DATAA/constants skeleton)
  - `linux/include/linux_port/win32_registry.h` (HKEY/registry stubs placeholders)
  - `linux/docs/platform-shim-skeleton.md`, `linux/docs/win32-shim-notes.md`, `linux/docs/dacom-readfile-harness-spike.md`
- Added CMake targets **`fl_linux_port_platform`**, **`core_shim_harness`**, **`abi_module_harness`**, and **`dacom_boot_harness`**. Full `linux/build` targets compile and harnesses run successfully.
- **Gemini re-review fixes applied (post-feedback):** `path_util` exact-match preference preserved under case-collision cache entries, cache lookup encapsulated (no leaked refs), drive-letter normalization semantics documented; `module_loader` error capture/path mapping corrected; `win32_compat` GUID comparison/COM typedefs and GCC i386 `THISCALL` attribute added.

## #1 completion summary (Phase 0 scope)
- **Shim growth:** expanded `win32_compat` (GUID/COM aliases, calling-convention macros), added/extended `win32_file`, `win32_registry`, `win32_threads`, `win32_sync`, `win32_time`, `win32_string`; improved `module_loader` resolution/error handling and `path_util` behavior per review.
- **ABI/module init:** added `abi_module_harness` demonstrating explicit module-init/registration flow (DllMain-free), factory/component creation and QueryInterface-style interaction patterns as a Linux bring-up strategy model.
- **Deepened spike:** added `dacom_boot_harness` and `core_shim_harness` to validate shim+path+module primitives against real `bin/DACOM.INI`/`bin/freelancer.ini` and `DATA/` paths, with documented follow-on integration steps in `linux/docs/dacom-readfile-harness-spike.md`.

## Next (post-#1)
1. **Code review:** collect feedback on assessment, shims, probes/harnesses, and path/module strategies.
2. **#4 Graphics de-risk spike:** plan/implement standalone Wayland + D3D8-compat feasibility PoC (triangle, 32-bit, RADV/7900XTX) and in parallel evaluate minimal native `IRenderPipeline` backend scope (`src/include/render/IRenderPipeline.h`, `src/RP8/`).
3. **Port integration follow-ups:** move from skeletons to guarded harness/port TUs for selected `dacom`/`ReadFile` behaviors with ABI and behavior tests.

## Follow-ups for #3
- Add automated tests for resolver behavior (exact/case-mixed, separators, relative/absolute, `..`, missing tail, ambiguous collisions, symlinks, cache invalidation, drive-letter prefix handling).
- Add ambiguity/miss diagnostics hooks when integrating into engine-facing shims.
- Validate against real `bin/` and `DATA/` paths referenced by configs/assets.

## Follow-ups for #2
- Compare/align parser behavior with DACOM's built-in profile parser where boot-critical (duplicate section handling, key lookup, bare-entry semantics, includes if any).
- Produce a boot dependency/load-order report (libraries + system/engine composition) and map names for Linux (`.dll` → `.so`/static candidates).
- Cross-reference against `.vcxproj` dependency graph to identify minimal compilable boot subset.
