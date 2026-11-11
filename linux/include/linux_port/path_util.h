#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace fl_port {

struct ResolveOptions {
  // When true, append remaining components as-is after the first missing
  // directory component (useful for paths to be created). When false, failure to
  // resolve a required existing component yields std::nullopt.
  bool allow_missing_tail = false;
};

// Normalize path separators for Linux resolution.
// - Converts '\\' to '/'.
// - Collapses repeated internal '/' separators.
// - Preserves a leading '//' only when the input begins with repeated separators
//   (UNC-like); such paths are otherwise treated as unsupported for game data and
//   may not resolve meaningfully on Linux.
// - Preserves leading "./" / "../" segments textually where present.
std::string normalize_path_separators(std::string_view path);

// Resolve `path` against `base` on a case-sensitive filesystem by matching each
// path component case-insensitively when an exact match is not found.
//
// Behavior notes:
// - `base` is used when `path` is relative; if `base` is empty, current_path() is used.
// - Absolute input paths are rooted at their filesystem root.
// - Resolution is component-wise and uses cached directory listings.
// - Directory listings are collected and sorted for deterministic selection.
// - Case-insensitive comparison is ASCII-only (intentional, locale-independent).
// - If multiple directory entries collide under ASCII-lowercased comparison,
//   resolution selects a deterministic candidate (lexicographically smallest
//   actual filename) to avoid filesystem enumeration-order nondeterminism.
//   (Ambiguity reporting can be added in a future API revision.)
// - Exact component name match is preferred before case-insensitive lookup.
std::optional<std::filesystem::path> resolve_path_case_insensitive(
    const std::filesystem::path& base,
    std::string_view path,
    const ResolveOptions& options = {});

// Clear internal directory-entry caches used by case-insensitive resolution.
// Call after filesystem mutations in directories that may be re-resolved.
void clear_path_resolve_cache();

} // namespace fl_port
