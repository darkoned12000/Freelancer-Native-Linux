// M6: XDG-backed config/registry/save shim.

#include "linux_port/xdg_config.h"
#include "linux_port/platform.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace fl_port {

namespace fs = std::filesystem;

XdgConfig::XdgConfig() = default;

std::string XdgConfig::home() const {
  const char* h = std::getenv("HOME");
  if (h && *h) return h;
  return "/tmp";
}

std::string XdgConfig::getenv_or(const char* name, const std::string& fallback) const {
  const char* v = std::getenv(name);
  if (v && *v) return v;
  return fallback;
}

std::string XdgConfig::base_dir(XdgDir dir) const {
  std::string h = home();
  switch (dir) {
    case XdgDir::Config: return getenv_or("XDG_CONFIG_HOME", h + "/.config");
    case XdgDir::Data:   return getenv_or("XDG_DATA_HOME", h + "/.local/share");
    case XdgDir::State:  return getenv_or("XDG_STATE_HOME", h + "/.local/state");
    case XdgDir::Cache:  return getenv_or("XDG_CACHE_HOME", h + "/.cache");
  }
  return h + "/.config";
}

std::string XdgConfig::app_dir(XdgDir dir, const std::string& app) const {
  return base_dir(dir) + "/" + app;
}

bool XdgConfig::ensure_dir(XdgDir dir, const std::string& app) const {
  std::error_code ec;
  fs::create_directories(app_dir(dir, app), ec);
  return !ec;
}

std::string XdgConfig::config_file_path(const std::string& file) const {
  std::string norm = fl_port::normalize_path_separators(file);
  return app_dir(XdgDir::Config) + "/" + norm;
}

bool XdgConfig::parse_ini(const std::string& path, const std::string& section,
                          const std::string& key, std::string& out) const {
  std::ifstream in(path);
  if (!in) return false;
  std::string line, current;
  auto trim = [](std::string s) {
    size_t a = 0, b = s.size();
    while (a < b && (s[a]==' '||s[a]=='\t'||s[a]=='\r'||s[a]=='\n')) ++a;
    while (b > a && (s[b-1]==' '||s[b-1]=='\t'||s[b-1]=='\r'||s[b-1]=='\n')) --b;
    return s.substr(a, b-a);
  };
  auto lower = [](std::string s){ for(char&c:s) if(c>='A'&&c<='Z') c=char(c-'A'+'a'); return s; };
  std::string sec_want = lower(section);
  std::string key_want = lower(key);
  while (std::getline(in, line)) {
    auto sc = line.find(';');
    std::string s2 = trim(sc==std::string::npos?line:line.substr(0,sc));
    if (s2.empty()) continue;
    if (s2.front()=='[' && s2.back()==']') { current = lower(trim(s2.substr(1,s2.size()-2))); continue; }
    if (current != sec_want) continue;
    auto eq = s2.find('=');
    if (eq == std::string::npos) continue;
    if (lower(trim(s2.substr(0,eq))) == key_want) { out = trim(s2.substr(eq+1)); return true; }
  }
  return false;
}

bool XdgConfig::read_key(const std::string& file, const std::string& section,
                         const std::string& key, std::string& out) const {
  return parse_ini(config_file_path(file), section, key, out);
}

bool XdgConfig::write_key(const std::string& file, const std::string& section,
                          const std::string& key, const std::string& value) const {
  ensure_dir(XdgDir::Config);
  std::string path = config_file_path(file);
  std::error_code ec;
  fs::create_directories(fs::path(path).parent_path(), ec);
  // Read existing lines
  std::vector<std::string> lines;
  std::ifstream in(path);
  std::string line;
  while (std::getline(in, line)) lines.push_back(line);
  in.close();

  auto trim = [](std::string s) {
    size_t a = 0, b = s.size();
    while (a < b && (s[a]==' '||s[a]=='\t'||s[a]=='\r'||s[a]=='\n')) ++a;
    while (b > a && (s[b-1]==' '||s[b-1]=='\t'||s[b-1]=='\r'||s[b-1]=='\n')) --b;
    return s.substr(a, b-a);
  };
  auto lower = [](std::string s){ for(char&c:s) if(c>='A'&&c<='Z') c=char(c-'A'+'a'); return s; };

  // Find section and key
  std::string sec_want = lower(section);
  std::string key_want = lower(key);
  int section_idx = -1, key_idx = -1;
  std::string current;
  for (size_t i = 0; i < lines.size(); ++i) {
    std::string raw = lines[i];
    // Strip trailing comments for detection, but keep raw line for output.
    auto sc = raw.find(';');
    auto hc = raw.find('#');
    size_t cp = std::string::npos;
    if (sc != std::string::npos) cp = sc;
    if (hc != std::string::npos && (cp == std::string::npos || hc < cp)) cp = hc;
    std::string t = trim(cp == std::string::npos ? raw : raw.substr(0, cp));
    if (!t.empty() && t.front()=='[' && t.back()==']') {
      current = lower(trim(t.substr(1,t.size()-2)));
      if (current == sec_want && section_idx == -1) section_idx = (int)i;
      continue;
    }
    if (current == sec_want) {
      auto eq = t.find('=');
      if (eq != std::string::npos && lower(trim(t.substr(0,eq))) == key_want) {
        if (key_idx == -1) key_idx = (int)i;
      }
    }
  }

  if (key_idx >= 0) {
    lines[key_idx] = key + "=" + value;
  } else if (section_idx >= 0) {
    // Insert after section header
    lines.insert(lines.begin() + section_idx + 1, key + "=" + value);
  } else {
    if (!lines.empty() && !trim(lines.back()).empty()) lines.push_back("");
    lines.push_back("[" + section + "]");
    lines.push_back(key + "=" + value);
  }

  std::ofstream out(path, std::ios::trunc);
  if (!out) return false;
  for (auto& l : lines) out << l << "\n";
  return out.good();
}

bool XdgConfig::reg_read(const std::string& hive_path, const std::string& value_name,
                         std::string& out) const {
  std::string section = fl_port::normalize_path_separators(hive_path);
  while (!section.empty() && (section.front()=='/' || section.front()=='\\')) section.erase(section.begin());
  return read_key("registry.ini", section, value_name, out);
}

bool XdgConfig::reg_write(const std::string& hive_path, const std::string& value_name,
                          const std::string& value) const {
  std::string section = fl_port::normalize_path_separators(hive_path);
  while (!section.empty() && (section.front()=='/' || section.front()=='\\')) section.erase(section.begin());
  return write_key("registry.ini", section, value_name, value);
}

std::string XdgConfig::save_path(const std::string& filename) const {
  ensure_dir(XdgDir::State);
  std::string norm = fl_port::normalize_path_separators(filename);
  std::string full = app_dir(XdgDir::State) + "/" + norm;
  std::error_code ec;
  fs::create_directories(fs::path(full).parent_path(), ec);
  return full;
}

bool XdgConfig::save_exists(const std::string& filename) const {
  std::error_code ec;
  return fs::exists(save_path(filename), ec);
}

void XdgConfig::print_paths() const {
  std::cout << "[fl-port][xdg] config=" << app_dir(XdgDir::Config) << "\n";
  std::cout << "[fl-port][xdg] data=" << app_dir(XdgDir::Data) << "\n";
  std::cout << "[fl-port][xdg] state=" << app_dir(XdgDir::State) << "\n";
  std::cout << "[fl-port][xdg] cache=" << app_dir(XdgDir::Cache) << "\n";
}

}  // namespace fl_port