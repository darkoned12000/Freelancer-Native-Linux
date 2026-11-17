#pragma once
// Linux movie playback backend (FFmpeg-based) for M8.8/M12.
// Provides a minimal interface for WMV/ASF/Bink playback stubs.

#include <string>

namespace fl_port {

struct MovieConfig {
  int width = 0;
  int height = 0;
  bool loop = false;
};

class MovieBackend {
public:
  virtual ~MovieBackend() = default;
  virtual bool init(const MovieConfig& cfg) { (void)cfg; return false; }
  virtual bool open(const std::string& path) { (void)path; return false; }
  virtual void play() {}
  virtual void stop() {}
  virtual void update() {}
  virtual bool is_playing() const { return false; }
  virtual void shutdown() {}
};

// Null movie backend (no-op) for bring-up.
class NullMovieBackend : public MovieBackend {
public:
  bool init(const MovieConfig&) override { return true; }
  bool open(const std::string&) override { return true; }
  void play() override {}
  void stop() override {}
  void update() override {}
  bool is_playing() const override { return false; }
  void shutdown() override {}
};

} // namespace fl_port
