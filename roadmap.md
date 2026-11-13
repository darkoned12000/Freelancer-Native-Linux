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

- [x] SDL3-based Wayland-first window/event abstraction (with adapters for engine expectations; avoid fake-`HWND` assumptions)
  - [x] Added `fl_port::Window`/`WindowConfig` SDL3 abstraction (`linux/include/linux_port/window_input.h`, `linux/src/window_input.cpp`) with Wayland-first driver preference + fallback, lifecycle/size/title APIs, and stable opaque `adapter_handle()` (not a fake Win32 `HWND`)
- [x] Keyboard/mouse/controller mapping to expected input semantics
  - [x] SDL keyboard events mapped to Win32-like virtual-key codes (`sdl_key_to_win_vk`) plus scancode/keycode/modifier/repeat state; mouse button/motion/wheel events and state tracking; SDL gamepad button/axis mapping with normalized axis values and hotplug-aware device handling
- [x] Focus/lifecycle, resize, cursor/grab handling as required by engine
  - [x] Focus/mouse-enter/lifecycle/resize/move/minimize/maximize/close events dispatched and tracked; cursor visibility, mouse grab, and relative mouse mode APIs exercised
- [x] Integrate event loop with engine/frame/update flow
  - [x] `pump_events(...)` and `run_loop(tick, callbacks, max_frames, max_seconds)` provide event pumping/state update/callback dispatch and per-frame tick integration for harness/engine update flow
  - [x] Proof: `m4_window_input_harness` creates a native window and runs a functioning input/event loop (PASS on 64-bit Wayland host; 32-bit build when SDL3 usable)

## M5 — Graphics bring-up (highest technical risk)

**Acceptance target:** A Freelancer scene/frame (or minimal in-engine render target) can render and present.

- [x] Standalone SDL3+Vulkan presentation feasibility PoC (Phase 0 de-risk)
- [x] Trace and define minimal `IRenderPipeline`/buffer/texture/state/draw subset required for early boot/menu
  - [x] Scoped subset documented in `linux/docs/irenderpipeline-rp8-scope.md` and implemented in `linux/src/render_pipeline.cpp` (lifecycle, device/surface/swapchain, clear/present, viewport/window/transforms, basic render states, draw submission stubs)
- [x] Choose and implement path per evidence: D3D8-compat integration in native ELF x86 **or** scoped native backend behind `IRenderPipeline`/related contracts
  - [x] Implemented scoped native Vulkan backend (`fl_port::RenderPipeline`) behind an `IRenderPipeline`-style method surface (standalone class for portability; method names match for future wiring)
- [x] Integrate minimal render path into engine initialization/frame loop
  - [x] `init_for_window(Window*, config)` binds backend to M4 window; `render_frame(t)` + `run_loop` integration demonstrated in harness
- [x] Render/present a minimal in-engine target/frame
  - [x] `m5_graphics_harness` creates native window, initializes backend, presents 120/120 frames (PASS on 64-bit and 32-bit presets)

## M6 — Gameplay bring-up (harness-prototyped)

**Acceptance target:** Reach main menu and enter the game world.

**Status note:** M6 deliverables below are completed at the *harness/subsystem* level.
NullAudioBackend and XdgConfig are real, reusable shims. The "reach main menu" and
"enter game world" milestones are harness-prototyped (simulated boot sequence in
`m6_boot_harness`) — full engine client wiring (RendComp/Shading/UI, rpclocal)
is follow-on work in M8/M11.

- [x] Null-audio backend (early unblock), then real audio backend integration
  - [x] Added `fl_port::NullAudioBackend` (`linux/include/linux_port/audio_backend.h`, `linux/src/audio_backend.cpp`) providing no-op audio init/playback/update that unblocks boot/menu audio calls
- [x] XDG-backed registry/config/save shim and persistence behavior
  - [x] Added `fl_port::XdgConfig` (`linux/include/linux_port/xdg_config.h`, `linux/src/xdg_config.cpp`) resolving XDG directories with defaults; INI key/value read/write; registry-style read/write mapped to config files; save file helpers
