#include "linux_port/path_util.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

// Phase 0 (#2) probe: lightweight parser/view of DACOM-style INI files.
// This is NOT a full replica of dacom.dll's built-in IProfileParser; it is a
// bring-up harness to inspect boot-critical sections/entries in bin/DACOM.INI
// and emit a Linux-oriented boot/load-order and dependency report.

struct IniEntry {
  std::string key;
  std::string value; // empty for bare entries (e.g. DLL names in [Libraries])
};

struct IniSection {
  std::string name;
  std::vector<IniEntry> entries;
};

static std::string trim_copy(const std::string& s) {
  size_t a = 0;
  size_t b = s.size();
  while (a < b && std::isspace(static_cast<unsigned char>(s[a]))) ++a;
  while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1]))) --b;
  return s.substr(a, b - a);
}

static std::vector<IniSection> parse_ini(const std::string& path, std::string* error) {
  std::ifstream in(path);
  if (!in) {
    if (error) *error = "failed to open file";
    return {};
  }

  std::vector<IniSection> sections;
  IniSection* current = nullptr;
  std::string raw;

  while (std::getline(in, raw)) {
    // DACOM INI uses ';' for comments.
    const size_t comment = raw.find(';');
    std::string line = (comment == std::string::npos) ? raw : raw.substr(0, comment);
    line = trim_copy(line);
    if (line.empty()) continue;

    if (line.front() == '[' && line.back() == ']' && line.size() >= 2) {
      IniSection sec;
      sec.name = line.substr(1, line.size() - 2);
      sections.push_back(std::move(sec));
      current = &sections.back();
      continue;
    }

    if (!current) continue; // ignore content before first section

    const size_t eq = line.find('=');
    if (eq == std::string::npos) {
      current->entries.push_back({line, ""});
    } else {
      IniEntry e;
      e.key = trim_copy(line.substr(0, eq));
      e.value = trim_copy(line.substr(eq + 1));
      current->entries.push_back(std::move(e));
    }
  }

  return sections;
}

static const IniSection* find_section(const std::vector<IniSection>& sections, const std::string& name) {
  // Note: DACOM lookup semantics for duplicates can be specific; probe returns first match for reporting.
  for (const auto& s : sections) {
    if (s.name == name) return &s;
  }
  return nullptr;
}

static void dump_named_section(const std::vector<IniSection>& sections, const std::string& name) {
  const IniSection* s = find_section(sections, name);
  if (!s) {
    std::cout << "\n-- [" << name << "] -- (not found)\n";
    return;
  }
  std::cout << "\n-- [" << name << "] --\n";
  for (const auto& e : s->entries) {
    if (e.value.empty()) std::cout << e.key << "\n";
    else std::cout << e.key << " = " << e.value << "\n";
  }
}

static std::string basename_no_ext_lower(std::string name) {
  const size_t slash = name.find_last_of("/\\");
  if (slash != std::string::npos) name = name.substr(slash + 1);
  const size_t dot = name.rfind('.');
  if (dot != std::string::npos) name = name.substr(0, dot);
  for (char& c : name) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return name;
}

static std::string suggest_linux_module_name(const std::string& dll_name) {
  std::string base = dll_name;
  const size_t slash = base.find_last_of("/\\");
  if (slash != std::string::npos) base = base.substr(slash + 1);
  const size_t dot = base.rfind('.');
  if (dot != std::string::npos) base = base.substr(0, dot);
  // Candidate only; actual strategy may use static linking during early bring-up.
  return "lib" + base + ".so";
}

static std::string classify_library(const std::string& dll_name) {
  const std::string b = basename_no_ext_lower(dll_name);
  if (b == "readfile" || b == "x86math" || b == "engbase" || b == "system" || b == "dacom" || b == "dalib") return "core";
  if (b == "rp8" || b == "rp9" || b == "rendcomp" || b == "shading" || b == "ximage" || b == "flmaterials" || b == "alchemy" || b == "deformable2") return "render/visual";
  if (b == "soundstreamer" || b == "soundmanager") return "audio";
  if (b == "thorn") return "scripting";
  if (b == "rpclocal" || b == "remoteclient" || b == "remoteserver" || b == "server") return "network/server";
  if (b == "content") return "gameplay/content";
  return "other";
}

