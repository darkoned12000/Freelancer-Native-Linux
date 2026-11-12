# M8 — Game client integration (living document)

**Status:** M8.6 completed for bring-up client stabilization/handoff (reproducible bring-up target build flow, expanded smoke validation, 32/64-bit reassessment, documented stubs/deviations). M8.5 remains a bring-up-equivalent controlled main-menu state. M8.2 has incremental upstream-path progress (`DALib::OpenData`/`CloseData` bring-up integration) but full acceptance remains pending; **M8.7** is planned to complete upstream client/library TU linkage and close all open M8.2 items before M9.  
**Acceptance target:** The Freelancer game client (`src/freelancer`) compiles and links as a native Linux ELF and reaches a controlled early boot point (M8.5).

## Overview

M8 is the transition from isolated harnesses to the real engine client. Everything before M8
(M0–M7) was bring-up infrastructure: build system, platform shims, DACOM init, path
resolution, window/input, graphics backend, audio/config shims, and validation harnesses.
M8 is where the actual port begins.

## Sub-phases

See `roadmap.md` M8 section for the full task breakdown. Summary:

- **M8.0** — Preparation & inventory: dependency graph, call-site inventory, CMake skeleton
- **M8.1** — Minimal linkable client skeleton: Linux entry point, stubs, link
- **M8.2** — Core engine initialization: DACOM + config/DATA through real client paths
- **M8.3** — Window + input + render hookup: SDL3 window, input mapping, Vulkan clear-present
- **M8.4** — Audio / config / persistence unblock
- **M8.5** — Reach main menu / controlled state
- **M8.6** — Stabilization & handoff to M9

## M8.0 — Preparation & inventory (completed deliverables)

### 1) Dependency graph refresh (client and immediate dependencies)

- Regenerated vcxproj reference/dependency graph artifacts:
  - `linux/docs/vcxproj-dependency-graph.json`
  - `linux/docs/vcxproj-dependency-graph.md`
  - Generator: `linux/tools/generate_vcxproj_dep_graph.py` (39 projects scanned)
- **Direct build refs for `freelancer` (vcxproj):** `Common`, `DALib`, `dacom`, `imeui` plus additional system libs in Windows build (`advapi32`, `gdi32`, `ole32`, `shell32`, `user32`, `winmm`).
- **Immediate/runtime dependency focus for client bring-up (per roadmap/task and boot hints):**
  - **Foundation:** `dacom`, `ReadFile` (INI `[Libraries]`/file providers), `x86math` (math engine, ABI-sensitive).
  - **Core libs/services:** `DALib` (depends on `dacom`, `zlib`, `winmm`), `Common` (depends on `DALib`, `dacom`, `qhull`), `EngBase`, `system`.
  - **Client/UI/render/audio/content (later in M8+):** `Content`, `Server`/`rpclocal` flows as required, `RP8`/`RendComp`/`Shading`/`flmaterials` (render), `SoundManager`/`SoundStreamer` (audio), `Thorn`/script/UI resources, `imeui` integration.
  - **Reference note:** vcxproj graph is a build/reference hint; runtime/DACOM interface dependencies must be validated from `bin/DACOM.INI`, `freelancer.ini`, and code behavior (see `linux/docs/minimal-boot-subset.md`).

### 2) Win32/MSVC/COM/resource/registry/DirectX call-site inventory (not yet fully covered)

Inventory method: targeted `rg` scans of `src/freelancer` and key dependency modules against current shim surface in `linux/include/linux_port/*` and `linux/src/*`. This is a preparation inventory (representative, not exhaustive line-by-line signoff); remaining gaps must be resolved/stubbed incrementally in M8.1+.