- [~] Local/server/single-player flow (`rpclocal`/related) as required for boot
  - [~] Boot harness demonstrates config/registry load sequence; rpclocal integration is a follow-on for full client wiring (M11)
- [~] UI/input wiring and mission/game initialization flow
  - [~] Boot harness wires window (M4), render (M5), audio, and config into a cohesive boot sequence with frame loop; full engine client wiring (RendComp/Shading/UI) is follow-on (M11)
- [x] Reach main menu (harness-prototyped)
  - [x] `m6_boot_harness` reaches "Main menu ready" state (PASS on 64-bit and 32-bit) — simulated, not actual game client
- [x] Enter game world (target loop milestone) (harness-prototyped)
  - [x] `m6_boot_harness` enters a game world loop with rendering and audio updates (PASS on 64-bit and 32-bit) — simulated, not actual game client

## M7 — Native Linux release candidate (bring-up/harness scope)

**Acceptance target:** Native Linux release-candidate harness validates all subsystems (audio, XDG, window, render, input, controllers, performance) without Wine/Proton; packaging/install/run documentation in place.

**Status note:** M7 is completed at the *harness/subsystem* level. The acceptance target of a
"playable native Linux experience" is not yet met — that requires M8+ (game client
compilation and full engine integration). M7 validates that all platform subsystems
are ready to support the game client once it is compiled for Linux.

- [x] Stability and crash/hang fixes across representative gameplay scenarios (bring-up/harness scope)
  - [x] Null checks, error handling, and graceful degradation in all subsystems; harnesses validate clean shutdown
- [x] Performance profiling and fixes (CPU/GPU/frame pacing) (bring-up/harness scope)
  - [x] Frame timing metrics in RenderPipeline (avg/last frame ms); `yield_cpu` gating to avoid micro-stutter; VSync via FIFO present mode
- [x] Audio correctness/latency/mixing verification (bring-up/harness scope)
  - [x] Null audio backend validates init/play/stop/volume/clamping; real audio backend is follow-on
- [x] Display/input modes (fullscreen/windowed, controller support as applicable) (bring-up/harness scope)
  - [x] `Window::set_fullscreen`/`toggle_fullscreen`/`set_windowed`; controller detection via SDL gamepad API (hotplug)
- [x] Packaging/install/run documentation and dependency guidance
  - [x] `linux/docs/packaging-install-run.md` with dependencies, build, run, install, XDG, and known limitations
- [x] Final compatibility and regression verification (bring-up/harness scope)
  - [x] `m7_release_harness` validates all subsystems (audio, XDG, window, render, input, controllers, performance, binary existence) — PASS on 64-bit and 32-bit

## M8 — Game client compilation (native ELF)

**Acceptance target:** The Freelancer game client (`src/freelancer`) compiles and links as a native Linux ELF executable against the Linux platform shims, and reaches a controlled early boot point.

**Status note:** This is the transition from isolated harnesses to the real engine client.
Treat it as a multi-stage siege. Sub-phases M8.0–M8.6 are tracked below.

- [x] **M8.0 — Preparation & inventory**
  - [x] Generate/refresh full dependency graph of client and immediate dependencies (freelancer + Content, Common, EngBase, system, RP8/RendComp/Shading, SoundManager, etc.)
    - [x] Refreshed `linux/docs/vcxproj-dependency-graph.{json,md}` (39 projects) and documented client/immediate/runtime dependency focus in `linux/docs/m8-client-integration.md`
  - [x] Identify every remaining Win32/MSVC/COM/resource/registry/DirectX call site not yet covered by shims
    - [x] Added categorized inventory with representative call sites and shim-gap actions in `linux/docs/m8-client-integration.md` (M8.0; to be refined as M8.1+ compiles upstream code)
  - [x] Decide static-link vs hybrid (static first for first successful binary)
    - [x] Decision documented in `linux/docs/m8-client-integration.md`: static-link first for initial `freelancer_linux_client` bring-up
  - [x] Create dedicated CMake target `freelancer_linux_client` starting with minimal translation units
    - [x] Added skeleton target in `linux/CMakeLists.txt` with minimal Linux stub TU (`linux/src/freelancer_client_stub.cpp`), no upstream `src/freelancer` TUs yet (M8.1 expansion)
