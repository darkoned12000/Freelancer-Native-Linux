# Native Linux Port — Initial Assessment & Plan (refactored)

**Repository:** `darkoned12000/Freelancer-Native-Linux`  
**Assessment date:** 2026-11-10 (updated with external review feedback)  
**Target environment:** EndeavourOS, Hyprland (Wayland), AMD Radeon RX 7900 XTX  
**Primary objective:** produce a **native Linux** implementation that can run original Freelancer (2003) behavior/gameplay using legally obtained game data, **without Wine or Proton**.

> This is an evidence-based assessment and living plan. The project is currently a Windows/Win32, DirectX-era, DACOM-plugin codebase with substantial platform-specific surface area.

---

## 1. Project definition of success

### Mandatory requirements (target)
- Native Linux **ELF** executable and native Linux shared libraries/modules (as applicable).
- **No Wine or Proton** dependency; no Windows executable compatibility layer.
- Native Linux filesystem/config handling, windowing/input, audio, and process/thread/timing primitives.
- Compatible with original Freelancer game data and directory structure (with portability handling where needed).
- Preserve original game behavior and gameplay functionality wherever possible.

### Levels of graphics success
- **Level 1 (first major objective): Native Linux application with compatible D3D8 behavior**
  - Linux-native process, filesystem, windowing/input, audio, threading/timing/module loading.
  - Existing DirectX 8 rendering behavior provided via a **compatible translation/implementation path** if feasible.
  - **Important:** "not Wine" does not automatically mean "drop in DXVK/D8VK binaries". Any D3D8 compatibility approach must be validated to work with this project's **native Linux x86 ABI/module architecture** (and not assume a Wine Windows-runtime environment). Native ELF use of D8VK/DXVK/wined3d-derived components may require significant integration work.
- **Level 2 (long-term/future, and a key feasibility alternative): Native renderer behind engine interfaces**
  - Implement rendering behind `IRenderPipeline` (and related contracts such as `RendComp`, `Shading`, `flmaterials`) using Vulkan/OpenGL or another Linux-native graphics API, reproducing legacy behavior.
  - **Strategic note:** game logic/assets primarily target engine interfaces (e.g. `IRenderPipeline`) rather than raw D3D8 calls; `RP8.dll` is the D3D8 backend implementation. A minimal native backend behind `IRenderPipeline` may involve less fragile integration than forcing a full D3D8 runtime into a native Linux ELF process, but requires contract/behavior fidelity. Evaluate `src/include/render/IRenderPipeline.h` and `src/RP8/` early alongside the D3D8-compat PoC.

**Near-term commitment:** run a **standalone D3D8-compat feasibility PoC** and in parallel evaluate the scope of a minimal native `IRenderPipeline` backend. Do not commit to a full renderer rewrite or to D3D8 translation until evidence supports the chosen path.

### Initial architecture constraints
- Retain **32-bit x86** architecture initially (defer 32→64-bit migration).
- Preserve **DACOM** and existing engine interfaces wherever possible.
- Introduce a dedicated **Linux platform layer**; keep original game logic/interfaces stable and replace implementations underneath.

---

## 2. Executive summary

- **What this is:** *Freelancer Decomp* — a behavior-matching reimplementation of the Freelancer (2003) client and engine DLLs, built from `src/` (Visual Studio/Win32). Retail game data is required (repo currently contains `DATA/` and `DATA_EXTRA/`).
- **Current platform/toolchain:** Windows-only (MSVC v143, Win32, MFC for `FLServer`, DirectX 8.1 SDK, optional DX9 for `RP9`), MSBuild/`.vcxproj`, no Linux build system.
- **Architecture:** DACOM/COM-like component system loads modules from `bin/DACOM.INI` and composes `[System]`/`[Engine]`/`[RenderManager]`. Most subsystems are DLL-style modules with Win32 assumptions.
- **Highest-risk/critical areas (early):** DACOM/module loading semantics, filesystem/path behavior, x86/MSVC-specific math/ABI code, Win32 windowing/input/message model, D3D8 rendering contracts (`RP8`), DirectSound/COM audio. Audio/movies/IME/registry/resources/nonessential networking can be staged.

### Subsystem priority snapshot

