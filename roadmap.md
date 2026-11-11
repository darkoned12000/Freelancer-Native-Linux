# Roadmap — Freelancer Native Linux Port

This document tracks milestones and sub-steps for bringing up a native Linux (Wayland-first, no Wine/Proton) port of the Freelancer decomp codebase. It complements the high-level milestone table in [`README.md`](README.md), the technical assessment in [`assessment.md`](assessment.md), and current bring-up notes in [`linux/docs/phase0-status.md`](linux/docs/phase0-status.md).

**Status legend:** `[x]` done · `[~]` in progress / partial · `[ ]` not started

---

## Phase 0 — Bring-up foundation / de-risking (prerequisite track)

Goal: establish tooling, shims, harnesses, and feasibility evidence to reduce risk before full engine integration.

- [x] **Environment & repo setup**
  - [x] Clone/organize source for Linux work; keep `DATA/` canonical with `data -> DATA` compatibility symlink
  - [x] Add `linux/` scaffolding (CMake, toolchain options, docs, probes, harnesses)
  - [x] Validate 64-bit and 32-bit (`-m32`) builds and harness execution on host
- [x] **#3 Path normalization / case-insensitive resolution**
  - [x] `path_util` library (separator normalization, drive-letter handling, ASCII case-insensitive lookup, deterministic behavior, caching/thread-safety)
  - [x] `fl_path_resolve` CLI/probe and tests; resolve real config/data paths on case-sensitive FS
- [x] **#2 Config/boot & dependency probe**
  - [x] `dacom_ini_probe` for `bin/DACOM.INI` / `bin/freelancer.ini` parsing, boot/load-order hints, path checks, `.vcxproj` dependency hints
- [x] **#1 Platform abstraction / Win32 shim skeleton**
  - [x] Core shim headers/facade (`windows.h`, compat/types, file/registry/threads/sync/time/string/debug/io/path helpers)
  - [x] Platform logging/FDUMP-style diagnostics bridge, module loader abstractions
  - [x] Linux-backed Win32 file I/O shim (`CreateFileA`/`ReadFile`/mapping/find/etc.) with path normalization/case-insensitive resolution
  - [x] Shim/ABI/boot harnesses (`core_shim_harness`, `abi_module_harness`, `dacom_boot_harness`, `dacom_upstream_compile_probe`)
- [x] **#1.2 DACOM/ReadFile upstream integration (harness scope)**
  - [x] Guarded upstream wrapper TUs for `src/dacom/dacom.cpp` and `src/ReadFile/ReadFile.cpp` (32-bit target)
  - [x] Explicit registration/init hooks (DllMain-free) and FDUMP/log bridge integration
  - [x] Integration harnesses with real config/file-backed reads (`upstream_dacom_integration_harness`, `upstream_dacom_readfile_integration_harness`, `upstream_dacom_readfile_harness`)
  - [ ] Deepen init/load-order, component/factory/descriptor wiring, and behavior-faithful stub replacements (post-Phase-0 / M1-M2 follow-on)
- [x] **#4 Graphics de-risk spike (PoC + scope findings)**
  - [x] `gfx_env_probe` and SDL3+Vulkan `gfx_wayland_poc` (Wayland-preferred window, instance/surface, swapchain, 120-frame clear-color present)
  - [x] 64-bit and 32-bit presentation validation on Wayland/RADV (host-dependent deps for 32-bit SDL3/Vulkan/Wayland runtime and pkg-config resolution)
  - [x] Initial `IRenderPipeline`/`RP8` scope and feasibility notes (`linux/docs/irenderpipeline-rp8-scope.md`)
  - [ ] Trace early-boot `IRenderPipeline` usage to define minimal M5 implementation subset (post-Phase-0)

**Phase 0 exit state:** probes/harnesses/PoCs build and run to demonstrate path/config/shim/upstream-wrapper integration and native presentation feasibility; full engine boot and game runtime are not yet implemented.

---

## M0 — Project foundation

**Acceptance target:** CMake+Ninja Linux build scaffolding, 32-bit toolchain validation, source/build organization, dependency graph from `.vcxproj`.

- [x] Linux build infrastructure (CMake, 32-bit toolchain flags/multilib validation, warnings) for bring-up targets
  - [x] Dedicated CMake toolchain files for i686/x86_64 targets (`linux/cmake/toolchain-linux-x86.cmake`, `linux/cmake/toolchain-linux-x86_64.cmake`)
  - [x] Ninja presets / developer workflow polish (`linux/CMakePresets.json`: `linux-x64`, `linux-x86`)
  - [x] 64-bit and 32-bit build/harness validation documented (`linux/docs/build-and-env-status.md`, `linux/docs/m0-project-foundation.md`)
