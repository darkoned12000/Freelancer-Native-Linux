#include "linux_port/win32_file.h"

#if defined(_WIN32)
// Native Windows build uses system headers.
#else

#include "linux_port/path_util.h"
#include "linux_port/win32_sync.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <filesystem>
#include <mutex>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

namespace {

thread_local DWORD g_last_error = 0;

enum class HandleKind { File, Find, Mapping, View };

struct FileHandleState {
  int fd = -1;
  std::string path;
  bool writable = false;
};

struct FindHandleState {
  std::string directory;
  std::vector<std::string> matches;
  size_t index = 0;
};

struct MappingHandleState {
  int fd = -1;
  size_t size = 0;
  bool writable = false;
};

struct ViewHandleState {
  void* addr = nullptr;
  size_t size = 0;
};

struct HandleState {
  HandleKind kind;
  FileHandleState file;
  FindHandleState find;
  MappingHandleState mapping;
  ViewHandleState view;
};

std::mutex g_handles_mutex;
std::unordered_map<void*, HandleState*> g_handles;
uintptr_t g_next_handle_id = 0x1000;

HANDLE new_handle(HandleState* state) {
  std::lock_guard<std::mutex> lock(g_handles_mutex);
  void* h = reinterpret_cast<void*>(++g_next_handle_id);
  g_handles[h] = state;
  return h;
}

HandleState* get_handle(HANDLE h, HandleKind kind) {
  if (!h || h == INVALID_HANDLE_VALUE) return nullptr;
  std::lock_guard<std::mutex> lock(g_handles_mutex);
  auto it = g_handles.find(h);
  if (it == g_handles.end() || !it->second || it->second->kind != kind) return nullptr;
  return it->second;
}

HandleState* take_handle(HANDLE h, HandleKind kind) {
  if (!h || h == INVALID_HANDLE_VALUE) return nullptr;
  std::lock_guard<std::mutex> lock(g_handles_mutex);
  auto it = g_handles.find(h);
  if (it == g_handles.end() || !it->second || it->second->kind != kind) return nullptr;
  HandleState* s = it->second;
  g_handles.erase(it);
  return s;
}

std::filesystem::path resolve_input_path(const char* path, bool allow_missing_tail = false) {
  if (!path || !*path) return {};
  fl_port::ResolveOptions opts;
  opts.allow_missing_tail = allow_missing_tail;
  const std::string normalized = fl_port::normalize_path_separators(path);
  std::filesystem::path p(normalized.c_str());
  if (p.is_absolute()) {
    auto r = fl_port::resolve_path_case_insensitive(p.root_path(), p.relative_path().string(), opts);
    return r ? *r : p;
  }
  auto r = fl_port::resolve_path_case_insensitive(std::filesystem::current_path(), p.string(), opts);
  return r ? *r : (std::filesystem::current_path() / p);
}

bool match_wildcard(const std::string& pattern, const std::string& name) {
  // Simple '*'/'?' matcher (case-insensitive ASCII) sufficient for common FindFirstFile usage.
  size_t pi = 0, ni = 0, star = std::string::npos, match = 0;
  auto lower = [](unsigned char c) -> char { return (c >= 'A' && c <= 'Z') ? char(c - 'A' + 'a') : char(c); };
  while (ni < name.size()) {
    if (pi < pattern.size() && (pattern[pi] == '?' || lower((unsigned char)pattern[pi]) == lower((unsigned char)name[ni]))) {
      ++pi; ++ni;
    } else if (pi < pattern.size() && pattern[pi] == '*') {
      star = pi++; match = ni;
    } else if (star != std::string::npos) {
      pi = star + 1; ni = ++match;
    } else {
      return false;
    }
  }
  while (pi < pattern.size() && pattern[pi] == '*') ++pi;
  return pi == pattern.size();
}

FILETIME unix_time_to_filetime_value(std::time_t t) {
  const unsigned long long intervals_per_sec = 10000000ULL;
  const unsigned long long epoch_diff_sec = 11644473600ULL;
  const unsigned long long ft = (static_cast<unsigned long long>(t) + epoch_diff_sec) * intervals_per_sec;
  FILETIME out{};
  out.dwLowDateTime = static_cast<DWORD>(ft & 0xFFFFFFFFu);
  out.dwHighDateTime = static_cast<DWORD>((ft >> 32) & 0xFFFFFFFFu);
  return out;
}

void fill_find_data(const std::filesystem::path& full, WIN32_FIND_DATAA* out) {
  if (!out) return;
  std::memset(out, 0, sizeof(*out));
  std::error_code ec;
  auto st = std::filesystem::status(full, ec);
  out->dwFileAttributes = 0;
  if (!ec && std::filesystem::is_directory(st)) out->dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
  else out->dwFileAttributes |= FILE_ATTRIBUTE_NORMAL;
  if (!ec) {
    auto sz = std::filesystem::is_regular_file(st) ? std::filesystem::file_size(full, ec) : 0;
    if (ec) sz = 0;
    out->nFileSizeLow = static_cast<DWORD>(sz & 0xFFFFFFFFu);
    out->nFileSizeHigh = static_cast<DWORD>((static_cast<unsigned long long>(sz) >> 32) & 0xFFFFFFFFu);
  }
  if (!ec) {
    struct stat sb {};
    if (::stat(full.string().c_str(), &sb) == 0) {
      out->ftCreationTime = unix_time_to_filetime_value(sb.st_ctime);
      out->ftLastAccessTime = unix_time_to_filetime_value(sb.st_atime);
      out->ftLastWriteTime = unix_time_to_filetime_value(sb.st_mtime);
    }
  }
  const std::string name = full.filename().string();
  std::strncpy(out->cFileName, name.c_str(), MAX_PATH - 1);
  out->cFileName[MAX_PATH - 1] = '\0';
}

DWORD errno_to_win32(int e) {
  switch (e) {
    case 0: return 0;
    case ENOENT: return 2;   // ERROR_FILE_NOT_FOUND
    case EACCES: return 5;   // ERROR_ACCESS_DENIED
    case EEXIST: return 183; // ERROR_ALREADY_EXISTS
    case EINVAL: return 87;  // ERROR_INVALID_PARAMETER
    default: return 50;      // ERROR_NOT_SUPPORTED (generic fallback)
  }
}

} // namespace