| Area | Representative call sites (examples) | Current shim/port status | M8 action |
|---|---|---|---|
| **Process entry / bootstrap** | `WinMain`/startup flow assumptions in `src/freelancer/Client/main.cpp`, `window.cpp`, `engine_init.cpp` | No Win32 app entry wrapper; Linux harnesses use `main()` and explicit init hooks | M8.1: add Linux entry/wrapper, call existing explicit init paths |
| **Window creation & Win32 message pump** | `CreateWindowExA`, `RegisterClass*`, `ShowWindow`, `SetWindowPos`, `DestroyWindow`, `DefWindowProcA`, `PeekMessageA`/`GetMessageA`/`TranslateMessage`/`DispatchMessageA` in `window.cpp`, `message_pump.cpp`, `engine_init.cpp` | `fl_port::Window` (SDL3) exists for harnesses but upstream client code still calls Win32 APIs directly | M8.3: adapt/replace call sites or provide Win32-compatible wrappers backed by SDL3; do not treat opaque `HWND` as real Win32 handle |
| **WndProc/messages & input semantics** | `WM_*` handling in `window.cpp`, key/msg paths in `base_walk.cpp`, `ui_*`, IME/chat (`client_ime_focus.*`, `fl_ime_input_box.*`) | Partial key mapping in `window_input` abstraction; no full Win32 message translation layer for client | M8.1 stub minimal handlers, M8.3 map SDL events to expected client behavior |
| **Registry/config persistence** | `RegOpenKeyExA`/`RegCreateKeyExA`/`RegQueryValueExA`/`RegSetValueExA`/`RegCloseKey` in `Common/common_playeraccount.cpp`, `Client/reg_key_stack.cpp`, `ui_fast_table.cpp`, `startup_config.cpp`/`main.cpp` paths | `win32_registry.h` now provides XDG-backed (`fl_port::XdgConfig`/`registry.ini`) Reg* shim behavior for bring-up early settings/save calls; `freelancer_linux_client` exercises HKCU create/set/open/query + config/save persistence. Full key/fidelity audit pending upstream client linkage | M8.4: wire client registry access through XDG-backed shim and audit required keys (bring-up wiring done; audit pending M8.5+) |
| **Resources (.rc, strings, cursors/icons, dialogs)** | `resources/freelancer.rc`, `LoadCursor`/`LoadIcon`/`LoadString`/resource ID usage, dialog/movie/EULA helpers | No `.rc` resource compiler integration in Linux build; string/resource loading not ported | M8.1 stub/bypass nonessential resources (movies/EULA UI paths where safe), later load data-driven equivalents where required |
| **COM/DACOM interfaces & refcounting** | Extensive `QueryInterface`/`AddRef`/`Release` and COM-style patterns in client and deps (e.g. `fl_fx_mesh.cpp`, `gf_thorn_component.h`, `RendComp`, `Shading`) | DACOM/COM macros and core interfaces supported in bring-up shims; calling-convention/vtable/layout correctness under full client not yet validated | M8.2+: preserve DACOM semantics, validate `__stdcall`/`thiscall`/layout assumptions on i386 targets |
| **DirectX graphics (D3D8/D3DX)** | Render pipeline integration expected via `RP8`/`RendComp`/`Shading` and client render call paths; prior PoC/backend is scoped Vulkan (`fl_port::RenderPipeline`) | Native D3D8 API not implemented; `linux_port/d3d8.h`/Vulkan backend provide limited harness-scoped functionality only | M8.3: introduce adapter satisfying minimal client render expectations or stub draw paths to clear/present |
| **DirectSound/DirectInput/DirectShow/WinMM** | Audio/input/movie/system deps implied by Windows libs (`dsound`, `winmm`, etc.) and client boot/menu features | `NullAudioBackend` exists; no DirectSound/DirectShow/real input-device API parity | M8.4: satisfy boot/menu calls with null/stub backends, defer real media/input stacks |
| **MSVC/CRT/toolchain specifics** | MSVC pragmas, SEH/exception filter patterns, CRT-specific APIs/behavior, struct packing/alignment assumptions in parts of codebase | Linux build uses GCC/Clang with compat headers (`win32_compat.h`, `msvcrt_path.h`) and guarded wrappers; coverage incomplete | M8.1+: compile incrementally with shims/guards, replace or isolate MSVC-only constructs, verify ABI-sensitive code paths |
| **Networking / sockets / RPC-local** | `rpclocal`, server/client session flows, Winsock-era assumptions may appear in deps | Not inventoried as fully shimmed for client boot | Out of minimal M8.0/M8.1 scope except stubs needed to reach controlled boot; track for M8.2+ |

**Shim coverage snapshot (existing):** path normalization/resolution, Win32 file I/O subset, basic threading/sync/time, module loader (`dlopen`/`dlsym` wrappers), FDUMP/logging, registry/XDG config helpers, SDL3 window/input abstraction, scoped Vulkan render backend, null audio. **Known gaps:** full Win32 window/message API compatibility, complete registry semantics, resource loading pipeline, DirectX/DirectShow/DirectSound APIs, full COM apartment/runtime expectations.

### 3) Linking strategy decision (static-link vs hybrid)

**Decision:** **Static-link first** for the first successful `freelancer_linux_client` binary (M8.1 target), with selective use of existing Linux port static libraries.

- **Rationale:**
  - Minimizes runtime loader/`dlopen` ordering issues while DACOM registration/lifecycle is still being ported to explicit init.
  - Matches current bring-up model (harnesses link upstream TUs/port libs explicitly) and reduces variability in early boot debugging.
  - Simplifies symbol resolution and makes stub replacement/local overrides easier during bring-up.