- [x] Linux source/build organization (`linux/` bring-up layer, docs, tooling) while preserving upstream Windows build
- [x] Generate/maintain dependency graph from `src/*.vcxproj` / solution
  - [x] Generator: `linux/tools/generate_vcxproj_dep_graph.py`
  - [x] Artifacts: `linux/docs/vcxproj-dependency-graph.json`, `linux/docs/vcxproj-dependency-graph.md`
- [x] Define and document minimal boot subset/module order for first executable bring-up (`linux/docs/minimal-boot-subset.md`)

## M1 — Core platform compatibility

**Acceptance target:** `DACOM` and `ReadFile` compile (or run in harnessed form) and pass standalone tests for targeted behavior.

- [x] Filesystem/path normalization integration into engine-facing call paths (beyond harness/probe)
  - [x] Engine-facing Win32/MSVCRT path APIs improved to normalize separators/drive prefixes and resolve case-insensitively on Linux (`GetFullPathNameA`, `_fullpath`, `SearchPathA`, `GetCurrentDirectoryA`/`SetCurrentDirectoryA`, `_access`, `_mkdir`) while preserving existing Linux-backed file I/O shim behavior
  - [x] Path resolution behavior exercised for edge cases (backslashes, mixed case, `..`/`.`, drive-prefix stripping, missing-tail handling, cache reset)
- [x] Win32 compatibility shims (types, calling conventions/COM macros as needed, APIs) refined for behavior fidelity
  - [x] Added/refined common primitives used by upstream code paths (`Interlocked*`, `GetTickCount64`/`timeGetTime`/`SleepEx`, `DosDateTimeToFileTime`, directory/path helpers) and improved stub behavior where needed
- [x] Logging/diagnostics and error mapping aligned with expected engine behavior
  - [x] FDUMP/platform logging bridge retained; file/path shim operations continue to set/report `GetLastError`/errno-derived Win32-style failure codes for common cases
- [x] Timing primitives (`QueryPerformance*`, ticks, sleeps) with required semantics
  - [x] Monotonic steady-clock-backed `GetTickCount`/`GetTickCount64`/`timeGetTime`, `Sleep`/`SleepEx` (non-alertable wait completion), and `QueryPerformanceCounter`/`QueryPerformanceFrequency`
- [x] Threading/synchronization primitives (behavior-faithful where required)
  - [x] Recursive critical sections, basic event create/set/reset/wait/close semantics, and unified `CloseHandle` handling for event handles (sufficient for current bring-up scope)
- [x] Dynamic module loading strategy (static-link/explicit init now; `.so`/`dlopen`/`dlsym` + registration/init later as needed)
  - [x] Case-insensitive module path resolution with `.dll`/no-ext/`.so` candidate mapping, `dlopen`/`dlsym`/`dlclose` wrappers with last-error reporting; harness diagnostics clarified for source-only checkouts (no runtime module binaries)
- [x] `DACOM`/`ReadFile` harnessed compile/run with targeted tests
  - [x] Existing 32-bit upstream DACOM/ReadFile integration harnesses continue to compile/run (`upstream_dacom_readfile_integration_harness` overall ok in source-only checkout with explicit registration and empty `[Libraries]` smoke config)
  - [x] Added `m1_platform_tests` covering path edge cases, file I/O + find/wildcard behavior, config variant/failure-mode reads, timing/synchronization/interlocked primitives, module resolution candidates, and ABI-sensitive layout checks (validated in 64-bit and 32-bit builds)

## M2 — DACOM and engine initialization

**Acceptance target:** Linux harness/executable initializes the component system and loads core components successfully.

- [x] Port/adapt DACOM component manager/module initialization model for Linux (DllMain-free, explicit registration/lifecycle)
  - [x] Upstream DACOM wrapper/harness flow uses explicit init hooks in place of DllMain side effects (FDUMP bridge install, explicit registration of DACOM built-ins normally registered at attach, explicit ReadFile registration)
- [x] Preserve component registration/composition semantics and interfaces
  - [x] DACOM `CreateInstance`/registration/composition behavior exercised via built-in component creation/use (e.g. `RegExp`) and core module registration paths
- [x] Config/INI loading through Linux-compatible paths (`bin/DACOM.INI`, `freelancer.ini`)
  - [x] Resolve/load `bin/DACOM.INI` and `bin/freelancer.ini` through normalized/case-insensitive Linux paths; derive/resolve `data path` and perform file-backed reads via shimmed APIs
  - [x] Source-only explicit-registration init uses effective DACOM config handling to avoid incorrect auto-load attempts for unavailable external runtime module binaries while preserving config parsing/init intent
- [x] Load and initialize minimal core component set in dependency order
  - [x] Dependency-ordered explicit init: DACOM manager acquisition/built-ins -> config load -> ReadFile module registration -> core component creation/probe
- [x] Harness/executable proving component system init and core component load
  - [x] Added 32-bit `m2_engine_init_harness` (`linux/tests/m2_engine_init_harness.cpp`) demonstrating DACOM init, config loading, explicit core component registration/load, and Linux-compatible config/file access checks (PASS in source-only checkout)

