#if !defined(FL_LINUX_32BIT)

#include "linux_port/pulse_audio_backend.h"
#include "linux_port/audio_wav.h"

#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>

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
  mixer_.set_master_gain(master_volume_);
  return true;
}

void PulseAudioBackend::shutdown() {
  if (pa_) {
    pa_simple_free(pa_);
    pa_ = nullptr;
  }
  mixer_.stop_all();
  initialized_ = false;
}

int PulseAudioBackend::play_sound(const std::string& name, float volume, bool loop) {
  std::ifstream file(name, std::ios::binary);
  if (!file) return -1;
  const std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(file)), {});
  AudioPcmData pcm;
  if (!decode_wav(bytes.data(), bytes.size(), pcm)) return -1;
  return play_pcm(pcm.samples.data(), pcm.samples.size() / pcm.channels, pcm.channels,
                  volume, 0.0f, loop, pcm.sample_rate);
}

int PulseAudioBackend::play_pcm(const float* samples, std::size_t frames, unsigned channels,
                                float volume, float pan, bool loop, std::uint32_t sample_rate) {
  if (!initialized_ || (channels != 1 && channels != 2)) return -1;
  return mixer_.play(samples, frames, channels, loop, volume, pan, sample_rate);
}

void PulseAudioBackend::stop_sound(int handle) {
  mixer_.stop(handle);
}

void PulseAudioBackend::stop_all() {
  mixer_.stop_all();
}

bool PulseAudioBackend::is_playing(int handle) const {
  return mixer_.is_playing(handle);
}

void PulseAudioBackend::set_master_volume(float v) {
  master_volume_ = v;
  if (v < 0.0f) master_volume_ = 0.0f;
  if (v > 1.0f) master_volume_ = 1.0f;
  mixer_.set_master_gain(master_volume_);
}

void PulseAudioBackend::update() {
  if (!pa_ || !initialized_) return;
  std::vector<float> output(static_cast<size_t>(config_.buffer_frames) * config_.channels);
  mixer_.mix(output.data(), static_cast<size_t>(config_.buffer_frames));
  size_t bytes = output.size() * sizeof(float);
  int error = 0;
  if (pa_simple_write(pa_, output.data(), bytes, &error) < 0)
    std::cerr << "[pulse] pa_simple_write failed: " << pa_strerror(error) << "\n";
}

}  // namespace fl_port

#endif  // !FL_LINUX_32BIT