| Subsystem | Priority | Notes |
|---|---:|---|
| DACOM | Critical | Foundation: interfaces/factories/registration/composition |
| File system / `ReadFile` + path compat | Critical | Case sensitivity, separators, search paths, data loading |
| `x86math` / low-level CPU & CRT | Important | Isolate MSVC/x86 asm and ABI-sensitive behavior |
| Windowing/input | Critical | First graphical launch; **prefer SDL3 abstraction initially** (Wayland-capable) |
| RP8 / D3D8 rendering | Major challenge | Level 1 feasibility proof required before commitment |
| Audio (`SoundManager`/`SoundStreamer`) | Stageable | **Start with null-audio backend**, implement real audio later |
| Networking (nonessential MP/UI) | Defer | Preserve local/SP behavior first; `FLServer` MFC deprioritized |
| Thorn/Lua | Preserve | Keep original behavior; portability mostly integration/lifecycle |
| Movies/IME | Defer | Stub/disable for bring-up if blocking |
| Registry/resources | Needs strategy | XDG-backed config/state; portable resource handling plan |

---

## 3. Recommended architecture (Linux)

Introduce a dedicated Linux platform layer and replace implementations under existing interfaces (do not scatter ad-hoc `#ifdef` logic throughout game code where avoidable).

- **Freelancer client/game logic** (original behavior) →
- **DACOM component system** (interfaces, factories, registration, composition) →
- **Linux platform layer** (filesystem/path compat, module loading, threads/sync, timing, logging/diagnostics, registry replacement, process/runtime helpers) →
- **Linux runtime backends** (windowing/input, audio, graphics integration, resources) →
- **Linux kernel / userspace stack** (ELF/POSIX, Wayland, Vulkan/RADV, PipeWire/ALSA, etc.).

**Design rules**
- Keep original game interfaces/contracts where possible (e.g. `ISoundManager`, `ISoundStreamer`, `IRenderPipeline`, material/texture/render-component interfaces).
- Keep **path resolution separate from file I/O** (independently testable).
- Prefer minimal, reversible stubs for nonessential subsystems during bring-up.

### Proposed subsystem mapping

| Original Windows subsystem | Linux direction (initial) |
|---|---|
| Win32 window/messages/`HWND` consumers | **SDL3**-based window/event abstraction (Wayland), with adapters to engine-expected behavior (do **not** assume a fake `HWND` integer is sufficient) |
| DirectInput | SDL3 input (keyboard/mouse/controller), mapped to expected semantics |
| DirectSound / ACM / WinMM audio | **Null-audio backend first**; later SDL3 audio or OpenAL Soft (+ decode strategy) |
| Registry (`HKEY`/`Reg*`) | XDG-compliant config/state storage shim |
| Threads / `CRITICAL_SECTION` / events | `pthreads` / `std::thread` / `std::mutex` / condition variables (as appropriate) |
| Timing (`GetTickCount`, perf counters) | `std::chrono`-based equivalents preserving required behavior |
| DLL loading (`LoadLibrary/GetProcAddress`) | `dlopen`/`dlsym` (after static-link bring-up proves init), with explicit init/shutdown strategy |
| Filesystem APIs / search paths | `std::filesystem` + compatibility layer (`path_util` + VFS/ReadFile integration) |
| DirectShow/MCI movies | Disable/stub initially; replace later if needed |
| Win32 resources (`.rc` resource DLLs) | Investigate extraction/conversion to portable resources; avoid reproducing full Win32 resource loader early |

---

## 4. Repository & runtime layout (observed)

- `src/` — Visual Studio solution and modules (`Freelancer.sln`, many `.vcxproj`): core (`dacom`, `DALib`, `ReadFile`, `x86math`, `EngBase`, `system`, `Common`), rendering (`RP8`, `RP9`, `RendComp`, `Shading`, `flmaterials`, `ximage`, `alchemy`), audio (`SoundManager`, `SoundStreamer`), scripting/video/UI (`Thorn`, `movie`, `imeui`), client/server/network (`freelancer`, `Content`, `Server`, `FLServer`, `rpclocal`, `Remote*`), resources/tools.
- `bin/` — runtime/config: `DACOM.INI`, `dacomsrv.ini`, `freelancer.ini`, `flconfigdatabase.txt`, character templates.
- `DATA/` — game data present in this checkout (canonical uppercase dir; `data -> DATA` symlink exists for compatibility).
- `DATA_EXTRA/` — port extras (e.g. RP9 shaders/volumetric data).
- `docs/` — RE/INI/module documentation.
- Retail/third-party blobs present: `EBUEula.dll`, `EULA.RTF`, `EXE/` dir.

