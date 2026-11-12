#pragma once

// M6: null (and future real) audio backend for Linux bring-up.
// NullAudioBackend provides a no-op implementation that unblocks boot/menu
// audio calls without requiring a real audio subsystem (PulseAudio/PipeWire/ALSA).

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace fl_port {

struct AudioConfig {
  bool null_backend{true};   // true = silence (no real audio device)
  int sample_rate{44100};
  int channels{2};
  int buffer_frames{1024};
  float master_volume{1.0f};
};

class AudioBackend {
 public:
  AudioBackend() = default;
  virtual ~AudioBackend() = default;

  virtual bool init(const AudioConfig& config = {}) = 0;
  virtual void shutdown() = 0;
  virtual bool is_initialized() const = 0;

  // Playback control (no-op in null backend).
  virtual int play_sound(const std::string& name, float volume = 1.0f, bool loop = false) = 0;
  virtual void stop_sound(int handle) = 0;
  virtual void stop_all() = 0;
  virtual bool is_playing(int handle) const = 0;
  virtual void set_master_volume(float v) = 0;
  virtual float master_volume() const = 0;

  // Per-frame update (mixing, streaming) - no-op in null backend.
  virtual void update() = 0;
};

// Null (silent) audio backend.
class NullAudioBackend : public AudioBackend {
 public:
  NullAudioBackend();
  ~NullAudioBackend() override;

  bool init(const AudioConfig& config = {}) override;
  void shutdown() override;
  bool is_initialized() const override { return initialized_; }

  int play_sound(const std::string& name, float volume = 1.0f, bool loop = false) override;
  void stop_sound(int handle) override;
  void stop_all() override;
  bool is_playing(int handle) const override { return false; }
  void set_master_volume(float v) override;
  float master_volume() const override { return master_volume_; }

  void update() override;

  // Diagnostics.
  int play_count() const { return play_count_; }

 private:
  AudioConfig config_{};
  bool initialized_{false};
  float master_volume_{1.0f};
  int next_handle_{1};
  int play_count_{0};
};

}  // namespace fl_port