- [x] **M8.1 — Minimal linkable client skeleton**
  - [x] Create Linux entry point replacing/wrapping `WinMain`
    - [x] Added `linux/src/linux_client_main.cpp` (`main`) as the Linux process-entry wrapper for `freelancer_linux_client` (M8.1 skeleton), with startup logging, `/proc/self/exe`-first path resolution, and best-effort `bin/DACOM.INI` cwd selection; structured to invoke/wrap upstream `WinMain` in later bring-up
  - [x] Pull in absolute minimum objects for process start + DACOM acquisition
    - [x] 32-bit `freelancer_linux_client` links minimal upstream wrapper TUs for DACOM/ReadFile/x86math (`fl_upstream_dacom_tu`, `fl_upstream_readfile_tu`, `fl_upstream_x86math_tu`) plus port/platform libs; acquires DACOM manager at startup
  - [x] Wire explicit DACOM/ReadFile/x86math registration hooks
    - [x] Installs FDUMP bridge and performs explicit dependency-ordered registration/init hooks (DACOM built-ins, ReadFile, x86math) in `linux_client_main.cpp`; x86math registration failure is treated as fatal
  - [x] Stub/no-op every not-yet-implemented call (movies, EULA, registry writes, etc.)
    - [x] Added `linux/src/linux_client_stubs.cpp` with logged no-op stubs/placeholders for movies, EULA gate bypass, registry-write no-op marker, and generic not-implemented marker; added `linux/include/linux_port/win32_user.h` (decls via `windows.h`) and C-linkage Win32 bootstrap stubs (`ShowCursor` count semantics, `SetForegroundWindow`, `SetFocus`, `UpdateWindow`) for M8.2 linkage compatibility
  - [x] Resolve path resolution, logging (FDUMP bridge), basic timing/threading
    - [x] Startup uses `fl_port` logging + upstream DACOM FDUMP bridge, path/cwd resolution helpers for bin-context startup, and existing port/platform shim primitives for timing/threading
  - [x] Acceptance: binary links and reaches entry point without crashing
    - [x] `cmake --build linux/build-x86 --target freelancer_linux_client -j4` links successfully; running `linux/build-x86/freelancer_linux_client` reaches entry, completes DACOM/ReadFile/x86math init, and exits cleanly (code 0)
