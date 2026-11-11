#include "linux_port/platform.h"

#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

namespace fl_port {
namespace {
std::mutex g_log_mutex;

const char* level_name(LogLevel level) {
  switch (level) {
    case LogLevel::Trace: return "TRACE";
    case LogLevel::Debug: return "DEBUG";
    case LogLevel::Info: return "INFO";
    case LogLevel::Warning: return "WARN";
    case LogLevel::Error: return "ERROR";
    default: return "LOG";
  }
}

std::string timestamp_now() {
  using namespace std::chrono;
  const auto now = system_clock::now();
  const auto t = system_clock::to_time_t(now);
  const auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
  std::tm tm{};
#if defined(_POSIX_THREAD_SAFE_FUNCTIONS) || defined(__unix__) || defined(__APPLE__)
  localtime_r(&t, &tm);
#else
  tm = *std::localtime(&t);
#endif
  std::ostringstream oss;
  oss << std::put_time(&tm, "%H:%M:%S") << '.' << std::setfill('0') << std::setw(3) << ms.count();
  return oss.str();
}
} // namespace

void log_message(LogLevel level, std::string_view message) {
  std::lock_guard<std::mutex> lock(g_log_mutex);
  std::cerr << "[fl-port][" << level_name(level) << "][" << timestamp_now() << "][tid "
            << std::this_thread::get_id() << "] " << message << "\n";
}

void debug_output(std::string_view message) {
  // Placeholder equivalent for OutputDebugStringA-style diagnostics.
  log_message(LogLevel::Debug, message);
}

void fdump(std::uint32_t code, const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  std::string msg = format_printf(fmt, args);
  va_end(args);

  // DACOM convention (observed): 0x100002 used for warnings, 0x100001 for errors in many paths.
  LogLevel level = LogLevel::Info;
  if (code == 0x100002u) level = LogLevel::Warning;
  else if ((code & 0xFFFF0000u) == 0x10000000u || code == 0x100001u) level = LogLevel::Error;

  char prefix[32];
  std::snprintf(prefix, sizeof(prefix), "FDUMP(0x%08X) ", code);
  log_message(level, std::string(prefix) + msg);
}

} // namespace fl_port
