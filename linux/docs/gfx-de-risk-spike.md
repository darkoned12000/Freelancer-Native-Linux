# #4 Graphics de-risk spike (scaffold + environment validation)

Status: **in progress (prereq validation + PoC scaffolding added)**. #1 completion unblocks early #4 validation work.

## Objectives
- Validate **32-bit** Linux graphics/runtime stack on target host (Hyprland/Wayland, RADV, RX 7900 XTX).
- Run a standalone **Wayland-capable presentation PoC** (window + surface + swapchain/present/clear or triangle) in a 32-bit environment if dependencies are available.
- In parallel, evaluate the scope of a **minimal native `IRenderPipeline` backend** as an alternative/fallback to D3D8-compat integration.

## Immediate checks (host prerequisites)
- 32-bit toolchain/multilib: `gcc-multilib`/clang 32-bit support, 32-bit libc.
- 32-bit graphics stack (as applicable): `lib32-vulkan-radeon`/Mesa RADV, `lib32-vulkan-icd-loader`, `vulkan-tools` (and 32-bit `vulkaninfo` path if separate), Wayland client libs, SDL3 32-bit libs (if SDL3 path chosen).
- Verify GPU enumeration under 32-bit runtime (do not assume 64-bit Vulkan results apply).

## Implemented scaffolding/probes
- `linux/tests/gfx_env_probe.cpp` + CMake target `gfx_env_probe`: reports compile/runtime availability of SDL3/Vulkan (when usable for target arch), init/extension enumeration smoke, and target arch.
- `linux/tests/gfx_wayland_poc.cpp` + CMake target `gfx_wayland_poc` (conditional): standalone **SDL3 + Vulkan** PoC that creates a Wayland-preferred window, Vulkan instance/surface, selects device/queues, creates swapchain/command submission primitives, and presents a **clear-color loop** (120 frames).
- `linux/docs/irenderpipeline-rp8-scope.md`: initial `IRenderPipeline`/`RP8` backend-scope and feasibility findings (Level 1 compat vs Level 2 native-backend subset strategy).
- CMake (`FL_LINUX_BUILD_TOOLS`) probes `pkg-config` modules `sdl3` and `vulkan` with target-arch usability checks:
  - builds `gfx_env_probe` (with detected defs/includes/libs when usable)
  - builds `gfx_wayland_poc` only when usable target `sdl3` and `vulkan` libraries are available

## Validation results (this environment)
- **64-bit host (`linux/build`, Wayland session):** `gfx_env_probe` and `gfx_wayland_poc` build and run. `gfx_wayland_poc` validated Wayland/Vulkan path on `AMD Radeon RX 7900 XTX (RADV NAVI31)` with swapchain creation and `presented frames: 120/120 (ok)`.
- **32-bit target (`linux/build-x86`):** `gfx_env_probe` builds/runs and reports Vulkan headers/extension enumeration OK on i386, but **32-bit SDL3 runtime/dev libraries are not installed/usable on this host** (`/usr/lib32/libSDL3*` absent in probe), so `gfx_wayland_poc` is not built for 32-bit here.

## Remaining external dependency for 32-bit end-to-end PoC execution
- Install/verify 32-bit SDL3 dev/runtime (and ensure pkg-config/library resolution targets 32-bit), then build/run `gfx_env_probe` and `gfx_wayland_poc` in `linux/build-x86` to validate the 32-bit Wayland/RADV presentation path.
- The in-repo PoC implementation is **code-complete for swapchain clear-present validation**; remaining gap is host package availability for the 32-bit SDL3 link/runtime, not PoC code.

## `IRenderPipeline` / `RP8` evaluation notes
- Inspect `src/include/render/IRenderPipeline.h`, `src/RP8/`, and render/material contracts (`RendComp`, `Shading`, `flmaterials`) to identify the minimal subset for boot/menu/first-scene rendering.
- Compare effort/risk: native ELF D3D8 translation integration vs scoped native backend implementing required pipeline contracts.

## Current blockers/dependencies
- 32-bit SDL3/Vulkan/Wayland dev/runtime packages and verified RADV presentation on Hyprland in 32-bit mode (environment-dependent). This spike should proceed once those prerequisites are confirmed.