- **Hybrid/later considerations:**
  - Revisit for subsystem/plugin boundaries once client boots reliably (e.g. render/audio backends or DACOM modules) if it improves iteration or parity.
  - Keep module-loader abstraction available for cases where upstream expects loadable modules, but prefer explicit registration in-tree initially.

### 4) CMake skeleton target: `freelancer_linux_client`

- Added dedicated target `freelancer_linux_client` in `linux/CMakeLists.txt` under tool builds.
- **Initial scope (M8.0 skeleton):** minimal Linux-side translation unit(s) only, linked against core port/platform libraries; does **not** yet compile/link upstream `src/freelancer` client sources.
- **Purpose:** establish a single, reproducible CMake integration point for incremental M8 bring-up (M8.1 will evolve this into the real client link with entry/wrapper, selected upstream TUs, and stubs).
- **Build note:** target is intentionally minimal to keep `linux-x86`/`linux-x64` bring-up builds green while M8.1 source integration is prepared.
- **M8.1 gating plan:** when upstream `src/freelancer` TUs (and related 32-bit upstream wrapper/object dependencies) are linked into this target, move/define `freelancer_linux_client` under `if(FL_LINUX_32BIT)` in `linux/CMakeLists.txt` and emit a `message(STATUS ...)` skip notice when `FL_LINUX_32BIT=OFF`, since upstream bring-up TUs rely on 32-bit x86 layouts/calling conventions. Keep unconditional for the M8.0 stub.

## Risk hotspots

- Hidden global constructors / static initialization order
- Remaining COM / QueryInterface usage assuming Windows apartment or vtable layouts
- Resource loading (.rc / string tables / cursors)
- Code assuming HWND is a real window handle or classic Win32 message pump
- Float/math edge cases only appearing under the full client
- 32-bit library availability (SDL3, Vulkan loader, etc.)

## M8.1 — Minimal linkable client skeleton (completed deliverables)

- **Linux entry point / WinMain wrapper (skeleton):** added `linux/src/linux_client_main.cpp` with `main()` as the process-entry wrapper for `freelancer_linux_client`. Performs startup logging, `/proc/self/exe`-first exe path resolution (argv[0] fallback), and best-effort `bin/DACOM.INI` cwd selection from startup cwd and exe-relative locations; structured to wrap/invoke upstream client `WinMain` in later milestones (not wired yet).
- **Minimal objects for process start + DACOM acquisition (32-bit):** `freelancer_linux_client` (linux-x86) links upstream wrapper TUs `fl_upstream_dacom_tu`, `fl_upstream_readfile_tu`, and `fl_upstream_x86math_tu` plus `fl_linux_port_platform`/`fl_linux_port_path`; startup acquires `ICOManager*` via `DACOM_Acquire()`.
- **Explicit registration hooks (dependency order):** installs DACOM FDUMP bridge (`fl_upstream_dacom_install_fdump_bridge`), registers DACOM built-ins, registers ReadFile (`fl_upstream_readfile_integration_register`), and registers x86math (`fl_upstream_x86math_integration_register`); x86math registration failure is treated as fatal in the M8.1 skeleton.
- **Stubs / no-op placeholders:** added `linux/src/linux_client_stubs.cpp` with logged no-op helpers/placeholders for movies, EULA gate bypass, registry-write no-op, and generic not-implemented marker. Added `linux/include/linux_port/win32_user.h` (included via `linux_port/windows.h`) for Win32 bootstrap UI declarations and defined `ShowCursor`/`SetForegroundWindow`/`SetFocus`/`UpdateWindow` with C linkage in the stub TU (`ShowCursor` maintains display-count semantics).
- **Path/logging/timing/threading:** uses `fl_port` logging and DACOM FDUMP bridge for diagnostics; startup path/cwd handling via `std::filesystem` + port path utilities context; relies on existing Linux port shim/platform primitives for timing/threading/file/path behavior in linked components.
- **Build/link/run validation (acceptance):** `cmake --build linux/build-x86 --target freelancer_linux_client -j4` links successfully; `./linux/build-x86/freelancer_linux_client` reaches entry point, completes DACOM acquisition and explicit core registrations, prints skeleton status, and exits cleanly (code 0). Non-32-bit tools builds keep a fallback stub target with a status message (M8.1 upstream-linked skeleton requires `FL_LINUX_32BIT=ON`).

## M8.2 — Core engine initialization (bring-up prototype + incremental upstream-path progress; full acceptance pending)

