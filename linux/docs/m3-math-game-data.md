# M3 — Math and game data (status)

**Status:** completed in working tree (pending code review, not committed).  
**Acceptance target:** Core data/config load without Windows APIs; path resolution behaves correctly on case-sensitive layout.

## Summary

M3 ports/isolates the `x86math` (3DMathEngine) component for Linux bring-up with explicit registration and numeric validation, and proves case-insensitive path resolution integrated into data/resource loading flows by loading core Freelancer configuration and game data from `DATA/` via DACOM/ReadFile and shimmed file APIs.

## Implemented

### `x86math` port/isolation and numeric validation
- Added a portable Linux bring-up implementation of the `I3DMathEngine`/factory/registration contract: `linux/src/x86math_portable.cpp`.
  - Preserves DACOM interface/registration semantics (`3DMathEngine`, implementation `x86`, `FMathDesc`/AGGDESC create path) and core math behavior for bring-up.
  - Handles MSVC/x86 asm and ABI-sensitive upstream implementation by isolating Linux bring-up to a portable C++ implementation (MSVC/x87 instruction-level and some numeric edge behaviors may differ from the original asm path).
  - **Alias-safe:** vector/matrix/quaternion/transform/inverse operations compute via temporaries/buffers so in-place calls with aliased `Dst`/source operands are supported.
  - **Edge-case handling:** `Sqrt(0)`/`InvSqrt(0)` return `GR_OK` with `0`; negative inputs return `GR_INVALID_PARMS` with `0` (avoids NaN/denormal hazards).
  - **`QueryInterface`:** supports `I3DMathEngine` (and component name) and `IComponentFactory` with `AddRef`; unknown interfaces return `GR_INTERFACE_UNSUPPORTED` and null out-parameter.
  - **Quaternion interpolation:** `QuatSlerpValue` is an intentional NLERP (shortest-arc lerp + normalization) approximation for bring-up, not constant-angular-velocity true SLERP.
- Added guarded Linux harness TU: `linux/tests/upstream_x86math_tu.cpp` (builds portable implementation and exposes `fl_upstream_x86math_integration_register()` for DllMain-free explicit init/registration).
- Numeric/behavior validation in `m3_math_data_tests`: matrix/vector/transform ops, inverse, matrix↔quaternion, quaternion nlerp behavior, sqrt/invsqrt sanity/edge cases, `QueryInterface`, and in-place aliasing checks.

### Path resolution integrated into data/resource loading flows
- Core config and game data access uses Linux-compatible, normalized, case-insensitive path resolution (`fl_port::resolve_path_case_insensitive` / shimmed Win32 path/file APIs) for `bin` configs and `DATA/` assets on case-sensitive filesystems.
- `FileSystem/DOS` core component instantiated against resolved `data path` root (from `freelancer.ini [Freelancer].data path`) to prove component-level data-root wiring.
- Failure-mode behavior/consistency: missing path resolution returns failure accurately.

### Core config and game data loading from `DATA/`
- Loads/resolves and reads `bin/DACOM.INI`, `bin/freelancer.ini`, and core game data files from `DATA/` (e.g. `DATA\\solar\\solararch.ini`, `DATA\\UNIVERSE\\universe.ini`) via resolved paths and shimmed file I/O.
- Behavior/consistency spot checks: config key presence (`data path`), resolved-file existence, and content signatures (Freelancer BINI or text INI as applicable).

## Harness

- **Target:** `m3_math_data_tests` (32-bit, links upstream DACOM/ReadFile wrapper TUs and x86math portable TU)
- **Source:** `linux/tests/m3_math_data_tests.cpp`
- **Supporting:** `linux/tests/upstream_x86math_tu.cpp`, `linux/src/x86math_portable.cpp`
- **CMake:** added under 32-bit upstream integration targets in `linux/CMakeLists.txt`

### Validation (host, source-only checkout)

- `./linux/build-x86/m3_math_data_tests` → **PASS** (from repo root)
  - DACOM/ReadFile/x86math explicit init and core component instantiation succeed
  - `3DMathEngine` (`x86`) created and numeric checks pass
  - `bin/DACOM.INI`, `bin/freelancer.ini`, `DATA\solar\solararch.ini`, `DATA\UNIVERSE\universe.ini` resolved and read on case-sensitive layout
  - DACOM may emit an accurate summary warning when no external `[Libraries]` runtime modules are auto-loaded in source-only explicit-registration init

## Follow-on (M4+)
- Replace/augment portable x86math with a behavior-faithful, asm/ABI-validated path where required by engine parity.
- Broaden data/resource loading coverage (additional INI chains, binary assets, UTF/archive/resource flows) through FileSystem/readfile abstractions as they are deepened.
- Extend numeric/consistency comparisons against Windows/reference behavior for sensitive math paths.