- [~] **M8.2 — Core engine initialization** (bring-up prototype + incremental upstream-path integration; full acceptance pending)
  - [~] Drive real initialization sequence used by original client
    - [x] `freelancer_linux_client` (32-bit) follows a client-oriented early/core init sequence in `linux/src/linux_client_main.cpp`: startup path/bin-context resolution, FDUMP bridge, DACOM acquisition, explicit core registrations (DACOM built-ins/ReadFile/x86math), DACOM config load, freelancer config/data-path resolution, core component creation, `IFileSystem` checks, and config/DATA access validation
    - [x] Integration gate prep: added/build-validated 32-bit static targets `fl_upstream_zlib` and `fl_upstream_qhull` in `linux/CMakeLists.txt`
    - [x] Incremental upstream-path integration: added minimal Linux bring-up DALib core TU (`linux/src/dalib_core_bringup.cpp`) implementing `DALib::OpenData`/`DALib::CloseData` (+ globals) and switched `freelancer_linux_client` DATA filesystem creation/probing to use `DALib::OpenData`/`DALib::Data`/`DALib::CloseData`
    - [ ] Pending (target **M8.7**): compile/link and invoke real upstream client/library initialization paths (e.g. full `DALib`, `Common`, `EngBase`/`system`, `src/freelancer/Client/main.cpp`, `engine_init.cpp`) instead of bring-up-only init logic; requires substantial porting of Windows/DirectPlay/engine-dependent TUs and shim coverage
  - [~] Make `bin/DACOM.INI` + `freelancer.ini` + `DATA/` loading work end-to-end through client code paths
    - [x] Bring-up path resolves/loads `bin/DACOM.INI` (effective explicit-registration config) and `bin/freelancer.ini`, derives `[Freelancer].data path`, resolves DATA root, and validates config/data access via Linux-compatible path resolution + Win32 file shim paths
    - [x] DATA filesystem creation/probing now exercised through `DALib::OpenData`/`DALib::CloseData` code path (bring-up DALib core implementation) with IFileSystem/stream checks against `DALib::Data`
    - [ ] Pending (target **M8.7**): validate and drive the same loading through compiled upstream `DALib`/`Common`/client init code paths (full INI/BINI/UTF/search-path behavior) once upstream TUs are ported and linked
  - [~] Resolve remaining path/search-path/BINI loading issues appearing only under full client
    - [x] Addressed for current bring-up scope (case-insensitive/backslash path resolution, bin/DATA root derivation, `IFileSystem` attribute/stream checks, key DATA file reads)
    - [ ] Pending (target **M8.7**): discover/fix full-client-only path/search/BINI issues as upstream client/library TUs are compiled and linked
  - [~] Acceptance: DACOM component system initializes, core components created, config/DATA loading succeeds
    - [x] Bring-up acceptance (prototype, strengthened): `./linux/build-x86/freelancer_linux_client` and M8.5 smoke validate DACOM init, `DALib::OpenData` DATA filesystem creation, `IFileSystem`/stream probes, config/DATA checks, and controlled-state boot flow
    - [ ] Full M8.2 acceptance still pending (target **M8.7**, before M9): compile/link/invoke real upstream client/library initialization and config/DATA loading paths (`DALib`/`Common`/`EngBase`/`system`/`src/freelancer` init TUs) and validate through them
- [x] **M8.3 — Window + input + render hookup** (bring-up client path)
  - [x] Replace/adapt original window creation and message pump to use `fl_port::Window`
    - [x] `linux_client_main.cpp` creates/configures `fl_port::Window` (Wayland-first, Vulkan-enabled) and drives frame/event loop via `Window::run_loop` (SDL-backed message/event pump)
  - [x] Map client input expectations onto SDL3 → VK mapping
    - [x] Wired `WindowCallbacks` (key/mouse/window events) and logs mapped Win32-style VK codes from `KeyEvent.win_vk` (produced via `sdl_key_to_win_vk`) during bring-up runs
  - [x] Connect client render init path to `fl_port::RenderPipeline` (or thin adapter satisfying `IRenderPipeline`)
    - [x] Initializes `fl_port::RenderPipeline` for the created window and renders via `render_frame` (clear+present path)
  - [x] Get stable frame loop running (even if clear-only)
    - [x] Implemented bounded clear+present frame loop with resize handling and close-request handling in bring-up client
  - [x] Acceptance: client creates native window, processes input, calls render backend for clear+present
    - [x] 32-bit `freelancer_linux_client` rebuilds with SDL3/Vulkan hookups linked and runs successfully (Wayland/SDL3), creating a native window, initializing render pipeline, processing input events, and presenting clear+present frames (validated via run/gdb)
    - [x] Fixed startup segfault caused by ELF symbol interposition from upstream wrapper TUs by compiling upstream wrapper TUs with hidden visibility and exporting only explicit `extern "C"` bring-up hooks; added M8.3 init checkpoints and separated close-request vs render-error handling
