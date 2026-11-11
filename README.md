# Freelancer Native Linux (Wayland port)

This repository is a **native Linux port effort** based on [Freelancer Decomp](https://github.com/darkoned12000/Freelancer-Native-Linux): a reverse-engineered, behavior-matching reimplementation of the **Freelancer (2003)** client and engine DLLs.

**Goal:** run Freelancer **natively on Linux** (Wayland-first, targeting EndeavourOS/Hyprland + AMD RDNA3/RADV-class GPUs) **without Wine or Proton**, while preserving original game behavior and compatibility with legally obtained game data.

> Status: **early bring-up (M4 windowing and input completed, pending review)**. The original Windows build remains the upstream reference; Linux-native runtime is not yet launchable.

## Project goals & constraints

- **Native Linux only:** ELF executables/modules, Linux-native platform backends (filesystem, windowing/input, audio, threads/timing/module loading), no Windows compatibility layer.
- **Preserve behavior first:** keep gameplay/data behavior aligned with the original game wherever possible.
- **32-bit initially:** retain **x86 (32-bit)** architecture during bring-up to avoid compounding ABI/asm/layout/serialization risks; 64-bit is deferred.
- **Preserve architecture/interfaces:** keep DACOM and existing engine interfaces/contracts where possible; replace implementations behind a Linux platform layer.
- **Graphics strategy (phased):**
  - **Level 1 (first major objective):** native Linux app with compatible D3D8 rendering behavior if feasible (must be validated in a native Linux x86 architecture; not assumed to be a Wine drop-in).
  - **Level 2 (later):** fully native renderer behind existing interfaces (e.g. Vulkan) only if/when needed.

See [`assessment.md`](assessment.md) for the detailed technical assessment, subsystem inventory, risks, and refactored plan.

## Current progress

### Phase 0 — Bring-up foundation / de-risking (completed)
- [x] Repo cloned and organized for Linux work (`DATA/` canonicalized with `data -> DATA` compatibility symlink).
- [x] **#3 Path normalization:** deterministic, ASCII case-insensitive path resolution + separator normalization for Linux (`linux/include/linux_port/path_util.*`, `fl_path_resolve`).
- [x] **#2 Config/boot probe:** `dacom_ini_probe` parses `bin/DACOM.INI`/`bin/freelancer.ini`, emits boot/load-order and component/provider hints, resolves config/data paths, and extracts `.vcxproj` dependency hints.
- [x] **#1 Platform shims + #1.2 DACOM/ReadFile integration:** Win32/platform shims, Linux-backed file I/O, 32-bit build/runtime validation, guarded upstream `dacom.cpp`/`ReadFile.cpp` wrapper TUs, and integration harnesses with real config/file-backed reads and explicit registration hooks.
- [x] **#4 Graphics de-risk spike:** `gfx_env_probe` and SDL3+Vulkan `gfx_wayland_poc` (swapchain + present validation) plus `IRenderPipeline`/`RP8` scope findings. See `linux/docs/gfx-de-risk-spike.md`, `linux/docs/irenderpipeline-rp8-scope.md`, `linux/docs/build-and-env-status.md`.

### M0 — Project foundation (completed)
- [x] CMake/Ninja presets and dedicated i686/x86_64 toolchains (`CMakePresets.json`, `linux/CMakePresets.json`, `linux/cmake/toolchain-*.cmake`).
- [x] Vcxproj dependency graph tooling/artifacts (`linux/tools/generate_vcxproj_dep_graph.py`, `linux/docs/vcxproj-dependency-graph.{json,md}`).
- [x] Minimal boot subset and foundation docs (`linux/docs/m0-project-foundation.md`, `linux/docs/minimal-boot-subset.md`).

### M1 — Core platform compatibility (completed, pending review)
- [x] Engine-facing path normalization/resolution integration (`GetFullPathNameA`, `_fullpath`, `SearchPathA`, cwd helpers, `_access`/`_mkdir`) and case-insensitive/normalized Linux behavior.
- [x] Win32 shim fidelity improvements (timing/sync/atomics, pointer-sized types, file/time helpers) and preserved logging/diagnostics/error mapping.
- [x] Targeted validation expanded with `m1_platform_tests` (path edge cases, file I/O/find, config variants/failure modes, timing/sync/interlocked, module resolution, ABI/layout checks); upstream DACOM/ReadFile harnesses remain runnable.

### M2 — DACOM and engine initialization (completed, pending review)
- [x] DllMain-free explicit DACOM/init lifecycle hooks (FDUMP bridge, built-in registration, explicit ReadFile registration).
- [x] Config/INI loading through Linux-compatible paths (`bin/DACOM.INI`, `bin/freelancer.ini`, resolved `data path`) with source-only explicit-registration-aware handling for unavailable external runtime modules.
- [x] Minimal core component init/load in dependency order proven by 32-bit `m2_engine_init_harness` (component system init, config load, registration/composition, and successful core component instantiation including `FileSystem/DOS`). See [`roadmap.md`](roadmap.md) and `linux/docs/m2-dacom-engine-init.md`.

### M3 — Math and game data (completed, pending review)
- [x] Portable/isolated Linux `x86math` (3DMathEngine) implementation with explicit DACOM registration and numeric/behavior validation (`linux/src/x86math_portable.cpp`, `linux/tests/upstream_x86math_tu.cpp`, `m3_math_data_tests`).
- [x] Path resolution integrated into config/data loading flows on case-sensitive layout (resolved `bin` configs and `DATA/` assets, `FileSystem/DOS` instantiated against resolved data root).
- [x] Core Freelancer configuration and game data loaded from `DATA/` with consistency/spot checks (including BINI data files) via `m3_math_data_tests`. See [`roadmap.md`](roadmap.md) and `linux/docs/m3-math-game-data.md`.

### M4 — Windowing and input (completed, pending review)
- [x] SDL3-based Wayland-first window/event abstraction (`linux/include/linux_port/window_input.h`, `linux/src/window_input.cpp`) with stable opaque adapter handle (avoids fake-`HWND` assumptions) and lifecycle/size/title APIs.
- [x] Keyboard/mouse/controller mapping to expected input semantics (Win32-like VK key mapping helper, mouse button/motion/wheel, SDL gamepad buttons/axes with normalized values and hotplug handling).
- [x] Focus/lifecycle, resize, cursor visibility, mouse grab, and relative mouse mode handling.
- [x] Event loop integration with engine/frame/update flow (`pump_events`, `run_loop` tick integration) proven by `m4_window_input_harness` (native window + functioning input/event loop). See [`roadmap.md`](roadmap.md) and `linux/docs/m4-windowing-input.md`.

## Milestones

| Milestone | Focus | Acceptance target | Status |
|---|---|---|---|
| **M0** | Project foundation | CMake+Ninja Linux build scaffolding, 32-bit toolchain validation, source/build organization, dependency graph from `.vcxproj` | Completed |
| **M1** | Core platform compatibility | Filesystem/path compat, Win32 shims, logging/diagnostics, timing, threading/sync, module loading; **`DACOM`/`ReadFile` compile or run in harnesses with targeted tests** | Completed (pending review) |
| **M2** | DACOM & engine initialization | Linux DACOM/module init model, config/INI loading, preserved registration/composition semantics; **component system initializes and loads core components** | Completed (pending review) |
| **M3** | Math & game data | Portable/isolated `x86math` (3DMathEngine) registration + numeric validation, path-integrated data/resource loading, core config/game data loaded from `DATA/`; **assets/configs resolve correctly on case-sensitive FS** | Completed (pending review) |
| **M4** | Windowing & input | SDL3-based Wayland-first window/event abstraction, keyboard/mouse/controller, engine adapters; **native Linux window + input/event loop** | Completed (pending review) |
| **M5** | Graphics bring-up | Standalone D3D8-compat feasibility PoC (32-bit, RADV) then RP8 subset integration or scoped native Vulkan fallback; **minimal in-engine frame/scene renders and presents** | Planned |
| **M6** | Gameplay bring-up | Null-audio then real audio backend, XDG config/registry shim, save/load/local SP flow, UI/missions/init; **reach main menu and enter game world** | Planned |
| **M7** | Native Linux release candidate | Stability/perf/audio verification, fullscreen/windowed/controller, docs/install; **playable native Linux experience with provided game data, no Wine/Proton** | Planned |

## Linux bring-up tools (current)

Experimental scaffolding lives in [`linux/`](linux/):

- `linux/CMakeLists.txt` — CMake entry for helper prototypes/tools/harnesses
- **Path/config probes:** `fl_path_resolve`, `dacom_ini_probe`
- **Shim/integration harnesses:** `core_shim_harness`, `abi_module_harness`, `dacom_boot_harness`, `m1_platform_tests`, `dacom_upstream_compile_probe` (64/32-bit as applicable), plus 32-bit upstream wrapper/integration targets (`fl_upstream_dacom_tu`, `fl_upstream_readfile_tu`, `fl_upstream_x86math_tu`, `upstream_dacom_readfile_harness`, `upstream_dacom_integration_harness`, `upstream_dacom_readfile_integration_harness`, `m2_engine_init_harness`, `m3_math_data_tests`)
- **Windowing/input (M4):** `fl_linux_port_window`, `m4_window_input_harness` (SDL3, Wayland-first; built when usable target SDL3 libs are available)
- **Graphics de-risk (#4):** `gfx_env_probe`, `gfx_wayland_poc` (SDL3+Vulkan; built when usable target SDL3/Vulkan libs are available)
- **Docs:** `linux/docs/phase0-status.md`, `linux/docs/build-and-env-status.md`, `linux/docs/path-normalization.md`, `linux/docs/dacom-config-probe.md`, `linux/docs/win32-shim-notes.md`, `linux/docs/dacom-readfile-harness-spike.md`, `linux/docs/gfx-de-risk-spike.md`, `linux/docs/irenderpipeline-rp8-scope.md`, `linux/docs/m4-windowing-input.md`

### Build & run (to see progress)

> Run commands from the repository root. These are Phase 0/M1–M4 analysis, harness, and PoC targets; the full game is not yet buildable or launchable on Linux.

Requires CMake (3.20+), Ninja, and a C/C++ toolchain (Clang/GCC). The **32-bit** bring-up build (`linux-x86`) also requires a working multilib toolchain and 32-bit runtime/dev dependencies (including 32-bit SDL3/Vulkan/Wayland stacks for graphics targets where applicable; see `linux/docs/build-and-env-status.md`).

#### 1) 64-bit helper/probe/graphics build (preset: `linux-x64`)

```bash
cmake --preset linux-x64 -S linux
cmake --build --preset linux-x64 -j

# Path/config probes
./linux/linux-x64/fl_path_resolve bin '..\DATA\solar\solararch.ini'
./linux/linux-x64/dacom_ini_probe bin/DACOM.INI

# Platform/shim probes and M1 tests
./linux/linux-x64/core_shim_harness
./linux/linux-x64/m1_platform_tests
./linux/linux-x64/dacom_upstream_compile_probe
./linux/linux-x64/gfx_env_probe

# M4 windowing/input (native window + functioning input/event loop)
./linux/linux-x64/m4_window_input_harness

# SDL3+Vulkan presentation PoC (Wayland session; creates a window and presents 120 clear frames)
./linux/linux-x64/gfx_wayland_poc
```

Build output directory: `linux/linux-x64/`.

#### 2) 32-bit bring-up build (preset: `linux-x86`, target ABI for initial port)

```bash
cmake --preset linux-x86 -S linux
cmake --build --preset linux-x86 -j

# Core/path/config/shim checks
./linux/linux-x86/fl_path_resolve bin '..\DATA\solar\solararch.ini'
./linux/linux-x86/dacom_ini_probe bin/DACOM.INI
./linux/linux-x86/core_shim_harness
./linux/linux-x86/m1_platform_tests
./linux/linux-x86/dacom_boot_harness

# Upstream DACOM/ReadFile wrapper integration (real config/file-backed reads, explicit registration hooks)
./linux/linux-x86/upstream_dacom_readfile_integration_harness

# M2 engine init: DACOM component system init, config load, explicit core component load
./linux/linux-x86/m2_engine_init_harness

# M3 math + game data: x86math registration/numerics, path-integrated config/DATA load
./linux/linux-x86/m3_math_data_tests

# M4 windowing/input (native window + functioning input/event loop)
./linux/linux-x86/m4_window_input_harness

# 32-bit graphics capability + presentation PoC
./linux/linux-x86/gfx_env_probe
./linux/linux-x86/gfx_wayland_poc
# If native Wayland backend is unavailable in 32-bit SDL3 on your setup, try:
# SDL_VIDEODRIVER=x11 ./linux/linux-x86/gfx_wayland_poc
# SDL_VIDEODRIVER=x11 ./linux/linux-x86/m4_window_input_harness
```

Build output directory: `linux/linux-x86/`. The `linux-x86` configure preset sets `PKG_CONFIG_LIBDIR=/usr/lib32/pkgconfig:/usr/share/pkgconfig` for 32-bit SDL3/Vulkan discovery; you can also prefix the configure command with that environment variable if needed.

**What to look for:** `fl_path_resolve` prints resolved Linux paths for Windows-style inputs; `dacom_ini_probe` prints boot/config analysis; integration/harness targets end with success/`PASS`/`overall: ok` (accurate library-load/config warnings may appear in this source-only checkout without upstream runtime DLLs); `gfx_wayland_poc` reports device/swapchain and `presented frames: 120/120 (ok)`. See `linux/docs/phase0-status.md` and `linux/docs/build-and-env-status.md` for details.

> **Note:** If a build directory was deleted or you need to reconfigure (e.g. switching ABI or after cached pkg-config/toolchain changes), rerun the `cmake --preset ... -S linux` configure step. To force a clean reconfigure, remove the preset build directory (e.g. `linux/linux-x86` or `linux/linux-x64`) or delete its `CMakeCache.txt`/`CMakeFiles` first. Manual `-B` build directories and alternative layouts are documented in `linux/docs/m0-project-foundation.md`.

## Repository layout (relevant)

- `src/` — upstream decomp source and Visual Studio solution (`Freelancer.sln`, `.vcxproj` modules)
- `bin/` — runtime config/templates (`DACOM.INI`, `freelancer.ini`, `flconfigdatabase.txt`, etc.)
- `DATA/` — Freelancer game data (user-provided; required at runtime)
- `DATA_EXTRA/` — port-specific data (e.g. RP9 shaders/volumetric assets)
- `docs/` — reverse-engineering and format/config documentation
- `linux/` — native Linux port scaffolding, prototypes, and bring-up docs
- `assessment.md` — Linux port assessment and roadmap

## Upstream project (Windows build/reference)

The upstream *Freelancer Decomp* project builds on Windows (Visual Studio 2022, v143, Win32/x86) using `src/Freelancer.sln`, DirectX SDK dependencies, and outputs to `bin/`. Refer to upstream docs for original build instructions and module details:
- [docs/README.md](docs/README.md), [docs/MISSION.md](docs/MISSION.md)
- Russian README: [README.ru.md](README.ru.md)

## Contributing / development notes

- Keep changes **behavior-preserving** and, where possible, **additive/guarded** to avoid breaking the Windows reference build.
- Maintain a clear separation between original game logic and Linux platform/backend implementations.
- Prefer tests/harnesses and Windows-vs-Linux behavior comparisons for boot-critical systems (config parsing, path resolution, math, data loading).

## License & third-party components

- Project source (Freelancer Decomp) is released under [The Unlicense](LICENSE).
- Vendored libraries under `src/` (e.g. `src/zlib/vendor`, `src/qhull/vendor`) retain their own licenses.
- Retail/third-party game binaries and assets are **not** covered by this license and remain property of their respective owners (including game data and any retail DLLs present such as `EBUEula.dll`, `EULA.RTF`, `EXE/gundll.dll`).

## Upstream contacts & support

Original project contacts (see upstream README for donation/support details): Discord `@Giperion`, [gipdereline@gmail.com](mailto:gipdereline@gmail.com). 