void SetLastError(DWORD err) { g_last_error = err; }
DWORD GetLastError() { return g_last_error; }

HANDLE CreateFileA(const char* lpFileName, DWORD dwDesiredAccess, DWORD /*dwShareMode*/,
                   void* /*lpSecurityAttributes*/, DWORD dwCreationDisposition,
                   DWORD /*dwFlagsAndAttributes*/, void* /*hTemplateFile*/) {
  if (!lpFileName || !*lpFileName) { SetLastError(87); return INVALID_HANDLE_VALUE; }

  const bool want_write = (dwDesiredAccess & GENERIC_WRITE) != 0;
  const bool want_read = (dwDesiredAccess & GENERIC_READ) != 0 || !want_write;

  std::filesystem::path resolved;
  int flags = 0;
  if (dwCreationDisposition == CREATE_ALWAYS) {
    resolved = resolve_input_path(lpFileName, true);
    flags = O_CREAT | O_TRUNC | (want_write && want_read ? O_RDWR : (want_write ? O_WRONLY : O_RDONLY));
  } else if (dwCreationDisposition == 4u /* OPEN_ALWAYS */) {
    resolved = resolve_input_path(lpFileName, true);
    flags = O_CREAT | (want_write && want_read ? O_RDWR : (want_write ? O_WRONLY : O_RDONLY));
  } else { // OPEN_EXISTING and others treated as open-existing for bring-up
    resolved = resolve_input_path(lpFileName, false);
    flags = want_write && want_read ? O_RDWR : (want_write ? O_WRONLY : O_RDONLY);
  }

  const int fd = ::open(resolved.string().c_str(), flags, 0666);
  if (fd < 0) {
    SetLastError(errno_to_win32(errno));
    return INVALID_HANDLE_VALUE;
  }

  auto* st = new HandleState();
  st->kind = HandleKind::File;
  st->file.fd = fd;
  st->file.path = resolved.string();
  st->file.writable = want_write;
  SetLastError(0);
  return new_handle(st);
}