static std::string likely_provider_hint(const std::string& component) {
  const std::string c = basename_no_ext_lower(component);
  if (c == "directx8") return "RP8.dll (render pipeline)";
  if (c == "directx9") return "RP9.dll (optional render pipeline)";
  if (c == "soundmanager") return "SoundManager.dll";
  if (c == "soundstreamer") return "SoundStreamer.dll";
  if (c == "texturelibrary" || c == "materialanimation" || c == "materialbatcher") return "Shading.dll";
  if (c == "alchemy" || c == "fxruntime") return "alchemy.dll";
  if (c == "vmeshlibrary") return "RendComp.dll (and/or related mesh libs)";
  if (c == "rendermanager" || c == "vmesh" || c == "trimesh" || c == "spheremesh" || c == "vmeshwire") return "RendComp.dll";
  if (c == "flappearance") return "flmaterials.dll (appearance/material integration)";
  if (c == "animation" || c == "hardpoint") return "EngBase.dll";
  if (c == "deformable") return "Deformable2.dll";
  if (c == "systemcontainer") return "system.dll";
  return "see src/include and docs/re/*";
}

static bool src_dir_exists_for_module(const std::filesystem::path& repo_root, const std::string& dll_name) {
  std::string base = dll_name;
  const size_t slash = base.find_last_of("/\\");
  if (slash != std::string::npos) base = base.substr(slash + 1);
  const size_t dot = base.rfind('.');
  if (dot != std::string::npos) base = base.substr(0, dot);
  if (base.empty()) return false;
  // Common exact folder names in this repo.
  if (std::filesystem::exists(repo_root / "src" / base)) return true;
  // Case-insensitive scan fallback (small src/ set).
  std::error_code ec;
  for (const auto& e : std::filesystem::directory_iterator(repo_root / "src", ec)) {
    if (ec) break;
    if (!e.is_directory()) continue;
    std::string n = e.path().filename().string();
    std::string a = n, b = base;
    for (char& c : a) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    for (char& c : b) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (a == b) return true;
  }
  return false;
}

static std::string get_value(const IniSection* sec, const std::string& key) {
  if (!sec) return {};
  for (const auto& e : sec->entries) {
    if (e.key == key) return e.value;
  }
  return {};
}

static std::string module_base_from_dll(const std::string& dll_name) {
  std::string base = dll_name;
  const size_t slash = base.find_last_of("/\\");
  if (slash != std::string::npos) base = base.substr(slash + 1);
  const size_t dot = base.rfind('.');
  if (dot != std::string::npos) base = base.substr(0, dot);
  return base;
}

