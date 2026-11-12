#pragma once

// M6: XDG-backed config/registry/save shim for Linux bring-up.
// Maps Windows registry/config keys to files under XDG directories:
//   config:  $XDG_CONFIG_HOME/freelancer/ (default ~/.config/freelancer/)
//   data:    $XDG_DATA_HOME/freelancer/    (default ~/.local/share/freelancer/)
//   state:   $XDG_STATE_HOME/freelancer/    (default ~/.local/state/freelancer/)
//   cache:   $XDG_CACHE_HOME/freelancer/    (default ~/.cache/freelancer/)

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace fl_port {

enum class XdgDir {
  Config,
  Data,
  State,
  Cache,
};

class XdgConfig {
 public:
  XdgConfig();
  ~XdgConfig() = default;

  // Base directories (resolved from env or defaults).
  std::string base_dir(XdgDir dir) const;
  std::string app_dir(XdgDir dir, const std::string& app = "freelancer") const;

  // Ensure directory exists; returns true on success.
  bool ensure_dir(XdgDir dir, const std::string& app = "freelancer") const;

  // Simple key-value store backed by an INI-like file in the config dir.
  // Path is relative to app_dir(Config), e.g. "freelancer.ini" or "settings.cfg".
  bool read_key(const std::string& file, const std::string& section,
                const std::string& key, std::string& out) const;
  bool write_key(const std::string& file, const std::string& section,
                 const std::string& key, const std::string& value) const;

  // Registry-style helpers (maps HKCU\Software\... to config files).
  bool reg_read(const std::string& hive_path, const std::string& value_name,
                std::string& out) const;
  bool reg_write(const std::string& hive_path, const std::string& value_name,
                 const std::string& value) const;

  // Save file helpers (state dir).
  std::string save_path(const std::string& filename) const;
  bool save_exists(const std::string& filename) const;

  // Environment diagnostics.
  void print_paths() const;

 private:
  std::string getenv_or(const char* name, const std::string& fallback) const;
  std::string home() const;
  std::string config_file_path(const std::string& file) const;
  bool parse_ini(const std::string& path, const std::string& section,
                 const std::string& key, std::string& out) const;
};

}  // namespace fl_port