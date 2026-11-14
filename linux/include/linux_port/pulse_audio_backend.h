#pragma once

#if !defined(FL_LINUX_32BIT)

// M10: PulseAudio-backed audio backend for the 64-bit Linux build.
// Uses the simple PulseAudio API (pa_simple) for playback.

#include "linux_port/audio_backend.h"

#include <pulse/simple.h>
#include <pulse/error.h>

#include <cstdint>
#include <string>

namespace fl_port {

class PulseAudioBackend : public AudioBackend {
 public:
  PulseAudioBackend();
  ~PulseAudioBackend() override;

  bool init(const AudioConfig& config = {}) override;
  void shutdown() override;
  bool is_initialized() const override { return initialized_; }

  int play_sound(const std::string& name, float volume = 1.0f, bool loop = false) override;
  void stop_sound(int handle) override;
  void stop_all() override;
  bool is_playing(int handle) const override;
  void set_master_volume(float v) override;
  float master_volume() const override { return master_volume_; }

  void update() override;

 private:
  AudioConfig config_{};
  bool initialized_{false};
  float master_volume_{1.0f};
  pa_simple* pa_{nullptr};
  int next_handle_{1};
  struct Sound { int handle; bool playing; bool loop; float volume; };
  std::vector<Sound> sounds_;
};

}  // namespace fl_port

#endif  // !FL_LINUX_32BIT
