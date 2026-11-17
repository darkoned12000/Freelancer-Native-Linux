// FFmpeg-based movie playback backend for M8.8/M12.
// Scaffold: provides the interface; full decode/render integration pending.

#include "linux_port/movie_backend.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

namespace fl_port {

class FfmpegMovieBackend : public MovieBackend {
public:
  bool init(const MovieConfig& cfg) override {
    (void)cfg;
    // TODO: allocate frame buffers, init SDL texture
    return true;
  }
  bool open(const std::string& path) override {
    (void)path;
    // TODO: avformat_open_input, find video stream
    return false; // not yet implemented
  }
  void play() override {}
  void stop() override {}
  void update() override {}
  bool is_playing() const override { return false; }
  void shutdown() override {}
};

} // namespace fl_port
