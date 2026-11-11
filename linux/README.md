# Linux port scaffolding (M0/M1 bring-up)

This directory holds **Linux-native bring-up scaffolding** for Freelancer Decomp. The upstream project currently builds only on Windows (MSVC/Win32, DirectX). See [`../assessment.md`](../assessment.md), [`../roadmap.md`](../roadmap.md), and [`docs/m0-project-foundation.md`](docs/m0-project-foundation.md).

## Status
Scaffolding, probes, harnesses, and de-risk PoCs are in place (Phase 0 baseline + M0 foundation artifacts). The full game is **not** buildable/runnable on Linux yet.

## Build infrastructure (M0)
- **CMake:** `CMakeLists.txt`
- **Ninja presets:** `CMakePresets.json` (`linux-x64`, `linux-x86`) in this directory and at repo root
  - From repo root: `cmake --preset linux-x64 -S linux && cmake --build --preset linux-x64 -j`
  - From repo root (32-bit): `cmake --preset linux-x86 -S linux && cmake --build --preset linux-x86 -j` (`linux-x86` configure preset sets `PKG_CONFIG_LIBDIR=/usr/lib32/pkgconfig:/usr/share/pkgconfig` for 32-bit SDL3/Vulkan discovery)
  - From `linux/`: `cmake --preset linux-x64 && cmake --build --preset linux-x64 -j` (or `cmake --preset linux-x86 && cmake --build --preset linux-x86 -j`)
- **Dedicated toolchains:** `cmake/toolchain-linux-x86.cmake` (i686/`-m32`), `cmake/toolchain-linux-x86_64.cmake` (x86_64)
- **Manual configure examples:** see `docs/m0-project-foundation.md` and root `README.md` (if reconfiguring a 32-bit build dir with cached 64-bit pkg-config results, clear `CMakeCache.txt`/`CMakeFiles` first).

## Source/build organization
- `include/linux_port/`, `src/` — path normalization, Win32/platform shims, Linux-backed file I/O, module-loading/logging helpers
- `tests/` — path/config probes, shim/ABI/boot harnesses, upstream DACOM/ReadFile wrapper integration harnesses (32-bit-gated where required), graphics env probe and SDL3+Vulkan PoC
- `tools/` — maintenance scripts (dependency graph generator)
- `docs/` — Phase 0/M0 status, environment/build notes, de-risk findings, dependency graph, and minimal boot planning

## M0 planning artifacts
- **Dependency graph (from `src/**/*.vcxproj`):** `docs/vcxproj-dependency-graph.md`, `docs/vcxproj-dependency-graph.json`
  - Regenerate: `python3 tools/generate_vcxproj_dep_graph.py`
- **Minimal boot subset/module order:** `docs/minimal-boot-subset.md`
- **M0 foundation summary:** `docs/m0-project-foundation.md`

## Run / see progress
- 64-bit examples: `./linux/build/fl_path_resolve bin '..\\DATA\\solar\\solararch.ini'`, `./linux/build/dacom_ini_probe bin/DACOM.INI`, `./linux/build/gfx_env_probe`, `./linux/build/gfx_wayland_poc` (or preset build dirs such as `linux/linux-x64/`)
- 32-bit examples: `./linux/build-x86/core_shim_harness`, `./linux/build-x86/dacom_boot_harness`, `./linux/build-x86/upstream_dacom_readfile_integration_harness`, `./linux/build-x86/gfx_env_probe`, `./linux/build-x86/gfx_wayland_poc` (or `linux/linux-x86/`)

## Notes
- Keep changes behavior-preserving and preferably additive/guarded so the upstream Windows build remains unaffected.
- Upstream modules are Win32/MSVC/DirectX-heavy; integration proceeds via shims, guarded wrapper TUs, and harnesses.
- 32-bit builds require a working multilib toolchain and 32-bit runtime/dev dependencies (including Vulkan/Wayland/SDL3 stacks for graphics targets). See `docs/build-and-env-status.md`.
- Do not assume fake `HWND` integers satisfy engine consumers; adapt required behavior in platform backends.
- Phase/milestone tracking: `../roadmap.md`, `docs/phase0-status.md`.