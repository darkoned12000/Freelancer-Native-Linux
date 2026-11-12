#pragma once

// Win32 registry shim for Linux bring-up.
// Runtime persistence/config maps to XDG-compliant storage via fl_port::XdgConfig.
// Bring-up scope: basic key/value persistence (REG_SZ/REG_DWORD/REG_BINARY best-effort),
// logical handle tracking, and thread-safe handle table access. Not full Win32 semantics.

#if defined(_WIN32)
#include <windows.h>
#else

#include "linux_port/win32_compat.h"
#include "linux_port/xdg_config.h"
#include "linux_port/path_util.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <map>
#include <mutex>
#include <sstream>
#include <string>

using HKEY = void*;
using REGSAM = DWORD;
using LSTATUS = LONG;

#ifndef HKEY_CLASSES_ROOT
#  define HKEY_CLASSES_ROOT ((HKEY)(intptr_t)0x80000000)
#endif
#ifndef HKEY_CURRENT_USER
#  define HKEY_CURRENT_USER ((HKEY)(intptr_t)0x80000001)
#endif
#ifndef HKEY_LOCAL_MACHINE
#  define HKEY_LOCAL_MACHINE ((HKEY)(intptr_t)0x80000002)
#endif
#ifndef HKEY_USERS
#  define HKEY_USERS ((HKEY)(intptr_t)0x80000003)
#endif

#ifndef ERROR_SUCCESS
#  define ERROR_SUCCESS 0L
#endif
#ifndef ERROR_FILE_NOT_FOUND
#  define ERROR_FILE_NOT_FOUND 2L
#endif
#ifndef ERROR_ACCESS_DENIED
#  define ERROR_ACCESS_DENIED 5L
#endif
#ifndef ERROR_INVALID_PARAMETER
#  define ERROR_INVALID_PARAMETER 87L
#endif
#ifndef ERROR_MORE_DATA
#  define ERROR_MORE_DATA 234L
#endif
#ifndef ERROR_NOT_SUPPORTED
#  define ERROR_NOT_SUPPORTED 50L
#endif

#ifndef REG_NONE
#  define REG_NONE 0
#endif
#ifndef REG_SZ
#  define REG_SZ 1
#endif
#ifndef REG_EXPAND_SZ
#  define REG_EXPAND_SZ 2
#endif
#ifndef REG_BINARY
#  define REG_BINARY 3
#endif
#ifndef REG_DWORD
#  define REG_DWORD 4
#endif

#ifndef WINAPI
#  if defined(__i386__) || defined(__i386) || defined(i386)
#    if defined(__clang__) || defined(__GNUC__)
#      define WINAPI __attribute__((stdcall))
#    else
#      define WINAPI
#    endif
#  else
#    define WINAPI
#  endif
#endif

