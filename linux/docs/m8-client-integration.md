# M8 — Game client integration (living document)

**Status:** M8.0 preparation/inventory in progress (artifacts and skeleton target created).  
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

## Known stubs / deviations

- `freelancer_linux_client` (M8.0): skeleton executable only; no upstream game client TUs linked yet, no game boot behavior. Upstream WinMain/windowing/resource/DirectX/audio/registry call paths remain to be adapted in M8.1+.
- (To be expanded as M8 progresses.)

## Artifacts

- CMake target: `freelancer_linux_client` (M8.0 skeleton added in `linux/CMakeLists.txt`; to be expanded in M8.1+)
- Dependency graph: `linux/docs/vcxproj-dependency-graph.json`, `linux/docs/vcxproj-dependency-graph.md` (refreshed)
- Smoke-test script: (to be created in M8.6)
- Known-stubs/deviations list: (this document, section above)
