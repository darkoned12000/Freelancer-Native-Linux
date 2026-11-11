# Minimal Boot Subset & Module Order (M0)

This document defines the **smallest, evidence-based boot subset** and initial module order for Linux bring-up, derived from runtime configuration (`bin/DACOM.INI`, `bin/freelancer.ini`), the generated `.vcxproj` dependency graph (`linux/docs/vcxproj-dependency-graph.md`), and current harness/probe findings. It is a planning artifact for M1/M2 integration (not a claim that the full game boots).

## Evidence sources

- **Runtime composition/load hints:** `bin/DACOM.INI` (`[Libraries]`, `[System]`, `[Engine]`, `[RenderManager]`, `[RenderPipeline]`) and `bin/freelancer.ini` (`[Initial SP DLLs]`, `[Resources]`, `data path`, `[Data]`).
- **Build/reference hints:** `linux/docs/vcxproj-dependency-graph.{json,md}` generated from `src/**/*.vcxproj` (`<ProjectReference>` and `<AdditionalDependencies>` `.lib` tokens; normalized/canonicalized as project refs, additional deps, and union).
- **Bring-up probes/harnesses:** `dacom_ini_probe`, `upstream_dacom*_integration_harness`, `gfx_env_probe`/`gfx_wayland_poc` (Phase 0).

> Note: the vcxproj graph is not a complete runtime/interface/DACOM dependency model. Actual bring-up must be validated against registration order, COM/DACOM interface availability, and behavior.

## Boot tiers (incremental)

### Tier 0 — Harness bring-up (completed in Phase 0 scope)
**Goal:** exercise core contracts without full engine/runtime module graph.

- `linux_port` platform shims (paths, Win32 compat/file/threads/sync/time/string/debug, module-loader abstraction, logging/FDUMP bridge)
- `path_util` (normalization + case-insensitive resolution)
- Guarded upstream wrapper integration: `dacom.cpp`, `ReadFile.cpp` (32-bit harness TUs) with explicit registration/init hooks
- Config/file-backed reads against real `bin/*.INI` and `DATA/` paths

**Validation:** `core_shim_harness`, `dacom_boot_harness`, `upstream_dacom_readfile_integration_harness` (and related targets).

### Tier 1 — Core component system + file/config access (first true boot slice)
**Goal:** initialize DACOM, parse config, and make file/data access work for Linux paths.

1. **`dacom`** (foundation; component manager, INI/profile parsing, factory/instance/composition APIs)
2. **`ReadFile`** (file system providers; required early per `bin/DACOM.INI`)
3. **Path/config integration**: resolve `bin/DACOM.INI`, `bin/freelancer.ini`, `DllPath`, `MGSDB`, `data path = ..\DATA` via `path_util` in runtime-facing paths
4. **Minimal registration/lifecycle model:** DllMain-free explicit init/registration for statically-linked or harness-loaded modules

**Rationale:** `bin/DACOM.INI` lists `ReadFile.dll` first under `[Libraries]`. Vcxproj refs show `ReadFile -> dacom`. Most modules depend on `dacom`/`DALib`/`Common`.

**Acceptance direction (M1/M2):** harness/executable initializes DACOM, applies config, registers/creates core file/config components, and resolves/reads required files.

### Tier 2 — Base runtime/math/engine services (minimal engine init)
**Goal:** satisfy core `[System]`/`[Engine]` prerequisites that do not require full rendering/audio/content.

- **`x86math`** (listed early in `DACOM.INI`; provides 3D math engine; ABI/asm-sensitive — isolate/validate)
- **`EngBase`**, **`system`** (core engine/system services; both depend on `dacom` per graph/hints)
- **`DALib`** / **`Common`** as required by dependent modules and executable bring-up (Common depends on DALib, dacom, qhull; DALib depends on dacom, zlib, winmm)

**Order principle:** load/initialize in dependency-safe order: `dacom` → `ReadFile` → `x86math` → (`DALib`/support libs as needed) → `Common`/`EngBase`/`system` → higher components.

### Tier 3 — Minimal graphical/app bring-up (later milestones)
**Goal:** reach a native window/event loop and minimal render/present path (not required for M0).

- Window/input abstraction (SDL3 Wayland-first) and adapters for engine expectations
- Render path: standalone PoC proven in Phase 0; in-engine integration behind `IRenderPipeline` (M5), informed by `IRenderPipeline.h`/`RP8` scope
- Start with **null-audio** to avoid blocking on DirectSound/ACM; add real audio later (M6)
- Defer/stub initially where safe: movies/IME, MFC `FLServer`, nonessential networking/UI extras, resource-DLL-heavy flows not required for first frames/menu bring-up

## Suggested first executable bring-up subset (target)

For the first Linux-native bring-up executable/harness beyond isolated tests, prefer a **statically-linked or explicitly-registered** minimal set:

1. `dacom` + profile/config parsing
2. `ReadFile` (DOS/UTF path resolution compatible with Linux `path_util` integration)
3. `x86math` (or a validated portable shim/replacement path if required)
4. Minimal `EngBase`/`system` services needed by selected init path
5. Minimal app harness that: sets runtime roots (`bin/`, `DATA/`), loads/applies `DACOM.INI` (`SetINIConfig`/equivalent), registers core components, and performs targeted `CreateInstance`/init smoke checks

Exclude from the first target until Tier 1-2 are stable: full `RP8` device/render behavior, audio backends, `Thorn`/script-heavy flows, `Content`/gameplay/UI stacks, server/MFC tools, and retail-blob-dependent launch gates (handle via stubs/bypass where appropriate and behavior-preserving).

## Known risks / validation notes

- **ABI/calling conventions (32-bit x86):** COM-style `__stdcall`/`__thiscall`/struct layout and vtable assumptions must be validated for upstream TUs and interfaces.
- **Module registration/init:** Windows `DllMain`/implicit registration must be replaced with explicit, ordered init hooks.
- **Config/path fidelity:** Windows separators, relative paths, and case-insensitive lookup must be preserved semantically on Linux.
- **Dependency graph limitations:** `.vcxproj` refs omit many runtime/interface/service dependencies; use `DACOM.INI`, interface contracts, and runtime probes as authoritative signals.
- **Environment deps:** 32-bit multilib toolchain/libs and (for graphics) 32-bit SDL3/Vulkan/Wayland stacks must be configured correctly (see `README.md`, `linux/docs/build-and-env-status.md`).