namespace fl_port_registry_detail {

inline fl_port::XdgConfig& xdg() {
  static fl_port::XdgConfig cfg;
  return cfg;
}

inline std::mutex& mtx() {
  static std::mutex m;
  return m;
}

inline std::string hive_name(HKEY h) {
  auto v = reinterpret_cast<std::uintptr_t>(h);
  switch (v) {
    case 0x80000000u: return "HKCR";
    case 0x80000001u: return "HKCU";
    case 0x80000002u: return "HKLM";
    case 0x80000003u: return "HKU";
    default: return {};
  }
}

inline bool is_pseudo_hkey(HKEY h) {
  auto v = reinterpret_cast<std::uintptr_t>(h);
  return v >= 0x80000000u && v <= 0x80000003u;
}

struct Handle {
  std::string path;
};

inline std::map<std::uintptr_t, Handle>& handles() {
  static std::map<std::uintptr_t, Handle> h;
  return h;
}

inline std::uintptr_t next_id_locked() {
  static std::uintptr_t id = 0x1000;
  return ++id;
}

inline std::string normalize_subkey(const char* sub) {
  if (!sub) return {};
  std::string s = fl_port::normalize_path_separators(sub);
  while (!s.empty() && (s.front() == '/' || s.front() == '\\')) s.erase(s.begin());
  while (!s.empty() && (s.back() == '/' || s.back() == '\\')) s.pop_back();
  return s;
}

inline std::string join_path(const std::string& base, const std::string& sub) {
  if (base.empty()) return sub;
  if (sub.empty()) return base;
  return base + "/" + sub;
}

inline std::string key_path(HKEY hKey, const char* lpSubKey) {
  std::string sub = normalize_subkey(lpSubKey);
  if (is_pseudo_hkey(hKey)) {
    return join_path(hive_name(hKey), sub);
  }
  std::lock_guard<std::mutex> lock(mtx());
  auto it = handles().find(reinterpret_cast<std::uintptr_t>(hKey));
  if (it == handles().end()) return {};
  return join_path(it->second.path, sub);
}

inline HKEY make_handle(const std::string& path) {
  std::lock_guard<std::mutex> lock(mtx());
  const auto id = next_id_locked();
  handles()[id] = Handle{path};
  return reinterpret_cast<HKEY>(id);
}

inline bool lookup_handle(HKEY hKey, std::string& path_out) {
  if (is_pseudo_hkey(hKey)) {
    path_out = hive_name(hKey);
    return !path_out.empty();
  }
  std::lock_guard<std::mutex> lock(mtx());
  auto it = handles().find(reinterpret_cast<std::uintptr_t>(hKey));
  if (it == handles().end()) return false;
  path_out = it->second.path;
  return true;
}

inline std::string normalized_value_name(const char* lpValueName) {
  if (!lpValueName || *lpValueName == '\0') return "@"; // (Default)
  return std::string(lpValueName);
}

inline std::string bytes_to_hex(const BYTE* data, DWORD len) {
  static const char* hex = "0123456789abcdef";
  std::string out;
  out.resize(static_cast<size_t>(len) * 2);
  for (DWORD i = 0; i < len; ++i) {
    out[i * 2] = hex[(data[i] >> 4) & 0xF];
    out[i * 2 + 1] = hex[data[i] & 0xF];
  }
  return out;
}

inline bool hex_to_bytes(const std::string& hex, std::string& out_bytes) {
  auto nib = [](char c) -> int {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return -1;
  };
  if (hex.size() % 2 != 0) return false;
  out_bytes.clear();
  out_bytes.reserve(hex.size() / 2);
  for (size_t i = 0; i < hex.size(); i += 2) {
    int hi = nib(hex[i]), lo = nib(hex[i + 1]);
    if (hi < 0 || lo < 0) return false;
    out_bytes.push_back(static_cast<char>((hi << 4) | lo));
  }
  return true;
}

inline std::string encode_value(DWORD type, const BYTE* data, DWORD cbData) {
  if (type == REG_DWORD) {
    std::uint32_t v = 0;
    if (data && cbData >= 4) std::memcpy(&v, data, 4);
    else if (data && cbData > 0) std::memcpy(&v, data, std::min<DWORD>(cbData, 4));
    return "dword:" + std::to_string(static_cast<unsigned long>(v));
  }
  if (type == REG_BINARY) {
    return "hex:" + bytes_to_hex(data ? data : reinterpret_cast<const BYTE*>(""), data ? cbData : 0);
  }
  // REG_SZ / REG_EXPAND_SZ / others: store as string payload (trim trailing NULs for SZ-like data)
  std::string payload;
  if (data && cbData > 0) {
    size_t n = cbData;
    if (type == REG_SZ || type == REG_EXPAND_SZ) {
      while (n > 0 && data[n - 1] == 0) --n;
    }
    payload.assign(reinterpret_cast<const char*>(data), n);
  }
  if (type == REG_EXPAND_SZ) return "expand_sz:" + payload;
  return "sz:" + payload;
}

inline bool decode_value(const std::string& stored, DWORD* out_type, std::string& out_bytes) {
  auto starts_with = [](const std::string& s, const char* pfx) {
    const size_t n = std::strlen(pfx);
    return s.size() >= n && s.compare(0, n, pfx) == 0;
  };
  if (starts_with(stored, "dword:")) {
    const std::string num = stored.substr(6);
    char* end = nullptr;
    unsigned long v = std::strtoul(num.c_str(), &end, 10);
    if (end == num.c_str()) v = 0;
    std::uint32_t v32 = static_cast<std::uint32_t>(v);
    out_bytes.assign(reinterpret_cast<const char*>(&v32), 4);
    if (out_type) *out_type = REG_DWORD;
    return true;
  }
  if (starts_with(stored, "hex:")) {
    std::string bytes;
    if (!hex_to_bytes(stored.substr(4), bytes)) return false;
    out_bytes = std::move(bytes);
    if (out_type) *out_type = REG_BINARY;
    return true;
  }
  if (starts_with(stored, "expand_sz:")) {
    out_bytes = stored.substr(10);
    if (out_type) *out_type = REG_EXPAND_SZ;
    return true;
  }
  if (starts_with(stored, "sz:")) {
    out_bytes = stored.substr(3);
    if (out_type) *out_type = REG_SZ;
    return true;
  }
  // Backward-compat fallback (pre-typed values): treat as REG_SZ
  out_bytes = stored;
  if (out_type) *out_type = REG_SZ;
  return true;
}

} // namespace fl_port_registry_detail

