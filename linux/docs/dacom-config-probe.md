# DACOM/config parsing probe (Phase 0, item #2)

Purpose: exercise and inspect boot-critical configuration (`bin/DACOM.INI`) on Linux without requiring the full Windows/DACOM runtime to be ported.

## Why
- `DACOM` loads configuration, libraries, and component composition from INI files (`SetINIConfig` / profile parser behavior in `src/dacom/dacom.cpp`).
- Understanding `[DACOM]`, `[Libraries]`, `[System]`, `[Engine]`, `[RenderManager]`, and `[RenderPipeline]` is necessary to define the minimal Linux boot subset and module-loading strategy.

## Current probe
- **Source:** `linux/tests/dacom_ini_probe.cpp`
- **CMake target:** `dacom_ini_probe` (built when `FL_LINUX_BUILD_TOOLS=ON`)

### Build & run
```bash
cmake -S linux -B linux/build
cmake --build linux/build --target dacom_ini_probe
./linux/build/dacom_ini_probe bin/DACOM.INI
```

## What it shows
- Section inventory and entry counts.
- Extracted views of boot-critical sections:
  - `[DACOM]` (e.g. `IgnoreDACOMEnv`, `DllPath`)
  - `[Libraries]` (bare DLL/module names, load order)
  - `[System]`, `[Engine]`, `[RenderManager]` (component composition)
  - `[RenderPipeline]` and related keys present in the file
- **Linux boot report:**
  - `[Libraries]` load order with candidate Linux module names (`lib<name>.so`), coarse class, and `src/` directory presence hints
  - `[System]`/`[Engine]`/`[RenderManager]` component-to-likely-provider hints
  - Suggested minimal boot tiers (to be validated against project/runtime dependencies)
- **Path/module resolution checks (best-effort, via `path_util`):**
  - normalizes/resolves `DllPath` and `RenderPipeline.MGSDB` relative to the INI runtime directory
  - checks runtime presence of Windows-named modules listed in `[Libraries]` (source-only checkouts will typically show missing binaries)
- **`.vcxproj` dependency hints (for `[Libraries]` modules):**
  - scans `src/**/*.vcxproj` and reports `ProjectReference` edges for each listed module (project-level/static-link hints)

## Limitations (intentional for now)
- This is a **simplified** INI reader, not a drop-in replacement for DACOM's built-in `IProfileParser` (`FProfileParserObject` / `FIniDatabase` in `src/dacom/dacom.cpp`).
- Not yet implemented/emulated: exact tokenization quirks, include/flag semantics, full duplicate-section lookup behavior as relied upon by all callers, or write/round-trip behavior.
- Module/provider mappings are heuristic hints (not authoritative interface ownership), and it does not load modules or instantiate components.
- `.vcxproj` parsing is string-based and reports only `ProjectReference` hints (not full include/lib/interface/runtime dependency truth).
- Linux module naming shown as candidates; early bring-up may prefer static linking or explicit init functions over `.so` plugin loading.

## Next steps for #2
1. Compare probe output against documented/expected DACOM behavior and extend parsing fidelity where boot-critical (duplicate sections, lookup rules, value nuances).
2. Expand dependency analysis beyond `ProjectReference` (e.g. `AdditionalDependencies`, known interface/runtime relationships, `freelancer.exe`/content/server chains) to produce an evidence-based minimal boot subset and compile order.
3. Add mode(s) to resolve/check additional config/data paths (e.g. entries from `bin/freelancer.ini` `[Data]`, `MGSDB`, `DATA` paths) using `path_util`.
4. Evolve toward a harness that can drive a portable/profile-parser-compatible implementation once platform shims (#1) are available.