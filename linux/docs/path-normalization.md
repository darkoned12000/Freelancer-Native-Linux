# Path normalization strategy (Phase 0, item #3)

Goal: make game/config/data path resolution work reliably on Linux (case-sensitive filesystems, `/` separators) while preserving behavior expected by the original Win32/DACOM-based runtime.

## Problems observed
- **Separators:** runtime/config paths frequently use backslashes (e.g. `..\\DATA`, `solar\\solararch.ini`, `movies\\MGS_Logo_Final.wmv`, `DACOM.INI` `DllPath = .\\`).
- **Case sensitivity:** Linux filesystems are case-sensitive; assets/configs/INI lookups may vary in case (`DATA` vs `data`, mixed-case filenames in `DATA/`).
- **Relative paths / working directory:** runtime expects execution from a config/bin-like directory with relative paths (e.g. `freelancer.ini`: `data path = ..\DATA`).
- **Search paths:** DACOM/ReadFile resolve components/files through search/path mechanisms; Linux module lookup differs (`.so` vs `.dll`, `dlopen` behavior).

## Strategy

### 1. Canonical runtime layout
- `DATA/` is the canonical uppercase directory at repo root (real directory); `data -> DATA` compatibility symlink exists.
- Prefer launching from a runtime directory analogous to `bin/` so relative paths resolve predictably.

### 2. Input path normalization (string-level)
`normalize_path_separators(...)`:
- Converts `\\` to `/`.
- Strips a leading Windows drive prefix when present (e.g. `C:\...` / `C:/...`) for Linux bring-up compatibility, so the remainder can be resolved relative to a chosen base/runtime root.
- Collapses repeated internal separators.
- Preserves a leading `//` only when the input begins with repeated separators (UNC-like marker). UNC semantics are **not** a supported Linux runtime target; handle explicitly at integration boundaries if encountered.
- Intended for filesystem path strings (not arbitrary data strings).

### 3. Case-insensitive resolution (filesystem-level fallback)
`resolve_path_case_insensitive(base, path, options)`:
- Fast path: return exact normalized path if it exists.
- Otherwise resolve **component-by-component** from `base` (or `current_path()` for relative paths when base is empty).
- For each component: prefer **exact** name, then ASCII case-insensitive match from a cached directory listing.
- **Deterministic directory cache:** directory entries are collected and sorted before building lookup maps, so results do not depend on `directory_iterator` order.
- **ASCII-only case folding:** locale-independent (`A-Z` only) for predictable behavior.
- **Collision handling:** if multiple entries map to the same ASCII-lowercased key, resolver selects a deterministic candidate (lexicographically smallest actual filename). Future improvement: explicit ambiguity reporting/logging API.
- `allow_missing_tail`: if an existing component cannot be matched, append the remaining components as-is (single-pass tail mode) for create-path scenarios.
- Cache invalidation: `clear_path_resolve_cache()` must be called after filesystem mutations in directories that may be re-resolved.

### 4. Integration points (planned)
- **ReadFile / VFS shim:** apply normalization + resolution in open/stat/enumeration paths (kept separate from raw file I/O primitives).
- **DACOM library loading:** normalize `DllPath`/module names and map Windows DLL names/extensions to Linux modules where needed.
- **INI/config consumers:** preserve parse behavior; feed normalized/resolved paths into platform/filesystem APIs.
- **Harness/tools:** use resolver CLI for debugging missing assets and config path issues.

## Prototype (current)
- `linux/include/linux_port/path_util.h`
- `linux/src/path_util.cpp`
- `linux/tests/path_resolve_main.cpp` (`fl_path_resolve`)

Implementation notes (refined): deterministic sorted directory caching with `exact_names` + ASCII-lower lookup map (preserves exact-match preference even when case-fold collisions exist), locale-independent ASCII case folding, cache-based component resolution (avoids per-component `stat()` misses), reduced lock scope around directory traversal, and single-pass `allow_missing_tail` handling. Drive-letter prefixes are stripped during normalization for bring-up compatibility (stripped paths resolve relative to the chosen base/runtime root). Cache internals are not exposed across lock boundaries.

Example:
```bash
cmake -S linux -B linux/build
cmake --build linux/build --target fl_path_resolve
./linux/build/fl_path_resolve bin '..\\DATA\\solar\\solararch.ini'
```

## Planned tests (to add before engine integration reliance)
- Exact match, case-mixed filename, case-mixed directory, mixed separators
- Relative paths against base, `.` / `..` handling
- Absolute Linux paths
- Missing final component with/without `allow_missing_tail`
- Missing intermediate directory behavior
- Ambiguous case-colliding entries (deterministic result + future reporting)
- Symlinked components/directories
- Cache invalidation after directory changes
- Non-ASCII filename policy cases (documented/explicit)

## Notes
- Resolver is **not** a full Windows path semantic emulator; it is a compatibility aid for game data/config/module lookup.
- Do not silently reinterpret UNC/drive paths; handle explicitly at integration boundaries.