- **Client-oriented early/core init sequence (bring-up scope):** `linux/src/linux_client_main.cpp` drives startup path/bin-context resolution, FDUMP bridge installation, DACOM acquisition, explicit core registrations (DACOM built-ins, ReadFile, x86math), DACOM config application, `freelancer.ini` parsing/data-path resolution, core component creation, and config/DATA access validation before exit. This is a bring-up prototype and does **not** yet replace or invoke compiled upstream client/library initialization paths.
- **`bin/DACOM.INI` + `bin/freelancer.ini` + `DATA/` loading (bring-up path):** resolves/loads `DACOM.INI` via effective explicit-registration config (source-only checkout: `[Libraries]` entries filtered to avoid unavailable external module auto-loads), resolves `freelancer.ini`, reads `[Freelancer].data path`, resolves the DATA root case-insensitively, and validates config/data access through Linux-compatible path resolution and Win32 file shim APIs.
- **Core component creation/load + IFileSystem exercise:** creates `FileSystem/DOS` over resolved DATA root via `ICOManager::CreateInstance` after explicit ReadFile registration and exercises `IFileSystem` methods (`GetCurrentPath`, `GetPathAttributes` on key DATA paths) to validate ReadFile component path/search behavior in addition to Win32 shim reads.
- **Path/search-path/BINI-loading handling (current scope):** validated case-insensitive/backslash-tolerant resolution and reads for key files (`DACOM.INI`, `freelancer.ini`, `DATA/solar/solararch.ini`, `DATA/UNIVERSE/universe.ini`, `DATA/EQUIPMENT/select_equip.ini`, `DATA/FONTS/fonts.ini`) and resolved `DATA/InitialWorld.ini` presence. Full-client-only path/search/BINI issues remain pending until upstream client/library TUs are compiled and linked.
- **Validation (prototype):** `cmake --build linux/build-x86 --target freelancer_linux_client -j4` succeeds; `./linux/build-x86/freelancer_linux_client` exits 0 with DACOM initialized, core component created, `IFileSystem` exercised, and config/DATA checks passing.
- **Incremental upstream DALib path integration:** added `linux/src/dalib_core_bringup.cpp` (minimal Linux bring-up implementation of `DALib::OpenData`/`DALib::CloseData` and `DALib::Dacom`/`DALib::Data`) and wired `freelancer_linux_client` to create/probe/release the DATA filesystem via `DALib::OpenData`/`DALib::CloseData`. This advances M8.2 toward client-code-path usage but does not yet compile/link full upstream `DALib`/`Common`/`src/freelancer` init sequences.

### Strategic correction (review-driven, in progress)
- Freeze new simulation-only init logic in `linux_client_main.cpp`; treat current code as a temporary bring-up prototype.
- Prioritize compiling/linking real upstream sources in dependency order (`zlib`/`qhull` -> `DALib` -> `Common` -> `EngBase`/`system` -> `src/freelancer` client init TUs) so MSVC/header/ABI/static-init/BINI/INI issues surface before M8.3+.
- **Integration gate progress:** added 32-bit Linux static library targets `fl_upstream_zlib` and `fl_upstream_qhull` in `linux/CMakeLists.txt` (leaf deps) and verified they build in `linux/build-x86`. DALib/Common (and downstream upstream client/library TUs) compilation/linkage targets are still pending.
- M8.2 remains `[~]` until upstream client/library code paths drive config/DATA/core initialization and acceptance is validated through them.

## M8.3 — Window + input + render hookup (completed for bring-up client path)

- **Window/message pump adaptation (bring-up client):** `linux_client_main.cpp` creates a native window via `fl_port::Window` (SDL3, Wayland-first, Vulkan-enabled config) and runs the frame/event loop through `Window::run_loop` (SDL event pump integrated in port layer) after M8.2 init succeeds.
- **Input mapping (SDL3 → VK):** `WindowCallbacks` are wired for key/mouse/window events; key events expose `KeyEvent.win_vk` mapped via `sdl_key_to_win_vk` (logged in bring-up path) to validate client-facing VK mapping integration.
- **Render hookup:** `fl_port::RenderPipeline` is initialized against the created window and driven with `render_frame(...)` for clear+present output; resize events trigger `rp.resize(...)`.
- **Stable frame loop:** bounded bring-up frame loop (clear+present) with close-request handling and basic event processing implemented.
- **Runtime validation:** 32-bit `freelancer_linux_client` (linux-x86) successfully creates a native Wayland/SDL3 window, initializes the render pipeline, processes input events, and presents 120 clear+present frames before clean exit (validated via run/gdb).
- **Segfault fix (review):** resolved M8.3 startup crash caused by ELF global symbol interposition from upstream wrapper TUs linked into the executable (affecting dlopened SDL/Vulkan/Mesa components) by compiling upstream wrapper TUs with hidden visibility and exporting only explicit bring-up hooks. Added M8.3 init checkpoints (`window.create`, render init) and separated `render_error` from `close_requested` so render failures return exit code 22.

## M8.4 — Audio / config / persistence unblock (completed for bring-up client path)

