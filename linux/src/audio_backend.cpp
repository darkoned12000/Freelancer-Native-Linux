// M6: null audio backend (silent) for Linux bring-up.

#include "linux_port/audio_backend.h"
#include "linux_port/platform.h"

#include <iostream>

namespace fl_port {

NullAudioBackend::NullAudioBackend() = default;
NullAudioBackend::~NullAudioBackend() { shutdown(); }

bool NullAudioBackend::init(const AudioConfig& config) {
  config_ = config;
  master_volume_ = config.master_volume;
  initialized_ = true;
  std::cout << "[fl-port][audio] null audio backend initialized (rate=" << config.sample_rate
            << " ch=" << config.channels << " buf=" << config.buffer_frames << ")\n";
  return true;
}

void NullAudioBackend::shutdown() {
  if (!initialized_) return;
  initialized_ = false;
  std::cout << "[fl-port][audio] null audio backend shut down\n";
}

int NullAudioBackend::play_sound(const std::string& name, float volume, bool loop) {
  (void)name; (void)volume; (void)loop;
  if (!initialized_) return 0;
  ++play_count_;
  return next_handle_++;
}

void NullAudioBackend::stop_sound(int handle) { (void)handle; }
void NullAudioBackend::stop_all() {}
void NullAudioBackend::set_master_volume(float v) {
  if (v < 0.0f) v = 0.0f;
  if (v > 1.0f) v = 1.0f;
  master_volume_ = v;
}
void NullAudioBackend::update() {}

}  // namespace fl_port