BOOL ReadFile(HANDLE hFile, void* lpBuffer, DWORD nNumberOfBytesToRead,
              DWORD* lpNumberOfBytesRead, void* /*lpOverlapped*/) {
  if (lpNumberOfBytesRead) *lpNumberOfBytesRead = 0;
  HandleState* st = get_handle(hFile, HandleKind::File);
  if (!st || st->file.fd < 0 || (!lpBuffer && nNumberOfBytesToRead > 0)) {
    SetLastError(6); // ERROR_INVALID_HANDLE
    return FALSE;
  }
  ssize_t n = ::read(st->file.fd, lpBuffer, nNumberOfBytesToRead);
  if (n < 0) {
    SetLastError(errno_to_win32(errno));
    return FALSE;
  }
  if (lpNumberOfBytesRead) *lpNumberOfBytesRead = static_cast<DWORD>(n);
  SetLastError(0);
  return TRUE;
}

BOOL WriteFile(HANDLE hFile, const void* lpBuffer, DWORD nNumberOfBytesToWrite,
               DWORD* lpNumberOfBytesWritten, void* /*lpOverlapped*/) {
  if (lpNumberOfBytesWritten) *lpNumberOfBytesWritten = 0;
  HandleState* st = get_handle(hFile, HandleKind::File);
  if (!st || st->file.fd < 0) { SetLastError(6); return FALSE; }
  ssize_t n = ::write(st->file.fd, lpBuffer, nNumberOfBytesToWrite);
  if (n < 0) { SetLastError(errno_to_win32(errno)); return FALSE; }
  if (lpNumberOfBytesWritten) *lpNumberOfBytesWritten = static_cast<DWORD>(n);
  SetLastError(0);
  return TRUE;
}

DWORD GetFileSize(HANDLE hFile, DWORD* lpFileSizeHigh) {
  HandleState* st = get_handle(hFile, HandleKind::File);
  if (!st || st->file.fd < 0) { SetLastError(6); return INVALID_FILE_SIZE; }
  struct stat sb {};
  if (::fstat(st->file.fd, &sb) != 0) { SetLastError(errno_to_win32(errno)); return INVALID_FILE_SIZE; }
  const unsigned long long sz = sb.st_size < 0 ? 0ULL : static_cast<unsigned long long>(sb.st_size);
  if (lpFileSizeHigh) *lpFileSizeHigh = static_cast<DWORD>((sz >> 32) & 0xFFFFFFFFu);
  SetLastError(0);
  return static_cast<DWORD>(sz & 0xFFFFFFFFu);
}

DWORD SetFilePointer(HANDLE hFile, LONG lDistanceToMove, LONG* lpDistanceToMoveHigh, DWORD dwMoveMethod) {
  HandleState* st = get_handle(hFile, HandleKind::File);
  if (!st || st->file.fd < 0) { SetLastError(6); return 0xFFFFFFFFu; }
  int whence = SEEK_SET;
  if (dwMoveMethod == FILE_CURRENT) whence = SEEK_CUR;
  else if (dwMoveMethod == FILE_END) whence = SEEK_END;
  off_t offset = 0;
  if (lpDistanceToMoveHigh) {
    uint64_t uoff = static_cast<uint32_t>(static_cast<ULONG>(lDistanceToMove)) |
                    (static_cast<uint64_t>(static_cast<uint32_t>(*lpDistanceToMoveHigh)) << 32);
    offset = static_cast<off_t>(uoff);
  } else {
    offset = static_cast<off_t>(lDistanceToMove);
  }
  const off_t pos = ::lseek(st->file.fd, offset, whence);
  if (pos == (off_t)-1) { SetLastError(errno_to_win32(errno)); return 0xFFFFFFFFu; }
  if (lpDistanceToMoveHigh) *lpDistanceToMoveHigh = static_cast<LONG>((static_cast<unsigned long long>(pos) >> 32) & 0x7FFFFFFFu);
  SetLastError(0);
  return static_cast<DWORD>(static_cast<unsigned long long>(pos) & 0xFFFFFFFFu);
}