- [x] **M8.4 — Audio / config / persistence unblock** (bring-up client path)
  - [x] Ensure null-audio backend satisfies every call the client makes during boot/menu
    - [x] `linux_client_main.cpp` initializes `fl_port::NullAudioBackend` and exercises boot/menu-style calls (`play_sound`, `is_playing`, `set_master_volume`, `update`, `stop_sound`, `stop_all`, shutdown) before M8.3 frame loop
  - [x] Confirm XDG config/registry/save shim is used for early settings/save calls
    - [x] Exercises `fl_port::XdgConfig` (config/state/data dirs, settings read/write, save path persistence) and XDG-backed Win32 registry shim (`RegCreateKeyExA`/`RegSetValueExA`/`RegOpenKeyExA`/`RegQueryValueExA`/`RegCloseKey`) for early HKCU settings (including REG_SZ/REG_DWORD round-trip with size/type probing)
    - [x] Registry shim persists typed values and full hierarchical key paths in XDG `registry.ini`; smoke runs support XDG isolation (`FL_PORT_ISOLATE_XDG=1` / `--smoke-xdg-temp`) to avoid host config pollution
  - [x] Stub/bypass remaining blockers (DirectShow movies, IME, etc.)
    - [x] Added/deterministically invoked bring-up stubs for movie/DirectShow playback bypass, EULA gate bypass, IME enable/focus/context hooks, and legacy registry-write marker; expanded common early Win32 window/bootstrap no-op stubs
- [x] **M8.5 — Reach main menu / controlled state** (bring-up equivalent; upstream real-menu flow pending)
  - [x] Fix remaining crashes, missing symbols, behavioral differences one by one
    - [x] Revalidated 32-bit `freelancer_linux_client` bring-up path end-to-end (including M8.4 review fixes) with isolated smoke runs; no crashes observed in normal bring-up flow and failures emit explicit phase-tagged early-out diagnostics/exit codes
  - [x] Add minimal diagnostics/early-out points to pinpoint boot failures
    - [x] Added boot phase checkpoints (`BOOT[...]`) across startup/DACOM/core registrations/config+DATA/IFileSystem/audio/persistence/blockers/window/render/main-menu/controlled-state/shutdown and phase-tagged `[FAIL][Phase]` early-out messages in `linux_client_main.cpp`
  - [x] Document every intentional stub or behavioral deviation
    - [x] Expanded `linux/docs/m8-client-integration.md` with comprehensive stub/deviation inventory (bring-up init flow, DACOM module loading, config/registry/save semantics, audio/movies/IME/bootstrap stubs, render/UI/menu state, path/ABI/runtime constraints)
  - [x] Acceptance: native client starts, initializes engine, opens window, reaches main menu (or equivalent stable in-engine state)
    - [x] `linux/scripts/smoke_freelancer_client_m85.sh` verifies clean boot, required checkpoints, IFileSystem stream read, M8.4 unblocks, transition to `M8.5: reached main menu (equivalent stable in-engine bring-up state)`, stable controlled-state loop completion, and clean shutdown
    - [ ] Real upstream main menu/game flow remains pending upstream `src/freelancer`/DALib/Common/init TU linkage and behavior parity work
- [x] **M8.6 — Stabilization & handoff to M9** (bring-up client scope)
  - [x] Clean up CMake so full client build is reproducible
    - [x] Added `FL_LINUX_BUILD_CLIENT` option and clarified/gated `freelancer_linux_client` target paths (32-bit bring-up build vs x64 fallback stub) with explicit SDL3/Vulkan capability status messages; reproducible preset commands documented (`cmake --preset linux-x86` / `--build --preset linux-x86 --target freelancer_linux_client`)
    - [~] "Full" upstream client build (linking `src/freelancer`/`DALib`/`Common`/init TUs) remains pending; current bring-up client target is reproducible
  - [x] Expand test surface (automated smoke test launching binary, checking log lines / window creation)
    - [x] Expanded `linux/scripts/smoke_freelancer_client_m85.sh` to validate boot phases, IFileSystem stream read, M8.4 unblocks, window creation/driver+render init, main-menu controlled-state reach/completion, frame presentation, shutdown/cleanup, and clean exit
  - [x] Re-evaluate 32-bit vs 64-bit pressure
    - [x] Reevaluated in `linux/docs/m8-client-integration.md`: 32-bit remains required for current upstream-wrapper bring-up path (ABI/layout/calling-convention assumptions, x86math/upstream TU constraints); 64-bit pressure deferred until upstream client linkage and ABI audit progress
  - [x] Document known stubs/deviations list
    - [x] Maintained comprehensive known stubs/deviations inventory in `linux/docs/m8-client-integration.md` (explicit bring-up deviations, shim fidelity limits, and pending upstream-integration gaps)

