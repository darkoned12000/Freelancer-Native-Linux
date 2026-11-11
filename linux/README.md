# Linux port (Phase 0 bring-up)

This directory holds **Linux-native bring-up scaffolding** for Freelancer Decomp. The upstream project currently builds only on Windows (MSVC/Win32, DirectX). See [`../assessment.md`](../assessment.md) for the refactored assessment/roadmap.

## Objective (aligned with plan)
- **Level 1 target:** native Linux ELF binaries/modules running without Wine/Proton, preserving original game behavior, using Linux-native platform backends. D3D8-compatible rendering may be used initially **only if proven viable** in this native architecture (separate feasibility PoC required).
- **Level 2 (later):** fully native renderer behind existing interfaces if needed.

## Current Phase 0 focus (user priority: #3, #2, #1, #4)
- **#3 Path normalization (prototyped/refined):** separator normalization + deterministic, ASCII case-insensitive path resolution prototype, docs, and CLI helper.
- **#2 DACOM/config parsing probe (completed for Phase 0 scope):** `dacom_ini_probe` inspects `DACOM.INI`/`freelancer.ini`, emits boot/load-order and component/provider hints, resolves config/data paths via `path_util`, and extracts `.vcxproj` dependency hints for boot planning.
- **#1 Platform abstraction/shim skeleton (in progress):** Win32/type/COM aliases and calling-convention shims, logging/debug hooks, module-loader skeleton (`dlopen`/`path_util` mapping, `dlerror` capture), initial threading/time stubs, and 32-bit CMake toolchain skeleton.
- **#4 Graphics de-risk spike (next):** standalone Wayland + proposed D3D8-compat presentation PoC (triangle, 32-bit, Hyprland/RADV 7900 XTX), alongside evaluation of a minimal native `IRenderPipeline` backend scope.

## Status
Scaffolding and prototypes only; the full game is **not** buildable/runnable on Linux yet.

### Present in this directory
- `CMakeLists.txt` — experimental CMake entry (builds Linux helper prototypes/tools).
- `cmake/toolchain-linux-x86.cmake` — 32-bit (i686) toolchain skeleton.
- `include/linux_port/` — path utility and platform shim headers (`win32_compat.h`, `platform.h`, `module_loader.h`, `win32_file.h`, `win32_registry.h`, `win32_threads.h`, `win32_sync.h`, `win32_time.h`, `win32_string.h`, `path_util.h`, `README.md`).
- `src/` — implementations for path utility and platform shim skeleton.
- `tests/` — `fl_path_resolve`, `dacom_ini_probe`, `core_shim_harness`, `abi_module_harness`, `dacom_boot_harness`.
- `docs/` — strategy, probe details, shim/harness notes, and status.

### Key constraints/blockers
- Core modules (`src/dacom`, `src/ReadFile`) are Win32-heavy (`LoadLibrary*`, `HMODULE`, `DllMain`, Win32 file/sync APIs, calling conventions).
- Widespread Windows/DirectX/COM/MFC/RC dependencies and MSVC/x86-specific code (including inline asm) across the tree.
- Runtime configs assume Windows-style paths/module names (`..\\DATA`, `.dll`, backslashes).
- 32-bit Linux toolchain/dependencies (including Vulkan stack) must be validated early.

## Notes
- Keep changes behavior-preserving and preferably additive/guarded so Windows builds remain unaffected.
- Prefer **SDL3** for initial Wayland window/input abstraction; use **null-audio** to unblock bring-up; store user config/state via XDG-compliant locations when registry shims are introduced.
- Do not assume fake `HWND` integers satisfy engine consumers; adapt actual required behavior.
- Run from a runtime working directory analogous to `bin/` with configs/data resolvable.
