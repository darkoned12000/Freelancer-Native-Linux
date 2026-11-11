# M2 — DACOM and engine initialization (status)

**Status:** completed in working tree (pending code review, not committed).  
**Acceptance target:** Linux harness/executable initializes the component system and loads core components successfully.

## Summary

M2 adapts the DACOM/component initialization flow for Linux bring-up in a DllMain-free, explicit-registration model and proves component system init plus minimal core component load from harnessed upstream DACOM/ReadFile integration.

## Implemented

### DllMain-free explicit initialization/lifecycle
- Upstream DACOM wrapper TU (`linux/tests/upstream_dacom_tu.cpp`) provides explicit harness hooks:
  - `fl_upstream_dacom_install_fdump_bridge()` — installs FDUMP bridge for Linux diagnostics
  - `fl_upstream_dacom_register_builtin_components()` — explicitly registers DACOM built-ins normally registered during `DLL_PROCESS_ATTACH` (DllMain renamed/disabled in wrapper TU), e.g. `RegExp`
- ReadFile explicit registration hook remains available (`fl_upstream_readfile_integration_register()` in `linux/tests/upstream_readfile_tu.cpp`), simulating module attach without DllMain.

### Config/INI loading through Linux-compatible paths
- `linux/tests/m2_engine_init_harness.cpp` resolves `bin\DACOM.INI`, `bin\freelancer.ini`, and `freelancer.ini` `[Freelancer].data path` (e.g. `..\DATA`) using `fl_port::resolve_path_case_insensitive` / normalized path handling on case-sensitive Linux.
- DACOM config is loaded via `ICOManager::SetINIConfig` using a Linux-resolved/effective init config path; file-backed access to resolved configs and `DATA` assets is exercised via shimmed Win32 file APIs.
- **Source-only checkout handling (accurate, not suppressed):** runtime module binaries listed in `DACOM.INI [Libraries]` are not shipped. For explicit-registration bring-up, the harness uses an effective DACOM init config with `[Libraries]` auto-load entries removed to avoid incorrect attempts to load unavailable external modules; compiled-in core components are registered explicitly in dependency order. DACOM may still emit an accurate summary warning when no libraries are loaded from `[Libraries]`.

### Minimal core component load and registration/composition
- Dependency-ordered init demonstrated in `m2_engine_init_harness`:
  1. DACOM manager acquisition (`DACOM_Acquire`)
  2. Explicit DACOM built-in registration (DllMain-free)
  3. DACOM config load through Linux-compatible path
  4. Built-in component creation/composition smoke (`CreateInstance`/use of `RegExp`)
  5. Explicit ReadFile module registration
  6. Core component load/creation probe (`FileSystem/DOS`) and Linux-compatible config/file access checks
- Preserves DACOM interface/registration semantics as exercised by upstream manager APIs (`RegisterComponent`, `CreateInstance`, component usage/release).

## Harness

- **Target:** `m2_engine_init_harness` (32-bit, links upstream DACOM/ReadFile wrapper TUs)
- **Source:** `linux/tests/m2_engine_init_harness.cpp`
- **CMake:** added under 32-bit upstream integration targets in `linux/CMakeLists.txt`

### Validation (host, source-only checkout)

- `./linux/build-x86/m2_engine_init_harness` → **PASS**
  - Resolves `bin/DACOM.INI`, `bin/freelancer.ini`, and `DATA` path
  - Initializes DACOM manager, loads config via Linux-compatible path, registers built-ins + ReadFile explicitly
  - Proves `RegExp` create/compile/search (registration/composition) and performs resolved config/file reads
  - `DACOM CreateInstance(FileSystem/DOS)` succeeds (ReadFile core component instantiated) with correct `FFileSystemDesc` sizing/fields; harness fails if core component instantiation does not succeed
- Existing 32-bit upstream integration harness (`upstream_dacom_readfile_integration_harness`) remains **overall: ok**.

## Follow-on (M3+ / integration)
- Deepen behavior-faithful ReadFile/FileSystem instance creation and component wiring in shimmed context.
- Replace harness-effective `[Libraries]` handling with full Linux module init/registration flow as runtime module artifacts or static/explicit registrations become available.
- Expand config variants, lifecycle/shutdown semantics, and multi-component dependency/order coverage as more modules are brought up.