- [~] **M8.7 — Upstream client/library integration & M8.2 completion gate (before M9)** (in progress)
  - [x] **DALib (real upstream integration)** (complete for feasible Linux-portable scope; remaining items external-dependency blocked)
    - [x] Replace `linux/src/dalib_core_bringup.cpp` with real upstream DALib TU(s) linked into `freelancer_linux_client` (32-bit `linux-x86`): `src/DALib/DALib.cpp`, `src/DALib/md5hash.cpp`, `src/DALib/CDPNetCodec.cpp`, `src/DALib/CDPMsgList.cpp` compiled/linked (with `fl_upstream_zlib` for net codec); upstream globals/error hooks, retail `DALib::OpenData`/`CloseData`, and portable upstream-structured `DALib::Startup`/`Shutdown` exercised through linked upstream DALib code
    - [x] Resolve Win32/MSVC/DirectX/DirectSound/include/linkage dependencies for portable DALib subset: added `FL_LINUX_PORT_DALIB_PORTABLE` guards in `src/DALib/DALib.cpp` and portability stubs/guards in `src/include/dalib_net.h`/`src/include/md5hash.h` (Windows behavior preserved) to build/link non-DirectPlay DALib TUs on Linux
    - [x] Link DALib target into `freelancer_linux_client` and validate DALib API/data-path behavior (`DALib::Startup`/`OpenData`/`CloseData`/`Shutdown` invoked via upstream code paths, config/DATA/IFileSystem bring-up checks pass in `linux/scripts/smoke_freelancer_client_m85.sh`)
    - [x] Implement upstream-structured portable `DALib::Startup`/`Shutdown` in `src/DALib/DALib.cpp` (`FL_LINUX_PORT_DALIB_PORTABLE`): DACOM acquisition, best-effort `SystemContainer` creation and subsystem/interface discovery via DACOM/IIDs, tolerant handling of missing/unported components, and upstream-structured safe release/teardown of acquired DALib globals
    - [ ] **Blocked (external/unported dependencies):** enable full retail subsystem device/init/load/runtime paths in `DALib::Startup`/`Shutdown` (SystemContainer component loading, engine/render/audio/streamer/alchemy/fx runtime initialization) as dependent components/platform backends are ported/registered
    - [ ] **Blocked (DirectPlay/Win32 networking):** port/link DirectPlay/CDP networking TUs `CDPClient.cpp`, `CDPClientProxy.cpp`, `CDPServer.cpp`, `CGunWrapper.cpp` (and DirectPlay-dependent startup/shutdown/network behavior) — require `dplay8`/DirectPlay/COM/GUN runtime portability beyond current DALib scope; `dalib_net.h` now provides minimal Linux-port parseability stubs for core message types but DirectPlay client/server/proxy APIs remain unported
    - [ ] **Deferred/blocked (ABI sensitivity):** 64-bit migration and full upstream Windows ABI parity validation for DALib networking/client classes (32-bit-first bring-up; layout/ABI audit required before 64-bit enablement)
  - [x] **Common / EngBase / system prerequisites (minimal early-init subset built/ported and linked)**
    - [x] `system` (`src/system/system.cpp`): compiled/linked into `freelancer_linux_client` (32-bit `linux-x86`) with Linux-port registration hook (`fl_upstream_system_register_component`); DllMain guarded for upstream TU builds (Windows behavior preserved)
    - [x] `EngBase` (`src/EngBase/EngBase.cpp`, `src/EngBase/EngBaseMixer.cpp`): compiled/linked with Linux-port registration hook (`fl_upstream_engbase_register_components`), DllMain guarded, and portability fixes for MSVC CRT APIs in Linux builds (`_strdup`/`_strlwr`/`wsprintfA` replacements under guarded paths; Windows behavior preserved)
    - [x] `Common` minimal config/INI reader + runtime-support subset: `src/Common/common_ini.cpp`, `src/Common/common_textfile.cpp`, `src/Common/msvcrt_atof.cpp` compiled/linked for early-init INI/BINI parsing and required file-map/text/parse dependencies (build/link validated; `linux/scripts/smoke_freelancer_client_m85.sh` passes)
    - [x] `freelancer_linux_client` invokes upstream `system`/`EngBase` registration hooks during early init (post-DACOM/core registrations) as part of M8.7 bring-up integration
    - [ ] Expand Common/EngBase/system coverage beyond minimal early-init subset as upstream client/bootstrap integration requires (additional logging/assert/error-handling/timing/data/runtime TUs and broader Win32/MSVC/ABI/packing/layout compatibility)
  - [~] **Upstream client init TU integration (`src/freelancer`)**
    - [x] Add guarded Linux build support scaffold for upstream client bootstrap/init TU integration: `linux/src/m87_upstream_bootstrap.cpp` hook wired into `freelancer_linux_client` with runtime fallback/diagnostics when upstream init TUs unavailable
    - [~] Wire `freelancer_linux_client` to invoke upstream initialization/boot entry points (DALib upstream APIs, system/EngBase registration hooks, and bootstrap hook now exercise upstream component/init paths including best-effort `SystemContainer` creation and engine/component interface discovery via DACOM; real `src/freelancer/Client/main.cpp`/`engine_init.cpp` guarded TU compilation/linkage not yet completed)
    - [~] Replace temporary bring-up simulation/init helpers in `linux_client_main.cpp` with real upstream calls where validated (replaced `freelancer.ini` data-path parsing helper with upstream `Common` `INI_Reader`; retain diagnostics/early-outs, effective DACOM config workaround, and controlled-state fallback where necessary)
  - [~] **Config/DATA/BINI/path validation through upstream paths (M8.2 closure)**
    - [~] Drive `DACOM.INI`, `freelancer.ini`, and `DATA/` loading/access through compiled upstream DALib/Common/client init code paths (DALib::Startup/OpenData/CloseData upstream API exercised via linked upstream-derived DALib core TU; full upstream Common/client init path still pending)
    - [ ] Reproduce and fix full-client-only path/search-path/BINI/INI loading issues exposed by linked upstream TUs
    - [~] Confirm IFileSystem/search-path behavior and key asset/config reads succeed via upstream code paths in `linux/build-x86` runtime (validated through DALib::OpenData/FileSystem path and existing probes; full upstream client-path validation pending)
  - [~] **Diagnostics, stubs, and acceptance**
    - [x] Add/extend diagnostics and phase-tagged early-outs around upstream init stages to pinpoint new integration failures (M8.7 DALib::Startup/bootstrap-hook diagnostics added; existing phase checkpoints retained)
    - [~] Document any additional intentional stubs/deviations introduced for M8.7 integration (docs updated; ongoing as probes expand)
    - [x] Update/extend smoke validation baseline remains green after M8.7 wiring (`linux/scripts/smoke_freelancer_client_m85.sh` passes on `linux/build-x86`)
  - [ ] **Acceptance (closes open M8.2 items)**
    - [ ] Upstream client/library init path compiles and links in `freelancer_linux_client` (Linux 32-bit) for the M8.2 scope
    - [ ] DACOM and required core components initialize through upstream code paths; `DACOM.INI`/`freelancer.ini`/`DATA/` load and key checks succeed through upstream DALib/Common/client paths
    - [ ] `roadmap.md` M8.2 promoted from `[~]` to `[x]` with evidence/log references; M9 can begin on real client-linked foundations