- **Null audio backend (boot/menu bring-up calls):** `freelancer_linux_client` now initializes `fl_port::NullAudioBackend` (from `fl_linux_port_gameplay`) and exercises representative boot/menu audio calls in-process: `init`, `set_master_volume`, `play_sound` (menu/music), `is_playing`, `update`, `stop_sound`, `stop_all`, and `shutdown`. This unblocks audio-touching bring-up flow without requiring a real audio device.
- **XDG config/registry/save shim usage:** early settings/save calls are exercised via `fl_port::XdgConfig` (config/state/data dirs, `freelancer_linux.ini` read/write, state save path + persistence check) and via XDG-backed Win32 registry shim APIs (`RegCreateKeyExA`, `RegSetValueExA`, `RegOpenKeyExA`, `RegQueryValueExA`, `RegCloseKey`) targeting HKCU `Software\\Microsoft\\Microsoft Games\\Freelancer\\1.0`.
- **Registry shim implementation (bring-up, review-fixed):** `linux/include/linux_port/win32_registry.h` maps logical registry keys/values to XDG-backed `registry.ini` storage with typed value encoding/decoding (`sz:`, `dword:`, `hex:`, `expand_sz:`), correct `RegQueryValueExA` type/size semantics for `REG_SZ`/`REG_DWORD`, `(Default)` value mapping, `WINAPI` (`__stdcall` on i386) signatures, and thread-safe handle tracking. `XdgConfig::reg_read/reg_write` preserve full normalized hierarchical key paths as INI sections to avoid key collisions.
- **XDG isolation / smoke safety:** `freelancer_linux_client` supports isolated smoke runs via `FL_PORT_ISOLATE_XDG=1` or `--smoke-xdg-temp` (temp XDG root, cleaned on exit) and writes runtime-derived install-path values (no hardcoded host `/home/freelancer`).
- **Blocker stubs/bypasses:** `linux/src/linux_client_stubs.cpp` adds/invokes deterministic bring-up stubs for movie/DirectShow playback bypass (`fl_stub_movie_*`, `fl_stub_directshow_playback`), IME enable/focus/context hooks (`fl_stub_ime_*`), EULA gate bypass, and legacy registry-write marker; additional common Win32 window/bootstrap no-op stubs were added for early bring-up compatibility.
- **IFileSystem stream exercise (review feedback):** M8.2 bring-up checks additionally exercise ReadFile-component stream reads by opening a DOS child filesystem instance for a resolved DATA file and reading bytes via `IFileSystem::Read` (handle 0).
- **Runtime validation:** rebuilt and ran 32-bit `linux/build-x86/freelancer_linux_client --smoke-xdg-temp` successfully; M8.4 checks (including REG_SZ/REG_DWORD round-trip), IFileSystem stream read, and M8.3 window/input/render clear+present loop complete with clean exit and isolated XDG cleanup.


## M8.5 — Reach main menu / controlled state (bring-up equivalent completed)

- **Boot diagnostics & early-out points:** `linux_client_main.cpp` now emits phase checkpoints (`BOOT[Startup|DacomInit|CoreRegistrations|ConfigAndData|IFileSystemProbe|AudioUnblock|PersistenceUnblock|BlockerStubs|WindowCreate|RenderInit|MainMenu|ControlledState|Shutdown]`) and phase-tagged failure diagnostics (`[FAIL][Phase]`, `M8.5 early-out ...`) with stable exit codes for bring-up failure localization.
- **Crash/symbol/behavioral validation (bring-up scope):** rebuilt and reran 32-bit `freelancer_linux_client` (including isolated smoke runs) through M8.2–M8.5 bring-up flow; no crashes or missing-symbol runtime failures observed in the validated bring-up path. Upstream client TU linkage remains the primary source of future crash/symbol/ABI issues.
- **Controlled main-menu/in-engine state:** after M8.2–M8.4 bring-up initialization/unblocks and M8.3 window+render hookup, the client performs a boot-to-menu transition and enters a stable controlled in-engine loop (event pumping, input callbacks, audio updates, continuous clear+present frames). It logs and prints `M8.5: reached main menu (equivalent stable in-engine bring-up state)` and completes a bounded stable-state loop before clean shutdown. Boot-to-menu visual transition now uses dynamic render clear-color updates (`RenderPipeline::set_clear_color`) during the loop.
- **Acceptance (bring-up equivalent):** native client starts, initializes bring-up engine/core subsystems, opens a native window, processes input, renders/presents frames, and reaches a documented equivalent stable main-menu/controlled in-engine state. This is **not** the real upstream Freelancer menu/UI flow (upstream client/menu TUs not yet linked).
- **M8.5 review fixes:** isolated XDG temp cleanup is RAII-managed for all early-exit paths; IFileSystem stream verification now prefers parent-mounted `data_fs->CreateInstance` (upstream-style) with root-manager fallback and fails `[IFileSystemProbe]` explicitly if stream read cannot be verified; smoke script auto-detects `linux/linux-x86` then `linux/build-x86` and prints environment hints for `WindowCreate`/`RenderInit` failures (exit 20/21).
- **Smoke test:** `linux/scripts/smoke_freelancer_client_m85.sh` runs `freelancer_linux_client --smoke-xdg-temp` and verifies required checkpoints/logs, IFileSystem stream read, M8.4 unblocks, main-menu controlled-state reach/completion, and clean exit.


