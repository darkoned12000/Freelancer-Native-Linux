#pragma once

#if !defined(FL_LINUX_32BIT)

// M10: PulseAudio-backed audio backend for the 64-bit Linux build.
// Uses the simple PulseAudio API (pa_simple) for playback.

#include "linux_port/audio_backend.h"
#include "linux_port/audio_mixer.h"

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
  int play_pcm(const float* samples, std::size_t frames, unsigned channels,
               float volume = 1.0f, float pan = 0.0f, bool loop = false,
               std::uint32_t sample_rate = 0) override;
  bool update_pcm(int handle, float volume, float pan, std::uint32_t rate = 0) override { return mixer_.set_voice(handle, volume, pan, rate); }
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
  AudioMixer mixer_{2};
};

}  // namespace fl_port

#endif  // !FL_LINUX_32BIT
