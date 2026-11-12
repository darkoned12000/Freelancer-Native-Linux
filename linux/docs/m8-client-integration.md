# M8 — Game client integration (living document)

**Status:** M8.2 in progress — bring-up prototype complete (DACOM/config/DATA/core component init and IFileSystem checks succeed in `freelancer_linux_client`), but full M8.2 acceptance remains pending compilation/linkage and validation through real upstream client/library code paths (`src/freelancer`, `DALib`, `Common`, etc.).  
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
| **Registry/config persistence** | `RegOpenKeyExA`/`RegCreateKeyExA`/`RegQueryValueExA`/`RegSetValueExA`/`RegCloseKey` in `Common/common_playeraccount.cpp`, `Client/reg_key_stack.cpp`, `ui_fast_table.cpp`, `startup_config.cpp`/`main.cpp` paths | `fl_port::XdgConfig` + `win32_registry.h` shim exist for harness-level needs, but coverage/fidelity for all client keys and behaviors unverified | M8.4: wire client registry access through XDG-backed shim and audit required keys |
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

## M8.2 — Core engine initialization (bring-up prototype progress; full acceptance pending)

- **Client-oriented early/core init sequence (bring-up scope):** `linux/src/linux_client_main.cpp` drives startup path/bin-context resolution, FDUMP bridge installation, DACOM acquisition, explicit core registrations (DACOM built-ins, ReadFile, x86math), DACOM config application, `freelancer.ini` parsing/data-path resolution, core component creation, and config/DATA access validation before exit. This is a bring-up prototype and does **not** yet replace or invoke compiled upstream client/library initialization paths.
- **`bin/DACOM.INI` + `bin/freelancer.ini` + `DATA/` loading (bring-up path):** resolves/loads `DACOM.INI` via effective explicit-registration config (source-only checkout: `[Libraries]` entries filtered to avoid unavailable external module auto-loads), resolves `freelancer.ini`, reads `[Freelancer].data path`, resolves the DATA root case-insensitively, and validates config/data access through Linux-compatible path resolution and Win32 file shim APIs.
- **Core component creation/load + IFileSystem exercise:** creates `FileSystem/DOS` over resolved DATA root via `ICOManager::CreateInstance` after explicit ReadFile registration and exercises `IFileSystem` methods (`GetCurrentPath`, `GetPathAttributes` on key DATA paths) to validate ReadFile component path/search behavior in addition to Win32 shim reads.
- **Path/search-path/BINI-loading handling (current scope):** validated case-insensitive/backslash-tolerant resolution and reads for key files (`DACOM.INI`, `freelancer.ini`, `DATA/solar/solararch.ini`, `DATA/UNIVERSE/universe.ini`, `DATA/EQUIPMENT/select_equip.ini`, `DATA/FONTS/fonts.ini`) and resolved `DATA/InitialWorld.ini` presence. Full-client-only path/search/BINI issues remain pending until upstream client/library TUs are compiled and linked.
- **Validation (prototype):** `cmake --build linux/build-x86 --target freelancer_linux_client -j4` succeeds; `./linux/build-x86/freelancer_linux_client` exits 0 with DACOM initialized, core component created, `IFileSystem` exercised, and config/DATA checks passing.

### Strategic correction (review-driven, in progress)
- Freeze new simulation-only init logic in `linux_client_main.cpp`; treat current code as a temporary bring-up prototype.
- Prioritize compiling/linking real upstream sources in dependency order (`zlib`/`qhull` -> `DALib` -> `Common` -> `EngBase`/`system` -> `src/freelancer` client init TUs) so MSVC/header/ABI/static-init/BINI/INI issues surface before M8.3+.
- **Integration gate progress:** added 32-bit Linux static library targets `fl_upstream_zlib` and `fl_upstream_qhull` in `linux/CMakeLists.txt` (leaf deps) and verified they build in `linux/build-x86`. DALib/Common (and downstream upstream client/library TUs) compilation/linkage targets are still pending.
- M8.2 remains `[~]` until upstream client/library code paths drive config/DATA/core initialization and acceptance is validated through them.

## Known stubs / deviations

- `freelancer_linux_client` (M8.2 bring-up prototype): initializes DACOM/config/DATA/core components and exercises `IFileSystem` in bring-up scope, but does not yet compile/link upstream `src/freelancer`/`DALib`/`Common` init paths or call upstream `WinMain`/`InitializeEngine`; no game window/message/frame loop/boot flow yet (M8.3+).
- `linux_client_main.cpp` still contains temporary bring-up init/helper logic (INI parsing, config/data checks) pending replacement by real upstream client/library code paths; new simulation logic should be frozen.
- `linux_client_main.cpp` uses an effective `DACOM.INI` with `[Libraries]` entries removed for source-only explicit-registration bring-up (external runtime module binaries not shipped); differs from retail auto-load behavior.
- `linux_client_stubs.cpp`: movies/EULA/registry-write and Win32 bootstrap stubs remain intentional bring-up placeholders; additional stubs/shims will be required as upstream TUs are linked.
- (To be expanded as M8 progresses.)

## Artifacts

- CMake target: `freelancer_linux_client` (`linux/CMakeLists.txt`) — 32-bit bring-up client (DACOM/ReadFile/x86math upstream wrapper TUs + Linux entry/stubs), currently implements M8.1/M8.2 init scope; non-32-bit fallback stub preserved for tools presets
- Upstream TU config helper: `fl_upstream_tu_iface` INTERFACE target in `linux/CMakeLists.txt` (shared include dirs/defs for guarded 32-bit upstream wrapper consumers)
- Entry/stub/shim sources: `linux/src/linux_client_main.cpp` (M8.2 core init flow), `linux/src/linux_client_stubs.cpp`, `linux/include/linux_port/win32_user.h`
- Dependency graph: `linux/docs/vcxproj-dependency-graph.json`, `linux/docs/vcxproj-dependency-graph.md` (refreshed in M8.0)
- Smoke-test script: (to be created in M8.6)
- Known-stubs/deviations list: (this document, section above)
