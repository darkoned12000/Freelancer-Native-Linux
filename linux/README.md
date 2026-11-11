# Linux port scaffolding (M0–M3 bring-up)

This directory holds **Linux-native bring-up scaffolding** for Freelancer Decomp. The upstream project currently builds only on Windows (MSVC/Win32, DirectX). See [`../assessment.md`](../assessment.md), [`../roadmap.md`](../roadmap.md), and [`docs/m0-project-foundation.md`](docs/m0-project-foundation.md).

## Status
Scaffolding, probes, harnesses, and de-risk PoCs are in place (Phase 0 baseline + M0 foundation artifacts + M1 core platform compatibility + M2 DACOM/engine initialization + M3 math/game data work). The full game is **not** buildable/runnable on Linux yet. Current milestone changes are pending code review (not committed).

## Build infrastructure (M0)
- **CMake:** `CMakeLists.txt`
- **Ninja presets (recommended):** `CMakePresets.json` (`linux-x64`, `linux-x86`) in this directory and at repo root. Preset binary dirs are `linux/linux-x64` and `linux/linux-x86`.
  - From repo root (64-bit):
    ```bash
    cmake --preset linux-x64 -S linux
    cmake --build --preset linux-x64 -j
    ```
  - From repo root (32-bit, target ABI for initial bring-up):
    ```bash
    cmake --preset linux-x86 -S linux
    cmake --build --preset linux-x86 -j
    ```
    (`linux-x86` configure preset sets `PKG_CONFIG_LIBDIR=/usr/lib32/pkgconfig:/usr/share/pkgconfig` for 32-bit SDL3/Vulkan discovery.)
  - From `linux/`: `cmake --preset linux-x64 && cmake --build --preset linux-x64 -j` (or `cmake --preset linux-x86 && cmake --build --preset linux-x86 -j`)
- **Dedicated toolchains:** `cmake/toolchain-linux-x86.cmake` (i686/`-m32`), `cmake/toolchain-linux-x86_64.cmake` (x86_64)
- **Reconfigure/clean:** if a build directory was deleted or cached settings changed, rerun the `cmake --preset ...` configure step; for a clean reconfigure remove the preset build dir or delete its `CMakeCache.txt`/`CMakeFiles`. Manual `-B` layouts are documented in `docs/m0-project-foundation.md`.

## Source/build organization
- `include/linux_port/`, `src/` — path normalization, Win32/platform shims, Linux-backed file I/O, module-loading/logging helpers
- `tests/` — path/config probes, shim/ABI/boot harnesses, upstream DACOM/ReadFile wrapper integration harnesses, M2 engine init harness, and M3 math/data harness (32-bit-gated where required), graphics env probe and SDL3+Vulkan PoC
- `src/` — Linux-port implementation helpers (e.g. portable x86math bring-up implementation)
- `tools/` — maintenance scripts (dependency graph generator)
- `docs/` — Phase 0/M0 status, environment/build notes, de-risk findings, dependency graph, and minimal boot planning

## M0 planning artifacts
- **Dependency graph (from `src/**/*.vcxproj`):** `docs/vcxproj-dependency-graph.md`, `docs/vcxproj-dependency-graph.json`
  - Regenerate: `python3 tools/generate_vcxproj_dep_graph.py`
- **Minimal boot subset/module order:** `docs/minimal-boot-subset.md`
- **M0 foundation summary:** `docs/m0-project-foundation.md`

## Run / see progress
- 64-bit preset (`linux/linux-x64`): e.g. `./linux/linux-x64/fl_path_resolve bin '..\DATA\solar\solararch.ini'`, `./linux/linux-x64/dacom_ini_probe bin/DACOM.INI`, `./linux/linux-x64/core_shim_harness`, `./linux/linux-x64/m1_platform_tests`, `./linux/linux-x64/gfx_env_probe`, `./linux/linux-x64/gfx_wayland_poc`
- 32-bit preset (`linux/linux-x86`): e.g. `./linux/linux-x86/core_shim_harness`, `./linux/linux-x86/m1_platform_tests`, `./linux/linux-x86/dacom_boot_harness`, `./linux/linux-x86/upstream_dacom_readfile_integration_harness`, `./linux/linux-x86/m2_engine_init_harness`, `./linux/linux-x86/m3_math_data_tests`, `./linux/linux-x86/gfx_env_probe`, `./linux/linux-x86/gfx_wayland_poc`
- See root `README.md` for full preset build/run command sequences and expected output notes.

## Notes
- Keep changes behavior-preserving and preferably additive/guarded so the upstream Windows build remains unaffected.
- Upstream modules are Win32/MSVC/DirectX-heavy; integration proceeds via shims, guarded wrapper TUs, and harnesses.
- 32-bit builds require a working multilib toolchain and 32-bit runtime/dev dependencies (including Vulkan/Wayland/SDL3 stacks for graphics targets). See `docs/build-and-env-status.md`.
- Do not assume fake `HWND` integers satisfy engine consumers; adapt required behavior in platform backends.
- Phase/milestone tracking: `../roadmap.md`, `docs/phase0-status.md`.