static std::string read_file_to_string(const std::filesystem::path& p) {
  std::ifstream in(p, std::ios::binary);
  if (!in) return {};
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

static std::string extract_vcxproj_project_name(const std::string& xml, const std::filesystem::path& vcxproj_path) {
  // Prefer <ProjectName> if present, else filename stem.
  const std::string tag = "<ProjectName>";
  const size_t pos = xml.find(tag);
  if (pos != std::string::npos) {
    const size_t end = xml.find("</ProjectName>", pos + tag.size());
    if (end != std::string::npos) return trim_copy(xml.substr(pos + tag.size(), end - (pos + tag.size())));
  }
  return vcxproj_path.stem().string();
}

static std::vector<std::string> extract_project_references(const std::string& xml, const std::filesystem::path& vcxproj_path) {
  std::vector<std::string> refs;
  const std::string needle = "<ProjectReference Include=\"";
  size_t pos = 0;
  while ((pos = xml.find(needle, pos)) != std::string::npos) {
    const size_t start = pos + needle.size();
    const size_t end = xml.find('"', start);
    if (end == std::string::npos) break;
    std::string inc = xml.substr(start, end - start);
    std::replace(inc.begin(), inc.end(), '\\', '/');
    std::filesystem::path ref_path = vcxproj_path.parent_path() / inc;
    ref_path = ref_path.lexically_normal();
    refs.push_back(ref_path.stem().string());
    pos = end + 1;
  }
  std::sort(refs.begin(), refs.end());
  refs.erase(std::unique(refs.begin(), refs.end()), refs.end());
  return refs;
}

struct VcxprojInfo {
  std::filesystem::path path;
  std::string project_name;
  std::vector<std::string> refs; // by project/file stem
};

static std::map<std::string, VcxprojInfo> scan_vcxproj_graph(const std::filesystem::path& src_root) {
  std::map<std::string, VcxprojInfo> out;
  std::error_code ec;
  if (!std::filesystem::exists(src_root, ec)) return out;
  for (const auto& e : std::filesystem::recursive_directory_iterator(src_root, ec)) {
    if (ec) break;
    if (!e.is_regular_file()) continue;
    if (e.path().extension() != ".vcxproj") continue;
    const std::string xml = read_file_to_string(e.path());
    VcxprojInfo info;
    info.path = e.path();
    info.project_name = extract_vcxproj_project_name(xml, e.path());
    info.refs = extract_project_references(xml, e.path());
    const std::string key = e.path().stem().string();
    out[key] = std::move(info);
    // Also index by declared project name if different.
    if (!out[key].project_name.empty() && out[key].project_name != key) {
      // do not overwrite existing stem-keyed entry
    }
  }
  return out;
}

static std::string extract_additional_dependencies(const std::string& xml) {
  // Very simple extraction: collect tokens inside <AdditionalDependencies>...</AdditionalDependencies>
  const std::string open = "<AdditionalDependencies>";
  const std::string close = "</AdditionalDependencies>";
  size_t pos = 0;
  std::set<std::string> deps;
  while ((pos = xml.find(open, pos)) != std::string::npos) {
    const size_t end = xml.find(close, pos + open.size());
    if (end == std::string::npos) break;
    std::string body = xml.substr(pos + open.size(), end - (pos + open.size()));
    for (char& c : body) if (c == ';' || c == '\r' || c == '\n' || c == '\t') c = ' ';
    std::istringstream iss(body);
    std::string tok;
    while (iss >> tok) {
      if (tok == "%(AdditionalDependencies)") continue;
      deps.insert(tok);
    }
    pos = end + close.size();
  }
  std::string out;
  for (const auto& d : deps) {
    if (!out.empty()) out += "; ";
    out += d;
  }
  return out;
}

static void print_path_resolution_checks(const std::vector<IniSection>& sections, const std::filesystem::path& ini_path) {
  std::cout << "\n== Path/module resolution checks (best-effort) ==\n";
  const std::filesystem::path ini_abs = std::filesystem::absolute(ini_path).lexically_normal();
  const std::filesystem::path runtime_dir = ini_abs.parent_path();
  const std::filesystem::path repo_root = (runtime_dir.filename() == "bin" && std::filesystem::exists(runtime_dir.parent_path() / "src"))
                                              ? runtime_dir.parent_path()
                                              : std::filesystem::current_path();

  std::cout << "  ini: " << ini_abs.string() << "\n";
  std::cout << "  runtime_dir: " << runtime_dir.string() << "\n";
  std::cout << "  repo_root(guess): " << repo_root.string() << "\n";

  const IniSection* dacom = find_section(sections, "DACOM");
  const IniSection* rp = find_section(sections, "RenderPipeline");

  const std::string dll_path = get_value(dacom, "DllPath");
  if (!dll_path.empty()) {
    std::cout << "  DllPath: '" << dll_path << "' (normalized: " << fl_port::normalize_path_separators(dll_path) << ")\n";
    auto resolved = fl_port::resolve_path_case_insensitive(runtime_dir, dll_path);
    if (resolved) {
      std::cout << "    resolved: " << resolved->string() << (std::filesystem::exists(*resolved) ? " [exists]" : " [missing]") << "\n";
    } else {
      std::cout << "    resolved: <not found>\n";
    }
  }

  const std::string mgsdb = get_value(rp, "MGSDB");
  if (!mgsdb.empty()) {
    std::cout << "  RenderPipeline.MGSDB: '" << mgsdb << "' (normalized: " << fl_port::normalize_path_separators(mgsdb) << ")\n";
    auto resolved = fl_port::resolve_path_case_insensitive(runtime_dir, mgsdb);
    if (resolved) {
      std::cout << "    resolved: " << resolved->string() << (std::filesystem::exists(*resolved) ? " [exists]" : " [missing]") << "\n";
    } else {
      std::cout << "    resolved: <not found>\n";
    }
  }

  const IniSection* libs = find_section(sections, "Libraries");
  if (libs) {
    std::cout << "  [Libraries] runtime file presence (Windows names, best-effort):\n";
    for (const auto& e : libs->entries) {
      if (!e.value.empty()) continue;
      auto resolved = fl_port::resolve_path_case_insensitive(runtime_dir, e.key);
      if (resolved && std::filesystem::exists(*resolved)) {
        std::cout << "    - " << e.key << " => " << resolved->string() << "\n";
      } else {
        std::cout << "    - " << e.key << " => missing (expected in source-only checkout; Linux bring-up will map/load differently)\n";
      }
    }
  }
}

static void print_vcxproj_dependency_report(const std::vector<IniSection>& sections, const std::filesystem::path& ini_path) {
  std::cout << "\n== .vcxproj dependency hints ==\n";
  const std::filesystem::path ini_abs = std::filesystem::absolute(ini_path).lexically_normal();
  const std::filesystem::path runtime_dir = ini_abs.parent_path();
  std::filesystem::path repo_root = (runtime_dir.filename() == "bin" && std::filesystem::exists(runtime_dir.parent_path() / "src"))
                                        ? runtime_dir.parent_path()
                                        : std::filesystem::current_path();
  const std::filesystem::path src_root = repo_root / "src";
  if (!std::filesystem::exists(src_root)) {
    std::cout << "  src/ not found from guessed repo root: " << repo_root.string() << "\n";
    return;
  }

  const auto graph = scan_vcxproj_graph(src_root);
  const IniSection* libs = find_section(sections, "Libraries");
  std::set<std::string> in_libs;
  if (libs) {
    for (const auto& e : libs->entries) if (e.value.empty()) in_libs.insert(module_base_from_dll(e.key));
  }

  auto print_project = [&](const std::string& label, const std::string& key) {
    auto it = graph.find(key);
    if (it == graph.end()) {
      for (const auto& kv : graph) {
        std::string a = kv.first, b = key;
        for (char& c : a) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        for (char& c : b) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (a == b) { it = graph.find(kv.first); break; }
      }
    }
    if (it == graph.end()) {
      std::cout << "  - " << label << ": vcxproj not found\n";
      return;
    }
    const std::string xml = read_file_to_string(it->second.path);
    std::cout << "  - " << label << " (" << it->second.path.filename().string() << ")\n";
    if (it->second.refs.empty()) {
      std::cout << "      ProjectReference: (none found)\n";
    } else {
      std::cout << "      ProjectReference:";
      for (const auto& r : it->second.refs) {
        std::cout << " " << r;
        if (in_libs.count(r)) std::cout << "[in Libraries]";
      }
      std::cout << "\n";
    }
    const std::string deps = extract_additional_dependencies(xml);
    if (!deps.empty()) {
      std::cout << "      AdditionalDependencies: " << deps << "\n";
    }
  };

  if (libs) {
    std::cout << "  [Libraries] modules:\n";
    for (const auto& e : libs->entries) {
      if (!e.value.empty()) continue;
      print_project(module_base_from_dll(e.key), module_base_from_dll(e.key));
    }
  } else {
    std::cout << "  [Libraries] section not found.\n";
  }

  std::cout << "\n  Key runtime chain hints (project-level):\n";
  for (const char* key : {"freelancer", "rpclocal", "Content", "Server", "DALib", "Common"}) {
    print_project(key, key);
  }

  std::cout << "  Notes: ProjectReference/AdditionalDependencies are partial build-link hints; runtime behavior also depends on DACOM interfaces, load/registration order, and dynamic module loading.\n";
}

static std::vector<std::string> split_csv_like(const std::string& s) {
  std::vector<std::string> out;
  std::string cur;
  for (char c : s) {
    if (c == ',') {
      std::string t = trim_copy(cur);
      if (!t.empty()) out.push_back(t);
      cur.clear();
    } else {
      cur.push_back(c);
    }
  }
  std::string t = trim_copy(cur);
  if (!t.empty()) out.push_back(t);
  return out;
}

static void print_freelancer_ini_report(const std::filesystem::path& freelancer_ini_path) {
  std::cout << "\n== freelancer.ini checks (best-effort) ==\n";
  std::string error;
  const auto sections = parse_ini(freelancer_ini_path.string(), &error);
  if (!error.empty()) {
    std::cout << "  failed to parse: " << freelancer_ini_path.string() << " (" << error << ")\n";
    return;
  }

  const std::filesystem::path ini_abs = std::filesystem::absolute(freelancer_ini_path).lexically_normal();
  const std::filesystem::path runtime_dir = ini_abs.parent_path();
  const IniSection* fl = find_section(sections, "Freelancer");
  const IniSection* sp = find_section(sections, "Initial SP DLLs");
  const IniSection* mp = find_section(sections, "Initial MP DLLs");
  const IniSection* res = find_section(sections, "Resources");
  const IniSection* data = find_section(sections, "Data");
  const IniSection* startup = find_section(sections, "Startup");

  std::cout << "  ini: " << ini_abs.string() << "\n";
  const std::string data_path = get_value(fl, "data path");
  std::filesystem::path resolved_data_dir;
  if (!data_path.empty()) {
    std::cout << "  [Freelancer].data path: '" << data_path << "' (normalized: " << fl_port::normalize_path_separators(data_path) << ")\n";
    auto resolved = fl_port::resolve_path_case_insensitive(runtime_dir, data_path);
    if (resolved) {
      resolved_data_dir = *resolved;
      std::cout << "    resolved: " << resolved_data_dir.string() << (std::filesystem::exists(resolved_data_dir) ? " [exists]" : " [missing]") << "\n";
    } else {
      std::cout << "    resolved: <not found>\n";
    }
  }
  const std::string local_server = get_value(fl, "local_server");
  if (!local_server.empty()) {
    auto resolved = fl_port::resolve_path_case_insensitive(runtime_dir, local_server);
    std::cout << "  [Freelancer].local_server: " << local_server << " => "
              << (resolved && std::filesystem::exists(*resolved) ? resolved->string() + " [exists]" : "missing (source-only checkout expected)") << "\n";
  }

  auto print_dll_section = [&](const char* name, const IniSection* sec) {
    if (!sec) return;
    std::cout << "  [" << name << "]\n";
    for (const auto& e : sec->entries) {
      if (e.key == "path") {
        auto p = fl_port::resolve_path_case_insensitive(runtime_dir, e.value);
        std::cout << "    path=" << e.value << " => " << (p ? p->string() : "<not found>") << "\n";
        continue;
      }
      if (e.key == "DLL") {
        // format: Content.dll, Info, PRIORITY
        auto parts = split_csv_like(e.value);
        if (!parts.empty()) {
          auto dll = fl_port::resolve_path_case_insensitive(runtime_dir, parts[0]);
          std::cout << "    DLL " << parts[0];
          for (size_t i = 1; i < parts.size(); ++i) std::cout << " | " << parts[i];
          std::cout << " => " << (dll && std::filesystem::exists(*dll) ? "present" : "missing") << "\n";
        }
      }
    }
  };
  print_dll_section("Initial SP DLLs", sp);
  print_dll_section("Initial MP DLLs", mp);

  if (res) {
    std::cout << "  [Resources] (presence in runtime dir)\n";
    size_t ok = 0, miss = 0;
    for (const auto& e : res->entries) {
      if (e.key != "DLL") continue;
      auto r = fl_port::resolve_path_case_insensitive(runtime_dir, e.value);
      const bool exists = r && std::filesystem::exists(*r);
      if (exists) ++ok; else ++miss;
      std::cout << "    - " << e.value << " => " << (exists ? "present" : "missing") << "\n";
    }
    std::cout << "    summary: present=" << ok << " missing=" << miss << "\n";
  }

  if (startup) {
    std::cout << "  [Startup] movies (resolved relative to data path when available)\n";
    const std::filesystem::path base = resolved_data_dir.empty() ? runtime_dir : resolved_data_dir;
    for (const auto& e : startup->entries) {
      if (e.key != "movie_file") continue;
      auto r = fl_port::resolve_path_case_insensitive(base, e.value);
      std::cout << "    - " << e.value << " => " << (r && std::filesystem::exists(*r) ? r->string() + " [exists]" : "missing/unresolved") << "\n";
    }
  }

  if (data) {
    std::cout << "  [Data] mappings sample (resolved relative to data path when available)\n";
    const std::filesystem::path base = resolved_data_dir.empty() ? runtime_dir : resolved_data_dir;
    size_t shown = 0, exists_count = 0, total = 0;
    for (const auto& e : data->entries) {
      if (e.key.empty() || e.value.empty()) continue;
      ++total;
      auto r = fl_port::resolve_path_case_insensitive(base, e.value);
      const bool exists = r && std::filesystem::exists(*r);
      if (exists) ++exists_count;
      if (shown < 20) {
        std::cout << "    - " << e.key << " = " << e.value << " => " << (exists ? "exists" : "missing") << "\n";
        ++shown;
      }
    }
    std::cout << "    summary(sampled " << shown << " of " << total << "): exists=" << exists_count << " unresolved/missing=" << (total - exists_count) << "\n";
    std::cout << "    note: many [Data] keys repeat (e.g. equipment/voices/sounds); full per-entry validation can be added as needed.\n";
  }

  std::cout << "  note: Windows-style paths are normalized/resolved for checks; runtime integration must apply equivalent behavior in ReadFile/config/module loading.\n";
}

static void print_linux_boot_report(const std::vector<IniSection>& sections, const std::filesystem::path& ini_path) {
  std::cout << "\n== Linux boot report (Phase 0 / #2) ==\n";
  const IniSection* dacom = find_section(sections, "DACOM");
  const IniSection* libs = find_section(sections, "Libraries");
  const IniSection* system = find_section(sections, "System");
  const IniSection* engine = find_section(sections, "Engine");
  const IniSection* rendermgr = find_section(sections, "RenderManager");

  if (dacom) {
    std::cout << "[DACOM] summary:\n";
    for (const auto& e : dacom->entries) {
      if (e.value.empty()) std::cout << "  - " << e.key << "\n";
      else std::cout << "  - " << e.key << " = " << e.value << "\n";
    }
    std::cout << "  - Linux note: DllPath/module lookup must be reimplemented (dlopen/static init) with path normalization; '.\\' implies runtime directory.\n";
  }

  std::filesystem::path repo_root = ini_path.parent_path().parent_path();
  // Best-effort: if ini not under bin/, fall back to cwd.
  if (!std::filesystem::exists(repo_root / "src")) repo_root = std::filesystem::current_path();

  if (libs) {
    std::cout << "\n[Libraries] load order -> Linux mapping candidates:\n";
    int idx = 0;
    for (const auto& e : libs->entries) {
      if (!e.value.empty()) continue; // libraries are bare entries
      ++idx;
      const bool has_src = src_dir_exists_for_module(repo_root, e.key);
      std::cout << "  " << idx << ". " << e.key
                << " | candidate=" << suggest_linux_module_name(e.key)
                << " | class=" << classify_library(e.key)
                << " | src/dir=" << (has_src ? "yes" : "unknown")
                << "\n";
    }
    std::cout << "  Notes: actual bring-up may start with static linking or explicit init functions; preserve registration/load order semantics.\n";
  }

  auto print_components = [&](const char* name, const IniSection* sec) {
    if (!sec) return;
    std::cout << "\n[" << name << "] components -> likely providers:\n";
    for (const auto& e : sec->entries) {
      const std::string comp = e.value.empty() ? e.key : e.key; // entries are typically bare names
      std::cout << "  - " << comp << "  <= " << likely_provider_hint(comp) << "\n";
    }
  };
  print_components("System", system);
  print_components("Engine", engine);
  print_components("RenderManager", rendermgr);

  std::cout << "\nMinimal boot subset (suggested, to validate via dependency graph):\n";
  std::cout << "  - Tier 0 (config/core): dacom, ReadFile, path normalization + VFS shims, profile/config parsing harness\n";
  std::cout << "  - Tier 1 (engine bring-up): x86math, EngBase, system, (minimal Common/client init as needed)\n";
  std::cout << "  - Tier 2 (first visual frame): RP8/DirectX8 path behind IRP + RendComp + Shading + flmaterials + ximage (+ Deformable2 as required)\n";
  std::cout << "  - Tier 3 (runtime/gameplay): SoundManager/SoundStreamer (null-audio first), Thorn, alchemy/FxRuntime, Content/rpclocal/local-server paths\n";
  std::cout << "  Caveat: exact dependencies must be confirmed from .vcxproj graph and runtime symbol/interface requirements.\n";
}

static void print_dacom_ini_parse_notes(const std::vector<IniSection>& sections) {
  std::cout << "\n== DACOM INI parse notes (probe-level) ==\n";
  std::map<std::string, int> counts;
  for (const auto& s : sections) counts[s.name]++;
  bool any_dup = false;
  for (const auto& kv : counts) {
    if (kv.second > 1) {
      any_dup = true;
      std::cout << "  - duplicate section name observed: [" << kv.first << "] count=" << kv.second << " (probe reports first match for named lookups)\n";
    }
  }
  if (!any_dup) std::cout << "  - no duplicate section names observed in this file\n";
  std::cout << "  - parser is simplified (comments/bare entries/basic key=value); authoritative behavior: src/dacom/dacom.cpp (IProfileParser/FIniDatabase)\n";
}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Usage: dacom_ini_probe <path-to-DACOM.INI> [path-to-freelancer.ini]\n";
    std::cerr << "If freelancer.ini is omitted, the probe will try <DACOM.INI dir>/freelancer.ini when present.\n";
    return 2;
  }

  std::string error;
  const auto sections = parse_ini(argv[1], &error);
  if (!error.empty()) {
    std::cerr << "Error: " << error << ": " << argv[1] << "\n";
    return 1;
  }

  std::cout << "INI: " << argv[1] << "\n";
  std::cout << "Sections (" << sections.size() << "):\n";
  for (const auto& s : sections) {
    std::cout << "  [" << s.name << "] entries=" << s.entries.size() << "\n";
  }

  // Boot-critical views for DACOM bring-up.
  dump_named_section(sections, "DACOM");
  dump_named_section(sections, "Libraries");
  dump_named_section(sections, "System");
  dump_named_section(sections, "Engine");
  dump_named_section(sections, "RenderManager");
  dump_named_section(sections, "RenderPipeline");

  const std::filesystem::path ini_path = argv[1];
  print_linux_boot_report(sections, ini_path);
  print_path_resolution_checks(sections, ini_path);
  print_vcxproj_dependency_report(sections, ini_path);
  print_dacom_ini_parse_notes(sections);

  std::filesystem::path fl_ini;
  if (argc >= 3) {
    fl_ini = argv[2];
  } else {
    const auto candidate = std::filesystem::path(argv[1]).parent_path() / "freelancer.ini";
    if (std::filesystem::exists(candidate)) fl_ini = candidate;
  }
  if (!fl_ini.empty() && std::filesystem::exists(fl_ini)) {
    print_freelancer_ini_report(fl_ini);
  } else {
    std::cout << "\n== freelancer.ini checks (best-effort) ==\n  skipped: freelancer.ini not provided/found\n";
  }

  std::cout << "\nNote: probe parser and dependency extraction are simplified/heuristic; authoritative behavior lives in src/dacom/dacom.cpp and project build files.\n";
  return 0;
}
