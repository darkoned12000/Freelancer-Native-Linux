# M0 — Project Foundation (status & artifacts)

This document records the completion artifacts for **Milestone M0 (Project foundation)** as defined in `roadmap.md` and `assessment.md`.

**Acceptance target (M0):** CMake+Ninja Linux build scaffolding, 32-bit toolchain validation, source/build organization, dependency graph from `.vcxproj`.

## Completed M0 deliverables

### 1) Linux build infrastructure (CMake + Ninja, 32-bit/x86_64 toolchains, warnings)
- **CMake project:** `linux/CMakeLists.txt` — Linux bring-up targets for path/config probes, platform shims, harnesses, upstream wrapper integration (32-bit-gated), and graphics de-risk probes/PoC.
- **Dedicated toolchain files:**
  - `linux/cmake/toolchain-linux-x86.cmake` (i686, `-m32`, SSE2/FPU flags, warning flags)
  - `linux/cmake/toolchain-linux-x86_64.cmake` (x86_64, warning flags)
- **Ninja presets / workflow:**
  - `linux/CMakePresets.json` with `linux-x64` and `linux-x86` configure/build presets (generator `Ninja`, `binaryDir` `${sourceDir}/${presetName}` e.g. `linux/linux-x64`, `linux/linux-x86`).
  - `CMakePresets.json` at repo root and `linux/CMakePresets.json` provide equivalent `linux-x64`/`linux-x86` presets for repo-root and `linux/` workflows.
  - Example usage (from repo root): `cmake --preset linux-x64 -S linux && cmake --build --preset linux-x64 -j` (or `linux-x86`; `linux-x86` sets `PKG_CONFIG_LIBDIR=/usr/lib32/pkgconfig:/usr/share/pkgconfig` for multilib SDL3/Vulkan pkg-config discovery)
  - Example usage (from `linux/`): `cmake --preset linux-x64 && cmake --build --preset linux-x64 -j` (or `cmake --preset linux-x86 && cmake --build --preset linux-x86 -j`)
- **Warnings:** toolchains export `FL_LINUX_PORT_WARNING_FLAGS` (default `-Wall -Wextra -Wconversion -Wno-unused-parameter`), applied by `linux/CMakeLists.txt` when set.
- **Validation (from Phase 0/M0 bring-up):** 64-bit and 32-bit builds execute on host; harnesses/probes run (including 32-bit upstream DACOM/ReadFile integration harness and SDL3+Vulkan presentation PoC when 32-bit deps are available). See `linux/docs/build-and-env-status.md`.

### 2) Linux source/build organization
- **Separation maintained:**
  - `src/` and upstream Windows build inputs remain intact.
  - `linux/include/linux_port/`, `linux/src/`, `linux/tests/` contain Linux platform shims, probes, and harnesses.
  - `linux/docs/` contains bring-up, environment, spike, and planning documentation.
  - `linux/cmake/` and `linux/tools/` contain build/tooling assets.
- **Build outputs:** kept in separate build directories (e.g. `linux/build`, `linux/build-x86`, or preset dirs like `linux/linux-x64`, `linux/linux-x86`) and ignored by `.gitignore`.

### 3) Dependency graph from `.vcxproj`
- **Generator (maintainable):** `linux/tools/generate_vcxproj_dep_graph.py`
  - Parses `src/**/*.vcxproj` for `<ProjectReference>` includes and semicolon-delimited `<AdditionalDependencies>` `.lib` tokens.
  - Normalizes names (basename/path stripping, quote trimming, case-insensitive canonicalization to known project names where possible) and emits separate `project_references`, `additional_dependencies`, and `all_dependencies`.
  - Regenerate: `python3 linux/tools/generate_vcxproj_dep_graph.py`
- **Generated artifacts:**
  - `linux/docs/vcxproj-dependency-graph.json` (machine-readable)
  - `linux/docs/vcxproj-dependency-graph.md` (human-readable table)
- **Scope note:** build/reference hint graph only; not a complete runtime, interface, or COM/DACOM dependency model. System/SDK `.lib` entries from AdditionalDependencies are included as hints and are not project modules.

### 4) Minimal boot subset/module order
- **Document:** `linux/docs/minimal-boot-subset.md`
  - Defines incremental boot tiers (Tier 0 harness baseline → Tier 1 DACOM+ReadFile+path/config → Tier 2 base runtime/math/engine services → Tier 3 graphical/app bring-up).
  - Identifies evidence sources (`bin/DACOM.INI`, `bin/freelancer.ini`, vcxproj graph, probes/harnesses) and risks (ABI/calling conventions, registration order, config/path fidelity, 32-bit deps).

## Notes for code review (M0-critical/high-risk areas)

- **Build/toolchain/presets correctness:** root `CMakePresets.json`, `linux/CMakePresets.json`, `linux/cmake/toolchain-linux-x86*.cmake`, `linux/CMakeLists.txt` (preset include/binary dirs, 32-bit flags/env/pkg-config behavior, target gating).
- **Dependency graph fidelity/limitations:** `linux/tools/generate_vcxproj_dep_graph.py` and generated outputs (interpretation as hints, not runtime truth).
- **Boot-order assumptions:** `linux/docs/minimal-boot-subset.md` (should be validated/refined during M1/M2 integration).
- **Existing high-risk integration points (context):** guarded upstream wrapper integration in `src/dacom/dacom.cpp` and Linux platform shims under `linux/include/linux_port/` and `linux/src/` (reviewed/implemented across Phase 0; changes in M0 are primarily docs/tooling/build scaffolding and planning artifacts).