## M3 — Math and game data

**Acceptance target:** Core data/config load without Windows APIs; path resolution behaves correctly on case-sensitive layout.

- [x] Port/isolate `x86math` (handle MSVC/x86 asm and ABI-sensitive behavior carefully; validate numerics)
  - [x] Added portable Linux bring-up implementation of 3DMathEngine/DACOM registration contract (`linux/src/x86math_portable.cpp`, `linux/tests/upstream_x86math_tu.cpp`) to isolate MSVC/x86 asm/ABI-sensitive upstream behavior behind an explicit, harness-validated path
  - [x] Numeric/behavior validation via `m3_math_data_tests` (matrix/transform/quaternion/inverse/sqrt-invsqrt checks)
- [x] Integrate path resolution into data/resource loading flows
  - [x] Case-insensitive/normalized path resolution used for config and `DATA/` asset access in harnessed load flows; `FileSystem/DOS` instantiated against resolved data root
- [x] Load core Freelancer configuration and game data from `DATA/` on Linux
  - [x] `m3_math_data_tests` resolves/reads `bin/DACOM.INI`, `bin/freelancer.ini`, and core game data files from `DATA/` (e.g. `solar\\solararch.ini`, `UNIVERSE\\universe.ini`, including BINI content) via Linux-compatible paths
- [x] Behavior/consistency checks (spot comparisons vs expected values/Windows behavior where feasible)
  - [x] Config/content spot checks and resolved-path existence/consistency checks; missing-path failure behavior verified (accurate diagnostics preserved)

## M4 — Windowing and input

**Acceptance target:** Native Linux window created with functioning input/event loop.

- [ ] SDL3-based Wayland-first window/event abstraction (with adapters for engine expectations; avoid fake-`HWND` assumptions)
- [ ] Keyboard/mouse/controller mapping to expected input semantics
- [ ] Focus/lifecycle, resize, cursor/grab handling as required by engine
- [ ] Integrate event loop with engine/frame/update flow

## M5 — Graphics bring-up (highest technical risk)

**Acceptance target:** A Freelancer scene/frame (or minimal in-engine render target) can render and present.

- [x] Standalone SDL3+Vulkan presentation feasibility PoC (Phase 0 de-risk)
- [ ] Trace and define minimal `IRenderPipeline`/buffer/texture/state/draw subset required for early boot/menu
- [ ] Choose and implement path per evidence: D3D8-compat integration in native ELF x86 **or** scoped native backend behind `IRenderPipeline`/related contracts
- [ ] Integrate minimal render path into engine initialization/frame loop
- [ ] Render/present a minimal in-engine target/frame

## M6 — Gameplay bring-up

**Acceptance target:** Reach main menu and enter the game world.

- [ ] Null-audio backend (early unblock), then real audio backend integration
- [ ] XDG-backed registry/config/save shim and persistence behavior
- [ ] Local/server/single-player flow (`rpclocal`/related) as required for boot
- [ ] UI/input wiring and mission/game initialization flow
- [ ] Reach main menu
- [ ] Enter game world (target loop milestone)

## M7 — Native Linux release candidate

**Acceptance target:** Playable native Linux experience on a clean system with provided game data, without Wine/Proton.

- [ ] Stability and crash/hang fixes across representative gameplay scenarios
- [ ] Performance profiling and fixes (CPU/GPU/frame pacing)
- [ ] Audio correctness/latency/mixing verification
- [ ] Display/input modes (fullscreen/windowed, controller support as applicable)
- [ ] Packaging/install/run documentation and dependency guidance
- [ ] Final compatibility and regression verification

---

## Cross-cutting tracks (ongoing)

- [ ] **Testing/validation:** expand harness/unit/integration coverage, data/config fixtures, and Windows-vs-Linux behavior comparisons for critical systems
- [ ] **ABI/layout safety:** verify 32-bit struct sizes, calling conventions, vtable/interface assumptions where upstream contracts require it
- [ ] **Documentation:** keep `README.md`, `assessment.md`, `linux/docs/*`, and this roadmap updated as milestones progress
- [ ] **Upstream parity:** keep changes behavior-preserving and guarded to avoid regressing the Windows reference build

## References

- [`README.md`](README.md) — goals, current status, build/run instructions
- [`assessment.md`](assessment.md) — technical assessment, subsystem analysis, risks
- [`linux/docs/phase0-status.md`](linux/docs/phase0-status.md) — Phase 0 detailed status and follow-ons
- [`linux/docs/build-and-env-status.md`](linux/docs/build-and-env-status.md) — build/runtime environment validation
- [`linux/docs/irenderpipeline-rp8-scope.md`](linux/docs/irenderpipeline-rp8-scope.md) — renderer scope findings