inline LSTATUS WINAPI RegOpenKeyExA(HKEY hKey, const char* lpSubKey, DWORD, REGSAM, HKEY* phkResult) {
  if (!phkResult) return ERROR_INVALID_PARAMETER;
  *phkResult = nullptr;
  using namespace fl_port_registry_detail;
  std::string path = key_path(hKey, lpSubKey);
  if (path.empty() && !is_pseudo_hkey(hKey)) return ERROR_FILE_NOT_FOUND;
  // Bring-up behavior: open logical key handle; values are persisted in XDG-backed registry.ini.
  *phkResult = make_handle(path);
  return ERROR_SUCCESS;
}

inline LSTATUS WINAPI RegCreateKeyExA(HKEY hKey, const char* lpSubKey, DWORD, char*, DWORD, REGSAM,
                                      void*, HKEY* phkResult, DWORD* lpdwDisposition) {
  if (!phkResult) return ERROR_INVALID_PARAMETER;
  using namespace fl_port_registry_detail;
  std::string path = key_path(hKey, lpSubKey);
  if (path.empty()) return ERROR_INVALID_PARAMETER;
  *phkResult = make_handle(path);
  if (lpdwDisposition) *lpdwDisposition = 1; // REG_CREATED_NEW_KEY (logical)
  return ERROR_SUCCESS;
}

inline LSTATUS WINAPI RegCloseKey(HKEY hKey) {
  using namespace fl_port_registry_detail;
  if (!hKey || is_pseudo_hkey(hKey)) return ERROR_SUCCESS;
  std::lock_guard<std::mutex> lock(mtx());
  handles().erase(reinterpret_cast<std::uintptr_t>(hKey));
  return ERROR_SUCCESS;
}

inline LSTATUS WINAPI RegQueryValueExA(HKEY hKey, const char* lpValueName, DWORD*, DWORD* lpType,
                                       BYTE* lpData, DWORD* lpcbData) {
  using namespace fl_port_registry_detail;
  std::string path;
  if (!lookup_handle(hKey, path)) return ERROR_FILE_NOT_FOUND;

  std::string stored;
  if (!xdg().reg_read(path, normalized_value_name(lpValueName), stored)) return ERROR_FILE_NOT_FOUND;

  DWORD type = REG_SZ;
  std::string bytes;
  if (!decode_value(stored, &type, bytes)) return ERROR_FILE_NOT_FOUND;
  if (lpType) *lpType = type;

  DWORD needed = 0;
  if (type == REG_SZ || type == REG_EXPAND_SZ) {
    needed = static_cast<DWORD>(bytes.size() + 1); // include NUL terminator
  } else {
    needed = static_cast<DWORD>(bytes.size());
  }

  if (!lpData) {
    if (lpcbData) *lpcbData = needed;
    return ERROR_SUCCESS;
  }
  if (!lpcbData) return ERROR_INVALID_PARAMETER;
  if (*lpcbData < needed) {
    *lpcbData = needed;
    return ERROR_MORE_DATA;
  }

  if (!bytes.empty()) std::memcpy(lpData, bytes.data(), bytes.size());
  if (type == REG_SZ || type == REG_EXPAND_SZ) {
    lpData[bytes.size()] = 0;
  }
  *lpcbData = needed;
  return ERROR_SUCCESS;
}

inline LSTATUS WINAPI RegSetValueExA(HKEY hKey, const char* lpValueName, DWORD, DWORD dwType,
                                     const BYTE* lpData, DWORD cbData) {
  using namespace fl_port_registry_detail;
  std::string path;
  if (!lookup_handle(hKey, path)) return ERROR_INVALID_PARAMETER;

  const std::string stored = encode_value(dwType, lpData, cbData);
  return xdg().reg_write(path, normalized_value_name(lpValueName), stored) ? ERROR_SUCCESS : ERROR_ACCESS_DENIED;
}

#endif