## M8.6 — Stabilization & handoff to M9 (bring-up scope completed)

- **CMake cleanup / reproducibility (bring-up client):**
  - `linux/CMakeLists.txt` now exposes `FL_LINUX_BUILD_CLIENT` and documents/gates `freelancer_linux_client` paths:
    - `FL_LINUX_32BIT=ON` (recommended via `linux-x86` preset): builds full M8 bring-up client (upstream DACOM/ReadFile/x86math wrapper TUs + M8.3–M8.5 flow)
    - `FL_LINUX_32BIT=OFF`: builds fallback stub target to keep x64 tools presets usable
  - Added explicit status messages for missing SDL3/Vulkan window/render hook availability (runtime acceptance impact) and kept preset-driven reproducible commands as the primary workflow.
  - **Scope note:** this stabilizes/reproducibilizes the current bring-up client target; a "full client build" that links upstream `src/freelancer`/`DALib`/`Common`/engine init TUs is still pending (M8.2/M8.5 true upstream integration gate before M9-heavy UI/render assumptions).

- **Expanded test surface (automated smoke):**
  - `linux/scripts/smoke_freelancer_client_m85.sh` now validates a broader contract: boot/init phases, core registration/config+DATA success, IFileSystem stream read, M8.4 audio/config/persistence/blocker unblocks, window creation + SDL driver evidence, render pipeline init, main-menu controlled-state reach/completion with presented frame counts, final status, shutdown, and isolated XDG cleanup.
  - Script auto-detects `linux/linux-x86` or `linux/build-x86` build dirs and provides exit-code hints for `WindowCreate` (20) / `RenderInit` (21) environment issues.
  - Test remains display/graphics-runtime dependent (Wayland/X11 + 32-bit SDL3/Vulkan + repo DATA files) and is intended for opt-in/manual or environment-capable CI execution.

- **32-bit vs 64-bit pressure reevaluation (handoff):**
  - **Current decision:** keep `freelancer_linux_client` bring-up path **32-bit-first** (`FL_LINUX_32BIT=ON`, `linux-x86` preset).
  - **Reasons:** upstream wrapper TUs and bring-up integration assumptions target 32-bit x86 ABI/layout/calling conventions (`__stdcall`/Win32 shims, vtable/layout-sensitive interfaces, x86 math path), and linked upstream source objects are built/validated in that mode today.
  - **64-bit pressure:** desirable long-term for tooling/runtime/library availability, but premature for current upstream-linked bring-up because it requires audited struct/layout/vtable/calling-convention parity, pointer-size fixes, and revalidation of upstream TUs and COM/DACOM interfaces.
  - **Handoff guidance:** defer 64-bit migration until after upstream client/library TU linkage stabilizes on 32-bit and an explicit ABI/layout audit plan is executed; revisit once M8.2/M8.5 upstream integration points are compiling and testable.



## M8.7 — Upstream client/library integration & M8.2 completion gate (in progress)

Roadmap **M8.7** is the pre-M9 gate to complete open M8.2 items by linking/invoking real upstream client/library init paths. Progress in this iteration:

- **DALib core (upstream-derived integration, linked):**
  - `linux/src/dalib_core_bringup.cpp` upgraded to a more complete upstream-derived DALib core TU: defines DALib subsystem globals and error hooks (`FatalErrorFn`/`CriticalWarningFn`), implements retail-equivalent `DALib::OpenData`/`CloseData`, and provides `DALib::Startup`/`Shutdown` API stubs with upstream-compatible signatures for incremental integration.
  - `freelancer_linux_client` now invokes `DALib::Startup(...)` and `DALib::OpenData`/`CloseData` through DALib API paths during bring-up init/config/data flow (data-path exercised via linked DALib core TU); full upstream `Startup`/`Shutdown` subsystem init (SystemContainer/engine/audio/render/DirectPlay/CDP networking) remains unported.
- **Upstream bootstrap/init integration scaffold (guarded):**
  - Added `linux/src/m87_upstream_bootstrap.cpp` (`fl_m87_try_invoke_upstream_bootstrap`) and wired `linux_client_main.cpp` to attempt guarded upstream bootstrap/init invocation with diagnostics and controlled fallback when upstream `src/freelancer` init TUs are unavailable.