BOOL SetEndOfFile(HANDLE hFile) {
  HandleState* st = get_handle(hFile, HandleKind::File);
  if (!st || st->file.fd < 0) { SetLastError(6); return FALSE; }
  const off_t pos = ::lseek(st->file.fd, 0, SEEK_CUR);
  if (pos == (off_t)-1) { SetLastError(errno_to_win32(errno)); return FALSE; }
  if (::ftruncate(st->file.fd, pos) != 0) { SetLastError(errno_to_win32(errno)); return FALSE; }
  SetLastError(0);
  return TRUE;
}

DWORD GetFileAttributesA(const char* lpFileName) {
  if (!lpFileName) { SetLastError(87); return INVALID_FILE_ATTRIBUTES; }
  const auto p = resolve_input_path(lpFileName, false);
  std::error_code ec;
  if (!std::filesystem::exists(p, ec)) { SetLastError(2); return INVALID_FILE_ATTRIBUTES; }
  DWORD attrs = 0;
  if (std::filesystem::is_directory(p, ec)) attrs |= FILE_ATTRIBUTE_DIRECTORY;
  if (!attrs) attrs = FILE_ATTRIBUTE_NORMAL;
  SetLastError(0);
  return attrs;
}

BOOL GetFileTime(HANDLE hFile, FILETIME* lpCreationTime, FILETIME* lpLastAccessTime, FILETIME* lpLastWriteTime) {
  HandleState* st = get_handle(hFile, HandleKind::File);
  if (!st || st->file.fd < 0) { SetLastError(6); return FALSE; }
  struct stat sb {};
  if (::fstat(st->file.fd, &sb) != 0) { SetLastError(errno_to_win32(errno)); return FALSE; }
  if (lpCreationTime) *lpCreationTime = unix_time_to_filetime_value(sb.st_ctime);
  if (lpLastAccessTime) *lpLastAccessTime = unix_time_to_filetime_value(sb.st_atime);
  if (lpLastWriteTime) *lpLastWriteTime = unix_time_to_filetime_value(sb.st_mtime);
  SetLastError(0);
  return TRUE;
}

HANDLE FindFirstFileA(const char* lpFileName, WIN32_FIND_DATAA* lpFindFileData) {
  if (!lpFileName || !lpFindFileData) { SetLastError(87); return INVALID_HANDLE_VALUE; }
  std::string in = fl_port::normalize_path_separators(lpFileName ? lpFileName : "");
  // Split directory/pattern
  size_t slash = in.find_last_of('/');
  std::string dir_part = (slash == std::string::npos) ? std::string(".") : in.substr(0, slash);
  std::string pat_part = (slash == std::string::npos) ? in : in.substr(slash + 1);
  if (pat_part.empty()) pat_part = "*";

  auto dir_path = resolve_input_path(dir_part.c_str(), false);
  std::error_code ec;
  if (!std::filesystem::exists(dir_path, ec) || !std::filesystem::is_directory(dir_path, ec)) {
    SetLastError(2);
    return INVALID_HANDLE_VALUE;
  }

  auto* st = new HandleState();
  st->kind = HandleKind::Find;
  st->find.directory = dir_path.string();

  DIR* d = ::opendir(dir_path.string().c_str());
  if (d) {
    while (dirent* e = ::readdir(d)) {
      if (std::strcmp(e->d_name, ".") == 0 || std::strcmp(e->d_name, "..") == 0) continue;
      if (match_wildcard(pat_part, e->d_name)) st->find.matches.emplace_back(e->d_name);
    }
    ::closedir(d);
  }
  std::sort(st->find.matches.begin(), st->find.matches.end());

  if (st->find.matches.empty()) {
    delete st;
    SetLastError(2);
    return INVALID_HANDLE_VALUE;
  }
  fill_find_data(std::filesystem::path(st->find.directory) / st->find.matches[0], lpFindFileData);
  st->find.index = 1;
  SetLastError(0);
  return new_handle(st);
}