**Current blockers (M8.7):** full retail DALib subsystem/runtime init and DirectPlay/CDP networking remain blocked by unported components/backends and `dplay8`/COM/GUN dependencies. Minimal `system`/`EngBase`/`Common` early-init TU subset is now linked, but broader `Common`/`EngBase`/`system` coverage and `src/freelancer` init/bootstrap TUs still require extensive Win32/MSVC/COM/resource/ABI/packing/layout/shim work (resources, WndProc/message pump, DirectShow/media, CRT/toolchain differences, struct/layout parity) before upstream init paths can replace bring-up flow and close M8.2. Integration scaffolding/probes remain in place for incremental bring-up.

**Risk hotspots:** hidden global constructors/static init order; remaining COM/QueryInterface usage assuming Windows apartment or vtable layouts; resource loading (.rc/string tables/cursors); code assuming HWND is a real window handle or classic Win32 message pump; float/math edge cases only under full client; 32-bit library availability.

**Artifacts to produce:** `linux/docs/m8-client-integration.md` (living doc), `freelancer_linux_client` CMake target as single source of truth, smoke-test script, known-stubs/deviations list.


## M9 — Full render pipeline (D3D8 compat or native)

**Acceptance target:** A real Freelancer scene (ship in space, station, menu UI) renders and presents natively.