- **M8.7 upstream integration probes/scaffolding (CMake, EXCLUDE_FROM_ALL):**
  - Added `fl_m87_dalib_upstream_core_probe` target + stub headers/wrapper (`linux/include/m87_dalib_stubs/*`, `linux/src/dalib_upstream_core_m87.cpp`) to iterate on compiling upstream `src/DALib/DALib.cpp` for Linux core bring-up.
  - Added `fl_m87_common_ini_probe` target to probe `src/Common/common_ini.cpp` portability/build blockers under Linux 32-bit compile flags.
- **Validation:** `freelancer_linux_client` builds in `linux/build-x86` and `linux/scripts/smoke_freelancer_client_m85.sh` passes (boot/init, DALib upstream API data-path checks, IFileSystem probe, M8.4 unblocks, window+render, controlled main-menu state, clean shutdown).

### Remaining blockers to complete M8.7 / close M8.2
- **DALib (full upstream):** `src/DALib/DALib.cpp` Startup/Shutdown depends on SystemContainer/engine/sound/streamer/alchemy/render/network interfaces and DirectPlay/CDP paths not yet Linux-portable; requires guarded ports/stubs for subsystem init and networking-heavy TUs.
- **Common / EngBase / system:** selected early-init TUs require Win32/MSVC/COM/CRT/packing/ABI compatibility work (e.g. `__declspec` import/export semantics, `_itoa`/CRT differences, x87/inline-asm guards, struct size/layout parity, registry/time/thread/file APIs). No minimal upstream subset is linked into `freelancer_linux_client` yet.
- **Upstream client bootstrap (`src/freelancer`):** `Client/main.cpp` and `engine_init.cpp` have extensive transitive dependencies (Win32 windowing/resources, IME, game states/UI/sound/render/data subsystems, DirectShow/movie/EULA/resource DLLs) and need guarded Linux build wrappers, stubs, and incremental symbol resolution before they can replace bring-up flow.
- **M8.2 closure:** `DACOM.INI`/`freelancer.ini`/`DATA` and BINI/INI/search-path behavior are exercised through linked DALib core API and bring-up probes, but **not yet** fully driven/validated through compiled upstream Common/client init TUs; M8.2 remains `[~]` pending successful upstream TU linkage and runtime validation.

## Known stubs / deviations

This list documents intentional stubs, bring-up substitutions, and behavioral deviations in the current `freelancer_linux_client` path (M8.1–M8.5). Items remain until replaced/validated against real upstream client/library code paths.

### Client integration / boot flow
- `freelancer_linux_client` does **not** yet compile/link full upstream `src/freelancer`, `DALib`, or `Common` client/library init TUs and does **not** call real upstream `WinMain`/`InitializeEngine`/menu/game state code paths end-to-end.
- **M8.7 progress:** linked upstream-derived DALib core TU exercises DALib API paths (`DALib::Startup` bring-up stub + retail-equivalent `OpenData`/`CloseData`) and guarded upstream bootstrap hook (`fl_m87_try_invoke_upstream_bootstrap`) is invoked with fallback diagnostics when upstream init TUs are unavailable.
- M8.5 "main menu" acceptance remains an **equivalent controlled stable in-engine bring-up state** (boot transition + stable render/input/audio loop), not the real Freelancer UI/menu system.
- `linux/src/linux_client_main.cpp` still contains bring-up init/helper logic and controlled-state fallback pending replacement by compiled upstream client/library initialization and game-flow code. New simulation-only init logic should remain frozen except for diagnostics/bring-up unblock needs.

### DACOM / module loading / config
- Uses an effective `DACOM.INI` with `[Libraries]` entries removed for source-only explicit-registration bring-up (external runtime module binaries not shipped/auto-loaded); differs from retail auto-load behavior.
- Core registrations are explicit and bring-up-scoped (DACOM built-ins, ReadFile, x86math wrapper hooks) rather than full retail module discovery/load.
- `IFileSystem`/config/DATA checks are bring-up validation probes (selected files/paths/stream read), not full coverage of all client asset/search-path/BINI behaviors.

### Audio / persistence / config
- `fl_port::NullAudioBackend` satisfies boot/menu-style audio calls in bring-up path (init/play/stop/update/volume/shutdown) but produces no real audio output/mixing/streaming/spatialization.
- `fl_port::XdgConfig` and XDG-backed `win32_registry.h` provide bring-up persistence for early settings/save flows (INI config, `registry.ini` typed values, state save path checks), not full Windows registry or all client config semantics.
- `win32_registry.h` scope/fidelity: typed `sz`/`dword`/`hex`/`expand_sz` storage, basic create/open/query/set/close, `(Default)` mapping, full hierarchical key sections, `WINAPI` signatures, thread-safe handles; **not** full Win32 registry semantics (enumeration, key/value deletion, security/ACLs, atomicity/transactions, complete type coverage, handle rights semantics).
- Smoke runs can isolate XDG dirs (`FL_PORT_ISOLATE_XDG=1` / `--smoke-xdg-temp`) and clean up temp state; normal runs use standard XDG locations.