BOOL FindNextFileA(HANDLE hFindFile, WIN32_FIND_DATAA* lpFindFileData) {
  HandleState* st = get_handle(hFindFile, HandleKind::Find);
  if (!st || !lpFindFileData) { SetLastError(6); return FALSE; }
  if (st->find.index >= st->find.matches.size()) { SetLastError(18); return FALSE; } // ERROR_NO_MORE_FILES
  fill_find_data(std::filesystem::path(st->find.directory) / st->find.matches[st->find.index++], lpFindFileData);
  SetLastError(0);
  return TRUE;
}

BOOL FindClose(HANDLE hFindFile) {
  HandleState* st = take_handle(hFindFile, HandleKind::Find);
  if (!st) { SetLastError(6); return FALSE; }
  delete st;
  SetLastError(0);
  return TRUE;
}

HANDLE CreateFileMappingA(HANDLE hFile, void* /*lpFileMappingAttributes*/, DWORD flProtect,
                          DWORD dwMaximumSizeHigh, DWORD dwMaximumSizeLow, const char* /*lpName*/) {
  int dupfd = -1;
  size_t size = 0;
  if (hFile == INVALID_HANDLE_VALUE || hFile == reinterpret_cast<HANDLE>(static_cast<intptr_t>(-1))) {
    // Pagefile-backed (anonymous) mapping.
    const unsigned long long sz = (static_cast<unsigned long long>(dwMaximumSizeHigh) << 32) |
                                  static_cast<unsigned long long>(dwMaximumSizeLow);
    if (sz == 0) { SetLastError(87); return nullptr; }
    size = static_cast<size_t>(sz);
  } else {
    HandleState* fs = get_handle(hFile, HandleKind::File);
    if (!fs || fs->file.fd < 0) { SetLastError(6); return nullptr; }
    if (dwMaximumSizeLow == 0 && dwMaximumSizeHigh == 0) {
      struct stat sb {};
      if (::fstat(fs->file.fd, &sb) == 0 && sb.st_size > 0) size = static_cast<size_t>(sb.st_size);
    } else {
      const unsigned long long sz = (static_cast<unsigned long long>(dwMaximumSizeHigh) << 32) |
                                    static_cast<unsigned long long>(dwMaximumSizeLow);
      size = static_cast<size_t>(sz);
    }
    dupfd = ::dup(fs->file.fd);
    if (dupfd < 0) { SetLastError(errno_to_win32(errno)); return nullptr; }
  }
  auto* ms = new HandleState();
  ms->kind = HandleKind::Mapping;
  ms->mapping.fd = dupfd;
  ms->mapping.size = size;
  ms->mapping.writable = (flProtect == 0x04 /* PAGE_READWRITE */) || (flProtect == 0x08 /* PAGE_WRITECOPY */);
  SetLastError(0);
  return new_handle(ms);
}

void* MapViewOfFile(HANDLE hFileMappingObject, DWORD dwDesiredAccess, DWORD dwFileOffsetHigh,
                    DWORD dwFileOffsetLow, size_t dwNumberOfBytesToMap) {
  HandleState* ms = get_handle(hFileMappingObject, HandleKind::Mapping);
  if (!ms) { SetLastError(6); return nullptr; }
  const unsigned long long off64 = (static_cast<unsigned long long>(dwFileOffsetHigh) << 32) | dwFileOffsetLow;
  const off_t offset = static_cast<off_t>(off64);
  size_t len = dwNumberOfBytesToMap ? dwNumberOfBytesToMap : (ms->mapping.size > static_cast<size_t>(off64) ? ms->mapping.size - static_cast<size_t>(off64) : 0);
  if (len == 0) { SetLastError(87); return nullptr; }
  const bool want_write = (dwDesiredAccess & 0x0002u /* FILE_MAP_WRITE */) != 0 ||
                          (dwDesiredAccess & 0x000Fu /* FILE_MAP_ALL_ACCESS mask-ish */) == 0x000Fu;
  int prot = PROT_READ;
  if (ms->mapping.writable && want_write) prot |= PROT_WRITE;
  int flags = (ms->mapping.fd < 0) ? (MAP_SHARED | MAP_ANONYMOUS) : MAP_SHARED;
  void* addr = ::mmap(nullptr, len, prot, flags, ms->mapping.fd, offset);
  if (addr == MAP_FAILED) { SetLastError(errno_to_win32(errno)); return nullptr; }
  auto* vs = new HandleState();
  vs->kind = HandleKind::View;
  vs->view.addr = addr;
  vs->view.size = len;
  {
    std::lock_guard<std::mutex> lock(g_handles_mutex);
    g_handles[addr] = vs; // keyed by mapped address for UnmapViewOfFile
  }
  SetLastError(0);
  return addr;
}

