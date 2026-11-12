# M8 — Game client integration (living document)

**Status:** not started. Planning artifact.  
**Acceptance target:** The Freelancer game client (`src/freelancer`) compiles and links as a native Linux ELF and reaches a controlled early boot point (M8.5).

## Overview

M8 is the transition from isolated harnesses to the real engine client. Everything before M8
(M0–M7) was bring-up infrastructure: build system, platform shims, DACOM init, path
resolution, window/input, graphics backend, audio/config shims, and validation harnesses.
M8 is where the actual port begins.

## Sub-phases

See `roadmap.md` M8 section for the full task breakdown. Summary:

- **M8.0** — Preparation & inventory: dependency graph, call-site inventory, CMake skeleton
- **M8.1** — Minimal linkable client skeleton: Linux entry point, stubs, link
- **M8.2** — Core engine initialization: DACOM + config/DATA through real client paths
- **M8.3** — Window + input + render hookup: SDL3 window, input mapping, Vulkan clear-present
- **M8.4** — Audio / config / persistence unblock
- **M8.5** — Reach main menu / controlled state
- **M8.6** — Stabilization & handoff to M9

## Risk hotspots

- Hidden global constructors / static initialization order
- Remaining COM / QueryInterface usage assuming Windows apartment or vtable layouts
- Resource loading (.rc / string tables / cursors)
- Code assuming HWND is a real window handle or classic Win32 message pump
- Float/math edge cases only appearing under the full client
- 32-bit library availability (SDL3, Vulkan loader, etc.)

## Known stubs / deviations

(To be populated as M8 progresses.)

## Artifacts

- CMake target: `freelancer_linux` (to be created in M8.0)
- Smoke-test script: (to be created in M8.6)
- Known-stubs/deviations list: (this document, section above)