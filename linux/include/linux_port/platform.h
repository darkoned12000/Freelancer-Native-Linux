#pragma once

// Platform abstraction entry points for Linux bring-up (#1).
// Initial skeleton: APIs will be added as core modules (dacom/ReadFile/etc.)
// are ported or harnessed.

#include "linux_port/win32_compat.h"

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>

namespace fl_port {

enum class LogLevel {
  Trace,
  Debug,
  Info,
  Warning,
  Error,
};

// Minimal logging hook (replace with engine-integrated logging later).
void log_message(LogLevel level, std::string_view message);

// Debug-output shim for code paths expecting OutputDebugString-like behavior.
void debug_output(std::string_view message);

// DACOM/FDUMP-aligned formatted diagnostics (bring-up compatible).
// `code` mirrors DACOM dump/error codes (e.g. 0x100001 error, 0x100002 warning).
void fdump(std::uint32_t code, const char* fmt, ...);

inline std::string format_printf(const char* fmt, va_list args) {
  if (!fmt) return {};
  va_list copy;
  va_copy(copy, args);
  const int needed = std::vsnprintf(nullptr, 0, fmt, copy);
  va_end(copy);
  if (needed < 0) return {};
  if (needed == 0) return {};
  std::string out(static_cast<size_t>(needed) + 1, '\0');
  std::vsnprintf(out.data(), out.size(), fmt, args);
  out.resize(static_cast<size_t>(needed));
  return out;
}

} // namespace fl_port

// Compatibility macros for code paths expecting DACOM-style FDUMP/FDUMP_ERROR usage.
// Formatting and sink are Linux-port specific but preserve code/message intent.
#ifndef FDUMP
#define FDUMP(code, fmt, ...) ::fl_port::fdump(static_cast<std::uint32_t>(code), fmt, ##__VA_ARGS__)
#endif
#ifndef FDUMP_ERROR
#define FDUMP_ERROR(code, fmt, ...) ::fl_port::fdump(static_cast<std::uint32_t>(code), fmt, ##__VA_ARGS__)
#endif
#ifndef FDUMP_WARNING
#define FDUMP_WARNING(code, fmt, ...) ::fl_port::fdump(static_cast<std::uint32_t>(code), fmt, ##__VA_ARGS__)
#endif