### Movies / IME / UI / window bootstrap
- Movie/DirectShow playback is stubbed/bypassed (`fl_stub_movie_*`, `fl_stub_directshow_playback`); no video/audio playback, codecs, timing, or synchronization.
- IME support is stubbed (`fl_stub_ime_*`) with no real text composition/candidate UI/input-method integration.
- EULA/intro gating and legacy registry-write markers are bypassed/no-op for bring-up.
- Additional Win32 window/bootstrap APIs are no-op or simplified (`ShowCursor` count semantics, `SetForegroundWindow`/`SetFocus`/`GetFocus`/`GetForegroundWindow`/`IsWindow`/`ShowWindow`/`EnableWindow`/`UpdateWindow`/`DestroyWindow` stubs), not full Win32 behavior.

### Render / input / presentation
- Render path is the existing bring-up Vulkan/SDL3 clear+present pipeline via `fl_port::RenderPipeline`/`fl_port::Window` (not upstream render/UI/material/shader/scene graph integration).
- Controlled menu state is represented by stable frame loop/state transitions and clear+present output (no real menu widgets/textures/fonts/UI scripts).
- Input is processed through SDL3 callbacks with VK mapping logging/sampling for bring-up validation; full client input bindings/IME/mouse capture semantics are pending.

### Platform / ABI / runtime assumptions
- Upstream-wrapper bring-up executable is 32-bit (`FL_LINUX_32BIT`) and depends on compatible 32-bit SDL3/Vulkan/runtime libraries for window/render acceptance.
- Upstream wrapper TUs linked into the client use hidden visibility with exported explicit bring-up hooks to avoid ELF symbol interposition issues in dlopened graphics components.
- Path handling in bring-up logic includes Linux case-insensitive/backslash normalization helpers; this is compatibility behavior and not full retail filesystem/search-path parity.


## Artifacts

- CMake target: `freelancer_linux_client` (`linux/CMakeLists.txt`) — reproducible bring-up client entry point with `FL_LINUX_BUILD_CLIENT` gating:
  - 32-bit (`linux-x86` preset / `FL_LINUX_32BIT=ON`): DACOM/ReadFile/x86math upstream wrapper TUs + M8.2–M8.5 bring-up flow (window/input/render hooks when SDL3/Vulkan available)
  - 64-bit/tools presets (`FL_LINUX_32BIT=OFF`): fallback stub target for preset compatibility
- Upstream TU config helper: `fl_upstream_tu_iface` INTERFACE target in `linux/CMakeLists.txt` (shared include dirs/defs for guarded 32-bit upstream wrapper consumers; hidden visibility/exported hook policy for wrapper TUs)
- Entry/stub/shim sources: `linux/src/linux_client_main.cpp` (M8.2–M8.7 init flow, DALib::Startup/OpenData/CloseData upstream API calls, guarded upstream bootstrap hook attempt, M8.3 hookup, M8.4 unblocks, M8.5 diagnostics/controlled-state flow), `linux/src/dalib_core_bringup.cpp` (upstream-derived DALib core TU: globals/error hooks + OpenData/CloseData + Startup/Shutdown bring-up stubs), `linux/src/m87_upstream_bootstrap.cpp` (guarded upstream client bootstrap/init integration scaffold), `linux/src/linux_client_stubs.cpp`, `linux/include/linux_port/win32_user.h`, `linux/include/linux_port/win32_registry.h`
- M8.7 integration probes/scaffolding: `linux/src/dalib_upstream_core_m87.cpp`, `linux/include/m87_dalib_stubs/*`, CMake EXCLUDE_FROM_ALL targets `fl_m87_dalib_upstream_core_probe` and `fl_m87_common_ini_probe`
- Dependency graph: `linux/docs/vcxproj-dependency-graph.json`, `linux/docs/vcxproj-dependency-graph.md` (refreshed in M8.0)
- Smoke-test script: `linux/scripts/smoke_freelancer_client_m85.sh` (expanded M8.5/M8.6 bring-up validation: boot/init, window+render, M8.4 unblocks, controlled main-menu state, presented frames, shutdown/cleanup; auto-detects `linux/linux-x86` or `linux/build-x86`; display/32-bit SDL3/Vulkan dependent)
- Known-stubs/deviations list: (this document, "Known stubs / deviations" section)
