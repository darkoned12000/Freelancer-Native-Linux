#if defined(FL_LINUX_32BIT)

#include "linux_port/alsa_audio_backend.h"

#include <alsa/asoundlib.h>
#include <cstring>
#include <iostream>
#include <vector>

namespace fl_port {

AlsaAudioBackend::AlsaAudioBackend() = default;
AlsaAudioBackend::~AlsaAudioBackend() { shutdown(); }

bool AlsaAudioBackend::init(const AudioConfig& config) {
  config_ = config;
  snd_pcm_t* pcm = nullptr;
  int rc = snd_pcm_open(&pcm, "default", SND_PCM_STREAM_PLAYBACK, 0);
  if (rc < 0) {
    std::cerr << "[alsa] snd_pcm_open failed: " << snd_strerror(rc) << "\n";
    return false;
  }
  snd_pcm_hw_params_t* params = nullptr;
  snd_pcm_hw_params_alloca(&params);
  snd_pcm_hw_params_any(pcm, params);
  snd_pcm_hw_params_set_access(pcm, params, SND_PCM_ACCESS_RW_INTERLEAVED);
  snd_pcm_hw_params_set_format(pcm, params, SND_PCM_FORMAT_FLOAT_LE);
  unsigned int rate = (unsigned int)config.sample_rate;
  snd_pcm_hw_params_set_rate_near(pcm, params, &rate, nullptr);
  snd_pcm_uframes_t frames = (snd_pcm_uframes_t)config.buffer_frames;
  snd_pcm_hw_params_set_period_size_near(pcm, params, &frames, nullptr);
  rc = snd_pcm_hw_params(pcm, params);
  if (rc < 0) {
    std::cerr << "[alsa] snd_pcm_hw_params failed: " << snd_strerror(rc) << "\n";
    snd_pcm_close(pcm);
    return false;
  }
  pcm_ = pcm;
  initialized_ = true;
  master_volume_ = config.master_volume;
  return true;
}

void AlsaAudioBackend::shutdown() {
  if (pcm_) {
    snd_pcm_close((snd_pcm_t*)pcm_);
    pcm_ = nullptr;
  }
  initialized_ = false;
}

int AlsaAudioBackend::play_sound(const std::string& name, float volume, bool loop) {
  (void)name; (void)volume; (void)loop;
  if (!initialized_) return -1;
  int h = next_handle_++;
  return h;
}

void AlsaAudioBackend::stop_sound(int handle) { (void)handle; }
void AlsaAudioBackend::stop_all() {}
void AlsaAudioBackend::set_master_volume(float v) {
  master_volume_ = v;
  if (v < 0.0f) master_volume_ = 0.0f;
  if (v > 1.0f) master_volume_ = 1.0f;
}

void AlsaAudioBackend::update() {
  if (!pcm_ || !initialized_) return;
  size_t n = (size_t)config_.buffer_frames * (size_t)config_.channels;
  std::vector<float> buf(n, 0.0f);
  snd_pcm_sframes_t rc = snd_pcm_writei((snd_pcm_t*)pcm_, buf.data(), (snd_pcm_uframes_t)config_.buffer_frames);
  if (rc < 0) snd_pcm_recover((snd_pcm_t*)pcm_, (int)rc, 0);
}

}  // namespace fl_port

#endif  // FL_LINUX_32BIT