BOOL UnmapViewOfFile(const void* lpBaseAddress) {
  if (!lpBaseAddress) { SetLastError(87); return FALSE; }
  HandleState* vs = nullptr;
  {
    std::lock_guard<std::mutex> lock(g_handles_mutex);
    auto it = g_handles.find(const_cast<void*>(lpBaseAddress));
    if (it != g_handles.end() && it->second && it->second->kind == HandleKind::View) {
      vs = it->second;
      g_handles.erase(it);
    }
  }
  if (!vs) { SetLastError(6); return FALSE; }
  const int rc = ::munmap(vs->view.addr, vs->view.size);
  delete vs;
  if (rc != 0) { SetLastError(errno_to_win32(errno)); return FALSE; }
  SetLastError(0);
  return TRUE;
}

BOOL CloseHandle(HANDLE hObject) {
  if (!hObject || hObject == INVALID_HANDLE_VALUE) { SetLastError(6); return FALSE; }
  // Try known handle kinds.
  for (HandleKind k : {HandleKind::File, HandleKind::Mapping, HandleKind::Find}) {
    HandleState* st = take_handle(hObject, k);
    if (!st) continue;
    if (k == HandleKind::File && st->file.fd >= 0) ::close(st->file.fd);
    if (k == HandleKind::Mapping && st->mapping.fd >= 0) ::close(st->mapping.fd);
    delete st;
    SetLastError(0);
    return TRUE;
  }
  // Event handles are managed by win32_sync; accept them via CloseHandle for Win32 compatibility.
  if (fl_port_close_handle_if_event(hObject)) {
    SetLastError(0);
    return TRUE;
  }
  SetLastError(6);
  return FALSE;
}

#endif // !_WIN32

#if defined(_WIN32)
#else

#include <chrono>
#include <ctime>

DWORD GetCurrentDirectoryA(DWORD nBufferLength, char* lpBuffer) {
  if (!lpBuffer || nBufferLength == 0) { SetLastError(87); return 0; }
  std::error_code ec;
  auto cwd = std::filesystem::current_path(ec);
  if (ec) { SetLastError(errno_to_win32(errno)); return 0; }
  std::string s = cwd.string();
  // Preserve Windows-style separators less important than correctness; return normalized POSIX-ish.
  if (s.size() + 1 > nBufferLength) {
    SetLastError(122); // ERROR_INSUFFICIENT_BUFFER
    return static_cast<DWORD>(s.size() + 1);
  }
  std::memcpy(lpBuffer, s.c_str(), s.size() + 1);
  SetLastError(0);
  return static_cast<DWORD>(s.size());
}

BOOL SetCurrentDirectoryA(const char* lpPathName) {
  if (!lpPathName) { SetLastError(87); return FALSE; }
  fl_port::ResolveOptions opts;
  opts.allow_missing_tail = false;
  const std::string normalized = fl_port::normalize_path_separators(lpPathName);
  std::filesystem::path p(normalized);
  std::filesystem::path target = p;
  if (!p.is_absolute()) {
    auto r = fl_port::resolve_path_case_insensitive(std::filesystem::current_path(), p.string(), opts);
    if (r) target = *r;
    else target = std::filesystem::current_path() / p;
  } else {
    auto r = fl_port::resolve_path_case_insensitive(p.root_path(), p.relative_path().string(), opts);
    if (r) target = *r;
  }
  std::error_code ec;
  std::filesystem::current_path(target, ec);
  if (ec) { SetLastError(errno_to_win32(ec.value() ? ec.value() : errno)); return FALSE; }
  SetLastError(0);
  return TRUE;
}

