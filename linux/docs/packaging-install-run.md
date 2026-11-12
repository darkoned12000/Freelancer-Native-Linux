# Packaging, install & run (M7)

Native Linux bring-up release candidate notes. No Wine/Proton required.

## Dependencies

### 64-bit build (linux-x64)
- CMake 3.20+, Ninja, Clang or GCC
- SDL3 development libraries
- Vulkan development libraries + a Vulkan ICD (Mesa RADV recommended on AMD)
- Wayland client libraries (or X11 fallback)

### 32-bit build (linux-x86)
- All of the above in 32-bit/multilib form
- `lib32-sdl3`, `lib32-vulkan-icd-loader`, `lib32-mesa` (RADV), `lib32-wayland`, `lib32-libxkbcommon`, `lib32-libdecor`
- Multilib toolchain (`gcc-multilib` or clang with `-m32`)

## Build

From the repository root:

```bash
# 64-bit (recommended for host validation)
cmake --preset linux-x64 -S linux
cmake --build --preset linux-x64 -j

# 32-bit (target ABI for initial port)
cmake --preset linux-x86 -S linux
cmake --build --preset linux-x86 -j
```

The `linux-x86` preset sets `PKG_CONFIG_LIBDIR=/usr/lib32/pkgconfig:/usr/share/pkgconfig` for 32-bit SDL3/Vulkan discovery.

## Run

```bash
# 64-bit
./linux/linux-x64/m7_release_harness

# 32-bit
./linux/linux-x86/m7_release_harness
```

Other harnesses:
```bash
./linux/linux-x64/m4_window_input_harness   # window + input
./linux/linux-x64/m5_graphics_harness       # graphics (clear+present)
./linux/linux-x64/m6_boot_harness           # gameplay boot sequence
```

## Install (manual)

There is no install target yet. To run on a clean system:

1. Install dependencies (see above).
2. Clone the repository.
3. Place game data in `DATA/` at the repo root (user-provided, legally obtained).
4. Build with the appropriate preset.
5. Run the desired harness or (eventually) the game client.

## XDG directories

The port uses XDG base directories for configuration and state:
- Config: `$XDG_CONFIG_HOME/freelancer/` (default `~/.config/freelancer/`)
- Data: `$XDG_DATA_HOME/freelancer/` (default `~/.local/share/freelancer/`)
- State: `$XDG_STATE_HOME/freelancer/` (default `~/.local/state/freelancer/`)
- Cache: `$XDG_CACHE_HOME/freelancer/` (default `~/.cache/freelancer/`)

## Display/input modes

- Windowed and fullscreen modes are supported via `Window::set_fullscreen()` / `toggle_fullscreen()`.
- Wayland is preferred; X11 fallback is available via `SDL_VIDEODRIVER=x11`.
- Controller detection via SDL gamepad API (hotplug supported).

## Known limitations (M7)

- Render pipeline is clear-only (no vertex buffer/pipeline draw submission yet).
- Audio is null (silent) backend only.
- XDG registry shim covers basic read/write; full registry key coverage is follow-on.
- Game client (`freelancer`) is not yet compiled for Linux; harnesses prove subsystem readiness.
- 32-bit SDL3/Vulkan runtime must be installed for 32-bit targets.