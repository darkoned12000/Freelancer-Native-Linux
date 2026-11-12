# M6 — Gameplay bring-up (status)

**Status:** completed in working tree (pending code review, not committed).  
**Acceptance target:** Reach main menu and enter the game world.

## Summary

M6 adds the supporting infrastructure for gameplay bring-up: a null (silent) audio backend that unblocks boot/menu audio calls, an XDG-backed config/registry/save shim for Linux persistence, and a boot harness that ties together null audio, XDG config, window (M4), and graphics (M5) into a simulated boot sequence reaching a "main menu" state and entering a game world loop.

## Implemented

### Null audio backend
- `linux/include/linux_port/audio_backend.h`, `linux/src/audio_backend.cpp` (`fl_port::AudioBackend`, `fl_port::NullAudioBackend`).
- No-op audio: `init`, `shutdown`, `play_sound` (returns handle, increments counter), `stop_sound`, `stop_all`, `set_master_volume`, `update`. Unblocks boot/menu audio calls without a real audio subsystem.

### XDG-backed config/registry/save shim
- `linux/include/linux_port/xdg_config.h`, `linux/src/xdg_config.cpp` (`fl_port::XdgConfig`).
- Resolves `XDG_CONFIG_HOME`, `XDG_DATA_HOME`, `XDG_STATE_HOME`, `XDG_CACHE_HOME` (with XDG defaults under `~`).
- INI-like key/value read/write under config dir; registry-style read/write mapped to `registry.ini`; save file path/existence helpers.

### Boot harness
- `linux/tests/m6_boot_harness.cpp` (`m6_boot_harness`): demonstrates audio init, XDG config load, registry read/write, native window creation, render pipeline init, simulated boot sequence (loading resources → UI → menu → main menu ready), and a game world entry loop with rendering and audio updates.

## Harness

- **Target:** `m6_boot_harness` (built when usable target SDL3+Vulkan libs available)
- **Source:** `linux/tests/m6_boot_harness.cpp`
- **CMake:** `fl_linux_port_gameplay` static library + `m6_boot_harness`.

## Validation

- **64-bit preset (`linux-x64`):** `m6_boot_harness` → **PASS** (reached main menu state, entered game world loop).
- **32-bit preset (`linux-x86`):** `m6_boot_harness` → **PASS**.
- M1–M5 harnesses remain passing on both presets.

## Follow-on (M7+)

- Wire null audio backend into engine SoundManager interface; implement real audio backend (PulseAudio/PipeWire/ALSA).
- Deepen XDG registry shim to cover all registry keys used by client (login, settings, crash reporting).
- Wire UI/input/render into engine client (RendComp, Shading, menu system) for actual main menu rendering.
- Local server (rpclocal) integration for single-player flow.
- Mission/game initialization flow.