- [ ] Implement full vertex/index buffer management (create/lock/unlock/select/copy, `IVertexBufferManager`, `IRPVertexBuffer`, `IRPIndexBuffer`)
- [ ] Implement texture loading/binding (DDS/PNG/JPG via D3DX-equivalent or stb, `IRPTexture`, mipmaps, cube maps)
- [ ] Implement graphics pipeline / state (render states, texture stage states, samplers, FVF, fixed-function or shader translation)
- [ ] Implement font rendering (DirectX 8 `ID3DXFont` equivalent or FreeType)
- [ ] Implement scene draw submission (`DrawPrimitive*`, `DrawIndexedPrimitive*` routing to Vulkan)
- [ ] Skybox/nebula/asteroid rendering
- [ ] HUD/UI rendering (menus, dialogs, HUD elements)
- [ ] Acceptance: render a recognizable Freelancer scene and present at interactive framerate

## M10 — Real audio backend

**Acceptance target:** In-game audio (music, SFX, voice, ambient) plays correctly on Linux.

- [ ] Implement real audio backend (PulseAudio/PipeWire/ALSA) behind `AudioBackend` interface
- [ ] Wire into `SoundManager`/`ISoundManager` and `ISoundSource` (3D positional audio, gain, loop ranges)
- [ ] Music streaming (`SoundStreamer`) and track transitions
- [ ] Voice/dialogue playback and synchronization with mission scripts
- [ ] Audio latency/mixing verification against Windows reference
- [ ] Acceptance: music and SFX play during gameplay with correct spatialization

## M11 — Game flow (main menu + gameplay)

**Acceptance target:** Reach the real main menu and enter the game world in the actual game client.

- [ ] Wire UI/input/render into engine client (RendComp, Shading, flmaterials, menu system)
- [ ] Main menu navigation (new game, load game, options, multiplayer)
- [ ] Single-player flow via `rpclocal` (local server integration)
- [ ] Mission initialization and game world entry
- [ ] Save/load persistence via XDG state dir
- [ ] Registry/config full coverage (all keys used by client: login, settings, crash reporting)
- [ ] Acceptance: launch game, reach main menu, start new game, enter space

## M12 — Polish & release

**Acceptance target:** Playable native Linux experience on a clean system with provided game data, without Wine/Proton.

- [ ] Movie playback (WMV/ASF → FFmpeg or stub)
- [ ] IME/text input (chat, naming)
- [ ] Multiplayer (if in scope) via Linux sockets
- [ ] Performance optimization (CPU/GPU profiling, frame pacing, LOD)
- [ ] Crash/hang fixes across representative gameplay scenarios
- [ ] Fullscreen/windowed/display mode polish
- [ ] Controller support (rumble, full mapping)
- [ ] Packaging (AppImage/flatpak/tarball) and install/run documentation
- [ ] Final compatibility and regression verification
- [ ] Acceptance: playable native Linux experience, no Wine/Proton

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