Runtime/config notes:
- `bin/DACOM.INI`: local `DllPath=.\`, loads port-built modules; `[System]` selects **`DirectX8`** by default.
- `bin/freelancer.ini`: `data path = ..\DATA` (Windows-style separators/relative path), `local_server = rpclocal.dll`, resource/data mappings.

---

## 5. Build system & toolchain

### Current (Windows)
- `src/Freelancer.sln`, configs `Debug|Win32`, `DebugASan|Win32`, `Release|Win32`.
- Requires DX8 SDK (`dx8/`), optional DX9 SDK for `RP9`, Windows SDK, MFC for `FLServer`, `rc.exe`.
- Feature/build toggles in `src/Freelancer.Diagnostics.props`.

### Linux bring-up direction
- Add parallel **CMake + Ninja** build for Linux (keep Windows build intact).
- Use **Clang or GCC**, initially targeting **32-bit x86 (`-m32`)**.
- Start with the smallest bootable subset compiled into test executables/harnesses; restore DACOM-style dynamic module loading (`.so` + `dlopen/dlsym` + explicit init/shutdown) after initialization is proven.

### 32-bit dependency management (must be tracked early)
Linux bring-up requires **32-bit** development/runtime dependencies where applicable (not just 64-bit):
- 32-bit C/C++ runtime and toolchain/multilib support.
- 32-bit SDL3 (if selected for window/input/audio abstraction).
- 32-bit Vulkan loader and Mesa/RADV stack (verify **32-bit app can enumerate/use RX 7900 XTX**).
- 32-bit audio, compression/image, and other libraries needed by selected modules.

Do not defer 32-bit graphics stack validation until late rendering milestones.

---

## 6. Subsystem findings & portability implications

### 6.1 DACOM / module loading & composition
- **Role:** component manager, config/library loading, factory registration, COM-like `QueryInterface/AddRef/Release`, built-in profile parser/regexp.
- **Linux implications:** abstract `LoadLibraryA/GetProcAddress/FreeLibrary/SearchPathA/DllMain/HMODULE`; preserve registration order/init semantics; define explicit module init/shutdown for Linux bring-up.

### 6.2 File system & data access
- **Role:** `ReadFile` provides DOS/UTF/MEM filesystem and search-path behavior; critical for assets/configs.
- **Linux implications:** Win32 file/mapping/enumeration APIs, path separators, case-insensitive expectations, and search-path semantics.
- **Current state:** `DATA/` canonicalized at repo root with `data -> DATA` compatibility symlink. `bin/freelancer.ini` still uses Windows-style `..\DATA` and backslashes; normalization + case-insensitive resolution must be integrated into runtime paths.

### 6.3 Math / low-level CPU & CRT assumptions
- **Role:** `x86math` and other code paths contain MSVC/x86-specific constructs/asm and ABI-sensitive behavior.
- **Linux implications:** isolate/replace asm and compiler-specific idioms carefully; validate numerical/behavioral parity against Windows reference where possible.

### 6.4 Rendering, windowing, presentation
- **Abstraction:** `IRenderPipeline` and related render/material/mesh interfaces (`RP8`/`RendComp`/`Shading`/`flmaterials`/`ximage`).
- **Active backend:** `RP8` (`DirectX8`, D3D8). `RP9` is optional and not default.
- **Linux implications:** provide Linux-native window/surface + input/event loop (Wayland-first; SDL3 recommended for initial abstraction) and a graphics integration path satisfying D3D8-era contracts.
- **Graphics strategy (phased):**
  1. **Standalone PoC:** create Wayland-capable window, initialize proposed D3D8 compatibility/integration path, render/present a triangle (validate 32-bit/Vulkan/ABI viability).
  2. **RP8 subset integration:** only if PoC proves feasible in this native architecture.
  3. **Fallback:** plan a native Vulkan backend behind `IRenderPipeline` for a minimal render slice if compatibility route is not viable. Avoid full renderer rewrite until necessary.

### 6.5 Audio
- **Current:** DirectSound/COM/ACM/WinMM-heavy (`SoundManager`, `SoundStreamer`).
- **Linux implications:** implement **null-audio backend first** to unblock initialization/game loop; then implement real backend (SDL3 audio or OpenAL Soft) with behavior-compatible mixing/3D/streaming strategy.

### 6.6 Input, IME, video/movies
- **Input:** client uses Win32 messages and DirectInput-style handling; map to SDL3/Wayland events and engine-expected semantics.
- **IME/movies:** Windows-integrated (`imeui`, DirectShow/MCI). Reasonable to stub/defer for early milestones.

### 6.7 Networking, server, RPC
- Preserve local/SP (`rpclocal`, core content/server logic) behavior first. `FLServer` MFC UI is Windows-only and deprioritized for client launch.

### 6.8 Resources, registry, external components
- **Registry:** replace with XDG-backed storage shim preserving required semantics.
- **Resource DLLs (`.rc`):** plan portable extraction/loading strategy.
- **External retail blobs:** `EBUEula.dll`/`EULA.RTF`, `EXE/gundll.dll` may require bypass/stub/reimplementation depending on launch path.

---

## 7. Cross-cutting dependencies to abstract/replace

- Windowing/events (`HWND`, `WndProc`, message pump semantics, focus/lifecycle) — adapt to Linux window backend without assuming fake-`HWND` suffices.
- Dynamic loading/modules (`HMODULE`, `LoadLibrary*`, `GetProcAddress`, `DllMain`, search rules).
- Filesystem/path APIs and semantics (Win32 calls, separators, mappings, enumeration, case sensitivity).
- Threads/sync/waits, timers/performance counters.
- Registry and system info.
- COM and DirectX/DirectSound/DirectInput/DirectShow/MCI/WinMM usage.
- MSVC/x86-specific codegen/ABI/asm/struct-layout assumptions.

---

## 8. Testing, validation, and source organization (plan additions)

### 8.1 Automated Windows-vs-Linux behavior testing
Build a regression/comparison suite early (using original Windows behavior as reference where possible), especially for:
- Configuration/INI parsing
- Asset/path lookup and file loading
- Math/calculation behavior
- Serialization/save-game parsing
- Model/texture/data loading
- Mission/engine initialization checkpoints

Goal: detect behavior differences that affect gameplay, not just internal implementation details.

### 8.2 Build a dependency graph (milestone-worthy)
Derive and document from `.vcxproj`/source:
- Module dependencies and link graph
- Windows API / DirectX usage per module
- Modules that may compile with minimal changes
- Modules requiring shims/replacements
- Modules safe to stub/disable for a minimal boot

Use this to define the smallest bootable Linux configuration.

### 8.3 Source/layout separation (logical or physical)
Maintain a clear distinction between:
- Original/reverse-engineered game code (minimize churn)
- Linux platform implementations/shims
- Graphics backend experiments (`d3d8_compat`, native `IRenderPipeline` backends)
- Tests/harnesses and Linux build tooling

(Physical reorganization can be incremental; avoid disruptive moves before build/test scaffolding exists.)

### 8.4 ABI/calling-convention and toolchain notes (32-bit bring-up)
- **Calling conventions/COM-style ABI:** Win32 x86 uses `__stdcall`, `__cdecl`, `__fastcall`, and MSVC `__thiscall` semantics in places. Linux GCC/Clang defaults differ for member functions; use explicit attributes/macros and `-fms-extensions` (Clang) where appropriate, and validate struct-return/stack semantics early.
- **Windowing integration:** do not model `HWND` as a fake integer only; provide adapter/shim behavior for required Win32 window/message operations (sync/async semantics, rect/client coords, focus/lifecycle) mapped to SDL3/Wayland.
- **32-bit toolchain:** prefer a dedicated CMake toolchain file for `i686-linux-gnu` (rather than only ad-hoc `-m32`) so dependency/configure checks run with correct flags; validate 32-bit multilib deps early (Vulkan loader/RADV, Wayland, SDL3, audio libs).
- **Compiler diagnostics:** enable strict warnings early (e.g. `-Wall -Wextra -Wconversion -Wno-unused-parameter`) to catch 32-bit truncation/pointer-conversion issues.

---

## 9. Refactored roadmap (measurable milestones)

### M0 — Project foundation
- Establish Linux build infrastructure (CMake + Ninja, Clang/GCC, **32-bit x86** toolchain file preferred), enable strict warnings, and validate 32-bit multilib dependencies early.
- Establish Linux source/build organization and document original Windows dependencies.
- Create dependency graph from `.vcxproj` and identify minimal boot subset.

### M1 — Core platform compatibility
- Filesystem/path normalization and case-insensitive resolution (refine and test `linux/` path util; integrate later), including Windows-path edge cases encountered in configs/data.
- Win32 compatibility shims (types/calling conventions/COM macros/APIs as needed), logging/diagnostics, timing, threading/sync, dynamic module loading.
- **Acceptance:** `DACOM` and `ReadFile` compile (or run in harnessed form) and pass standalone tests for targeted behavior.

### M2 — DACOM and engine initialization
- Port/adapt DACOM component manager and module initialization model for Linux.
- Preserve component registration/composition semantics; test INI/config loading (`bin/DACOM.INI`).
- **Acceptance:** Linux harness/executable initializes the component system and loads core components successfully.

### M3 — Math and game data
- Port/isolate `x86math` (replace MSVC-specific asm carefully), verify numerical behavior.
- Load Freelancer configuration/game data through compatibility paths.
- **Acceptance:** Core data/config load without Windows APIs; path resolution behaves correctly on case-sensitive layout.

### M4 — Windowing and input
- Implement **SDL3**-based Linux window/event abstraction (Wayland-first), keyboard/mouse/controller, focus/lifecycle.
- Provide adapters for engine consumers (identify actual `HWND`/message dependencies; avoid fake-handle assumptions).
- **Acceptance:** Native Linux window created with functioning input/event loop.

### M5 — Graphics bring-up (highest technical risk)
- Run standalone **D3D8 compatibility feasibility PoC** (Wayland-capable window + init + triangle/present, 32-bit, RADV/7900XTX) and evaluate native ELF integration viability.
- In parallel, evaluate/optionally prototype a **minimal native `IRenderPipeline` backend** (informed by `IRenderPipeline.h`/`RP8`) as an alternative or fallback path.
- If D3D8-compat is feasible, integrate RP8 subset behind existing interfaces; otherwise proceed with scoped native backend behind engine interfaces.
- **Acceptance:** A Freelancer scene/frame (or minimal in-engine render target) can render and present.

### M6 — Gameplay bring-up
- Add **null-audio** early if needed; then real audio backend.
- Save/load/config/registry (XDG shim), local server/SP flow (`rpclocal`), UI/input wiring, mission/game initialization.
- **Acceptance:** Reach main menu and enter the game world (target loop milestone).

### M7 — Native Linux release candidate
- Stability/performance/audio verification, fullscreen/windowed/controller support, installation/docs.
- **Acceptance:** Playable native Linux experience on a clean system with provided game data, without Wine/Proton.

---

## 10. Immediate next actions (updated priority)

User-requested implementation order for current Phase 0 work: **#3, #2, #1, #4**.

1. **#3 Path normalization (in progress):** refine `linux/` path resolver (deterministic collision handling, ASCII-only lowering, simpler missing-tail logic, documented supported path forms), add dedicated tests (exact/case-mixed dirs/files, separators, relative/absolute, `..`, missing components, symlinks, cache invalidation scenarios).
2. **#2 Config/DACOM parsing probe:** build a Linux harness to parse/exercise `bin/DACOM.INI` and related config-loading behavior independent of full engine startup.
3. **#1 Platform abstraction/shim skeleton:** minimal types/macros and abstractions for Win32 handles/calling conventions/debug/module loading/threading primitives needed for early core-module compilation/harnessing.
4. **#4 Graphics de-risk spike (plan/PoC):** standalone Wayland + proposed D3D8-compat presentation experiment (triangle) validating native 32-bit integration on Hyprland/RADV; do not block core bring-up tasks on renderer integration assumptions.

**Also required early:** generate `.vcxproj` dependency graph and define minimal boot subset (feeds M0/M1/M2).

---

## 11. References (in-repo)

- Project/build overview: `README.md`, `README.ru.md`, `docs/README.md`, `docs/MISSION.md`
- DACOM/config: `bin/DACOM.INI`, `bin/freelancer.ini`, `docs/re/dacom.md`, `docs/ini/engine-dacom.md`
- Rendering: `docs/re/render.md`, `docs/re/rp9.md`, `docs/re/shading.md`, `src/include/render/IRenderPipeline.h`
- Audio: `docs/re/audio.md`
- Server/network: `docs/re/server.md`, `docs/re/rpclocal.md`
- Port notes: `docs/hd_edition.md`, `docs/walk_mode.md`, `docs/memory_diagnostics.md`, `docs/re/deviations.md`, `docs/re/crt_compat.md`
- Linux bring-up scaffolding: `linux/README.md`, `linux/docs/phase0-status.md`, `linux/docs/path-normalization.md`