BOOL DosDateTimeToFileTime(WORD wFatDate, WORD wFatTime, FILETIME* lpFileTime) {
  if (!lpFileTime) { SetLastError(87); return FALSE; }
  std::tm tm{};
  tm.tm_sec = (wFatTime & 0x1F) * 2;
  tm.tm_min = (wFatTime >> 5) & 0x3F;
  tm.tm_hour = (wFatTime >> 11) & 0x1F;
  tm.tm_mday = wFatDate & 0x1F;
  tm.tm_mon = ((wFatDate >> 5) & 0x0F) - 1;
  tm.tm_year = ((wFatDate >> 9) & 0x7F) + 80; // since 1980
  tm.tm_isdst = -1;
  const std::time_t t = std::mktime(&tm);
  if (t == static_cast<std::time_t>(-1)) {
    lpFileTime->dwLowDateTime = 0; lpFileTime->dwHighDateTime = 0;
    SetLastError(87);
    return FALSE;
  }
  // Convert Unix epoch (1970) to Windows FILETIME epoch (1601) in 100ns units.
  const unsigned long long intervals_per_sec = 10000000ULL;
  const unsigned long long epoch_diff_sec = 11644473600ULL;
  const unsigned long long ft = (static_cast<unsigned long long>(t) + epoch_diff_sec) * intervals_per_sec;
  lpFileTime->dwLowDateTime = static_cast<DWORD>(ft & 0xFFFFFFFFu);
  lpFileTime->dwHighDateTime = static_cast<DWORD>((ft >> 32) & 0xFFFFFFFFu);
  SetLastError(0);
  return TRUE;
}

DWORD GetFullPathNameA(const char* lpFileName, DWORD nBufferLength, char* lpBuffer, char** lpFilePart) {
  if (!lpFileName) { SetLastError(87); return 0; }

  fl_port::ResolveOptions opts;
  opts.allow_missing_tail = true;
  const std::string normalized = fl_port::normalize_path_separators(lpFileName);
  std::filesystem::path p(normalized);
  std::filesystem::path full;
  if (p.is_absolute()) {
    auto r = fl_port::resolve_path_case_insensitive(p.root_path(), p.relative_path().string(), opts);
    full = r ? r->lexically_normal() : p.lexically_normal();
  } else {
    auto r = fl_port::resolve_path_case_insensitive(std::filesystem::current_path(), p.string(), opts);
    full = r ? r->lexically_normal() : (std::filesystem::current_path() / p).lexically_normal();
  }

  std::string out = full.string();
  // Some engine code may prefer backslashes; but returning POSIX path is safer on Linux.
  // Keep as-is for compatibility with our path resolution users.
  const DWORD needed = static_cast<DWORD>(out.size() + 1);
  if (!lpBuffer || nBufferLength < needed) {
    SetLastError(0);
    return needed; // required buffer size including null terminator
  }
  std::memcpy(lpBuffer, out.c_str(), needed);

  if (lpFilePart) {
    char* last = nullptr;
    for (char* q = lpBuffer; *q; ++q) {
      if (*q == '/' || *q == '\\') last = q + 1;
    }
    *lpFilePart = (last && *last != '\0') ? last : nullptr;
  }
  SetLastError(0);
  return static_cast<DWORD>(out.size());
}

char* _fullpath(char* absPath, const char* relPath, size_t maxLength) {
  if (!relPath) { SetLastError(87); return nullptr; }
  const size_t sz = maxLength ? maxLength : MAX_PATH;
  char* target = absPath ? absPath : static_cast<char*>(std::malloc(sz));
  if (!target) { SetLastError(8); return nullptr; }
  const DWORD need = GetFullPathNameA(relPath, static_cast<DWORD>(sz), target, nullptr);
  if (need == 0 || need >= sz) {
    if (!absPath) std::free(target);
    return nullptr;
  }
  return target;
}

#endif
