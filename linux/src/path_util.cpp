#include "linux_port/path_util.h"

#include <algorithm>
#include <cctype>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace fl_port {
namespace {

struct DirCacheEntry {
  std::unordered_set<std::string> exact_names;
  // ASCII-lowercased filename -> deterministic resolved actual filename.
  std::unordered_map<std::string, std::string> lower_to_resolved;
};

std::shared_mutex g_cache_mutex;
std::unordered_map<std::string, DirCacheEntry> g_dir_cache;

char ascii_lower_char(unsigned char c) {
  if (c >= 'A' && c <= 'Z') return static_cast<char>(c + ('a' - 'A'));
  return static_cast<char>(c);
}

std::string to_lower_ascii(std::string_view s) {
  std::string out;
  out.reserve(s.size());
  for (unsigned char c : s) out.push_back(ascii_lower_char(c));
  return out;
}

DirCacheEntry build_dir_cache_entry(const std::filesystem::path& dir) {
  DirCacheEntry entry;
  std::error_code ec;
  if (std::filesystem::exists(dir, ec) && std::filesystem::is_directory(dir, ec)) {
    std::vector<std::string> names;
    for (const auto& dent : std::filesystem::directory_iterator(dir, ec)) {
      if (ec) break;
      names.push_back(dent.path().filename().string());
    }
    std::sort(names.begin(), names.end());
    for (const auto& name : names) {
      entry.exact_names.insert(name);
      // Deterministic collision policy: keep first insertion for each ASCII-
      // lowercased key (sorted names => lexicographically smallest actual name).
      const std::string lower = to_lower_ascii(name);
      entry.lower_to_resolved.emplace(lower, name);
    }
  }
  return entry;
}

// Resolve a single path component within `dir`. Lookup is performed while
// holding a shared lock so concurrent clear_path_resolve_cache() cannot destroy
// the cache entry mid-access.
std::optional<std::string> resolve_component_in_dir(const std::filesystem::path& dir,
                                                    std::string_view seg) {
  const std::string key = dir.lexically_normal().string();
  {
    std::shared_lock<std::shared_mutex> lock(g_cache_mutex);
    auto it = g_dir_cache.find(key);
    if (it != g_dir_cache.end()) {
      const auto& cache = it->second;
      const std::string seg_str(seg);
      if (cache.exact_names.find(seg_str) != cache.exact_names.end()) return seg_str;
      auto ci = cache.lower_to_resolved.find(to_lower_ascii(seg));
      if (ci != cache.lower_to_resolved.end()) return ci->second;
      return std::nullopt;
    }
  }

  // Cache miss: build outside unique lock, then insert and resolve under unique lock.
  DirCacheEntry built = build_dir_cache_entry(dir);
  std::unique_lock<std::shared_mutex> ulock(g_cache_mutex);
  auto [it, _] = g_dir_cache.emplace(key, std::move(built));
  const auto& cache = it->second;
  const std::string seg_str(seg);
  if (cache.exact_names.find(seg_str) != cache.exact_names.end()) return seg_str;
  auto ci = cache.lower_to_resolved.find(to_lower_ascii(seg));
  if (ci != cache.lower_to_resolved.end()) return ci->second;
  return std::nullopt;
}

bool is_input_sep(char c) { return c == '/' || c == '\\'; }

} // namespace

std::string normalize_path_separators(std::string_view path) {
  std::string out;
  out.reserve(path.size());

  size_t i = 0;

  // Windows drive-letter paths (e.g. C:\..., C:/...) are not native Linux
  // filesystem paths. Strip the drive prefix for bring-up compatibility so the
  // remainder is resolved relative to a chosen base/runtime root (intentionally
  // relative even if the original Windows path was absolute on that drive).
  if (path.size() >= 2 &&
      ((path[0] >= 'A' && path[0] <= 'Z') || (path[0] >= 'a' && path[0] <= 'z')) &&
      path[1] == ':') {
    i = 2;
    while (i < path.size() && is_input_sep(path[i])) ++i;
  }

  bool preserve_leading_double = false;
  if (i + 1 < path.size() && is_input_sep(path[i]) && is_input_sep(path[i + 1])) {
    // Preserve UNC-like leading '//' marker; collapse additional leading separators.
    preserve_leading_double = true;
    out = "//";
    i += 2;
    while (i < path.size() && is_input_sep(path[i])) ++i;
  }

  bool prev_sep = !out.empty() && out.back() == '/';
  for (; i < path.size(); ++i) {
    char c = path[i];
    if (c == '\\') c = '/';
    if (c == '/') {
      if (prev_sep) continue;
      out.push_back('/');
      prev_sep = true;
    } else {
      out.push_back(c);
      prev_sep = false;
    }
  }

  (void)preserve_leading_double;
  return out;
}

std::optional<std::filesystem::path> resolve_path_case_insensitive(
    const std::filesystem::path& base,
    std::string_view path,
    const ResolveOptions& options) {
  const std::string normalized = normalize_path_separators(path);
  if (normalized.empty()) return std::filesystem::path();

  const std::filesystem::path input(normalized);
  std::filesystem::path current = input.is_absolute()
                                      ? input.root_path()
                                      : (base.empty() ? std::filesystem::current_path() : base);

  std::error_code ec;

  // Fast path: exact full path exists.
  const std::filesystem::path direct = input.is_absolute() ? input : (current / input);
  if (std::filesystem::exists(direct, ec)) {
    return direct.lexically_normal();
  }

  const std::filesystem::path rel = input.is_absolute() ? input.relative_path() : input;
  bool tail_mode = false;

  for (const auto& part : rel) {
    const std::string seg = part.string();
    if (seg.empty() || seg == ".") continue;

    if (seg == "..") {
      current /= seg;
      // ".." changes the resolution base; resume lookup for subsequent components.
      tail_mode = false;
      continue;
    }

    if (tail_mode) {
      current /= seg;
      continue;
    }

    // Resolve via directory cache (avoids repeated stat() on mismatched-case
    // components). Exact-name match is preferred before ASCII-insensitive match.
    if (auto matched_name = resolve_component_in_dir(current, seg)) {
      current /= *matched_name;
      continue;
    }

    // Missing component.
    if (!options.allow_missing_tail) {
      return std::nullopt;
    }
    current /= seg;
    tail_mode = true; // append remaining components without further lookups
  }

  return current.lexically_normal();
}

void clear_path_resolve_cache() {
  std::unique_lock<std::shared_mutex> lock(g_cache_mutex);
  g_dir_cache.clear();
}

} // namespace fl_port
