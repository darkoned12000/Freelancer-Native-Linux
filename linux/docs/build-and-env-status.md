# Linux build and environment status

## 64-bit host/tool builds (validated)
- Default CMake config in `linux/build`: `FL_LINUX_32BIT=OFF`, tools built and run successfully.
- Verified targets: `fl_path_resolve`, `dacom_ini_probe`, `core_shim_harness`, `abi_module_harness`, `dacom_boot_harness`, `dacom_upstream_compile_probe`.

## 32-bit bring-up build (validated compile/link)
- Host has working 32-bit compiler toolchain (`gcc -m32`/`clang -m32` succeed).
- 32-bit configure/build validated in `linux/build-x86` with:
  - `cmake -S linux -B linux/build-x86 -DFL_LINUX_32BIT=ON -DFL_LINUX_BUILD_TOOLS=ON -DFL_LINUX_BUILD_PHASE0=OFF`
  - `cmake --build linux/build-x86`
- All current Linux harness/tool targets compile and link in 32-bit mode with `-m32 -msse2 -mfpmath=sse`.

## 32-bit runtime / #4 graphics prerequisites (validated/host-dependent)
- **Validated on this host:** 32-bit binaries execute (`/lib/ld-linux.so.2` present). Example: `linux/build-x86/core_shim_harness` runs successfully (32-bit ELF i386) with shim checks passing.
- **Still required for #4 de-risk spike (separate validation):** 32-bit-capable graphics/presentation stack on target hardware/session:
  - `lib32-vulkan-radeon`/Mesa RADV ICD for RX 7900 XTX, `lib32-vulkan-icd-loader`, 32-bit Wayland client libs
  - 32-bit SDL3 (if using SDL3 PoC path), Hyprland/Wayland session compatibility
  - 32-bit `vulkaninfo`/driver enumeration confirming 32-bit presentation support (64-bit results are not sufficient)
- `pkg-config` reports system modules for `vulkan` and `wayland-client`, and 64-bit `SDL3` is present; **32-bit development/runtime availability for SDL3/Vulkan presentation should be explicitly verified** before #4 PoC implementation.

## Upstream DACOM/ReadFile compilation scope
- `dacom_upstream_compile_probe` confirms `<windows.h>` facade + shims compile common DACOM/ReadFile-style API patterns.
- **Implemented (guarded harness wrappers):** upstream `src/dacom/dacom.cpp` and `src/ReadFile/ReadFile.cpp` compile and link as Linux harness TUs/wrappers in **32-bit** mode via:
  - `linux/tests/upstream_dacom_tu.cpp` (`fl_upstream_dacom_tu`)
  - `linux/tests/upstream_readfile_tu.cpp` (`fl_upstream_readfile_tu`, includes Linux-compatible `CallDacomStricmp` wiring)
  - `linux/tests/upstream_dacom_readfile_harness.cpp` (`upstream_dacom_readfile_harness`)
  - shim/facade support in `linux/include/linux_port/*` (including `<windows.h>`, `<io.h>`, MSVC path helpers, and Win32 API/type stubs used by wrapper compilation)
  - minimal guarded adjustments in `src/dacom/dacom.cpp` for Linux wrapper TU behavior (FDUMP symbol/export handling, non-modal `DefaultDump` path, and DllMain side-effect suppression under `FL_LINUX_PORT_UPSTREAM_TU && !_WIN32`) preserving upstream Windows behavior.
- **Validated (32-bit, `linux/build-x86`):** `cmake --build linux/build-x86` builds wrapper TUs/harness successfully and `./linux/build-x86/upstream_dacom_readfile_harness` runs (`rc=0`).
- **Current limitations:** this is a compilation/integration milestone in a shimmed harness context, not a full runtime-faithful port of original DLL behavior. Many Win32 APIs remain stubbed, resources/.rc and original init/export semantics are not reproduced, and 64-bit host compilation is not a target for these upstream 32-bit-layout-dependent TUs (upstream static assertions/layout assumptions may fail outside i386). Deeper behavioral integration requires replacing stubs with Linux-backed implementations and wiring guarded module init/ABI boundaries incrementally.

## #4 environment validation and PoC results
- Host checks observed: `vulkaninfo` present; `pkg-config` reports `vulkan 1.4.363`, `wayland-client 1.26.0`, `sdl3 3.4.18`, `xkbcommon 1.13.2`.
- 32-bit components observed: 32-bit Vulkan/Wayland runtime libs in `/usr/lib32` (`libvulkan.so.1.4.363`, `libvulkan_radeon.so`, `libwayland-client.so*`), but **no 32-bit `libSDL3*.so` found under `/usr/lib32`** in probe.
- Tool/probe results (`FL_LINUX_BUILD_TOOLS`):
  - `gfx_env_probe`:
    - 64-bit (`linux/build`): SDL3 compile/runtime probe available, Vulkan headers/instance extension enumeration `ok` (`count=26`), `arch: x86_64`.
    - 32-bit (`linux/build-x86`): SDL3 correctly unavailable for target, Vulkan headers/extension enumeration `ok` (`count=26`), `arch: i386 (32-bit)`.
  - `gfx_wayland_poc` (SDL3+Vulkan, swapchain + clear-color present loop):
    - 64-bit (`linux/build`) **validated on host**: Wayland platform/driver, `AMD Radeon RX 7900 XTX (RADV NAVI31)`, swapchain created, **`presented frames: 120/120 (ok)`**.
    - 32-bit (`linux/build-x86`) not built on this host because usable 32-bit SDL3 dev/runtime is unavailable.
- **Phase 0 status:** in-repo #4 PoC implementation and scope findings (`linux/docs/irenderpipeline-rp8-scope.md`) are complete; 32-bit end-to-end execution is gated on external installation of 32-bit SDL3 (and runtime verification of 32-bit presentation on target session).
