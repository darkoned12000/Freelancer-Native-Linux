#if !defined(FL_LINUX_32BIT)

#include "linux_port/pulse_audio_backend.h"

#include <cstring>
#include <iostream>

namespace fl_port {

PulseAudioBackend::PulseAudioBackend() = default;
PulseAudioBackend::~PulseAudioBackend() { shutdown(); }

bool PulseAudioBackend::init(const AudioConfig& config) {
  config_ = config;
  pa_sample_spec ss{};
  ss.format = PA_SAMPLE_FLOAT32LE;
  ss.rate = config.sample_rate;
  ss.channels = (uint8_t)config.channels;

  int error = 0;
  pa_ = pa_simple_new(nullptr, "Freelancer", PA_STREAM_PLAYBACK, nullptr,
                       "game", &ss, nullptr, nullptr, &error);
  if (!pa_) {
    std::cerr << "[pulse] pa_simple_new failed: " << pa_strerror(error) << "\n";
    return false;
  }
  initialized_ = true;
  master_volume_ = config.master_volume;
  return true;
}

void PulseAudioBackend::shutdown() {
  if (pa_) {
    pa_simple_free(pa_);
    pa_ = nullptr;
  }
  sounds_.clear();
  initialized_ = false;
}

int PulseAudioBackend::play_sound(const std::string& name, float volume, bool loop) {
  if (!initialized_) return -1;
  int h = next_handle_++;
  sounds_.push_back({h, true, loop, volume});
  return h;
}

void PulseAudioBackend::stop_sound(int handle) {
  for (auto& s : sounds_) {
    if (s.handle == handle) { s.playing = false; break; }
  }
}

void PulseAudioBackend::stop_all() {
  for (auto& s : sounds_) s.playing = false;
}

bool PulseAudioBackend::is_playing(int handle) const {
  for (const auto& s : sounds_) {
    if (s.handle == handle) return s.playing;
  }
  return false;
}

void PulseAudioBackend::set_master_volume(float v) {
  master_volume_ = v;
  if (v < 0.0f) master_volume_ = 0.0f;
  if (v > 1.0f) master_volume_ = 1.0f;
}

void PulseAudioBackend::update() {
  // Mixing/streaming would go here. For now, write silence to keep the stream alive.
  if (!pa_ || !initialized_) return;
  // Write a small buffer of silence.
  size_t bytes = config_.buffer_frames * config_.channels * sizeof(float);
  std::vector<float> silence(config_.buffer_frames * config_.channels, 0.0f);
  int error = 0;
  pa_simple_write(pa_, silence.data(), bytes, &error);
}

}  // namespace fl_port

#endif  // !FL_LINUX_32BIT
