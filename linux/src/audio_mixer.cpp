#include "linux_port/audio_mixer.h"

#include <algorithm>
#include <cmath>

namespace fl_port {
namespace {
float clamp_gain(float gain) {
  if (!std::isfinite(gain)) return 0.0f;
  return std::clamp(gain, 0.0f, 4.0f);
}
float clamp_pan(float pan) {
  if (!std::isfinite(pan)) return 0.0f;
  return std::clamp(pan, -1.0f, 1.0f);
}
}

AudioMixer::AudioMixer(unsigned output_channels, float master_gain, std::uint32_t output_sample_rate)
    : output_channels_(std::max(1u, output_channels)),
      output_sample_rate_(std::max(1u, output_sample_rate)),
      master_gain_(clamp_gain(master_gain)) {}

int AudioMixer::play(const float* samples, std::size_t frames, unsigned source_channels,
                     bool loop, float gain, float pan, std::uint32_t source_sample_rate) {
  if (!samples || frames == 0 || (source_channels != 1 && source_channels != 2)) return -1;
  Voice voice;
  voice.frames = frames;
  voice.sample_rate = source_sample_rate ? source_sample_rate : output_sample_rate_;
  voice.channels = source_channels;
  voice.loop = loop;
  voice.playing = true;
  voice.gain = clamp_gain(gain);
  voice.pan = clamp_pan(pan);
  voice.samples.assign(samples, samples + frames * source_channels);

  std::lock_guard<std::mutex> lock(mutex_);
  voice.handle = next_handle_++;
  if (next_handle_ <= 0) next_handle_ = 1;
  voices_.push_back(std::move(voice));
  return voices_.back().handle;
}

void AudioMixer::stop(int handle) {
  std::lock_guard<std::mutex> lock(mutex_);
  voices_.erase(std::remove_if(voices_.begin(), voices_.end(),
                               [handle](const Voice& voice) { return voice.handle == handle; }),
                voices_.end());
}

bool AudioMixer::set_voice(int handle, float gain, float pan, std::uint32_t source_sample_rate) {
  std::lock_guard<std::mutex> lock(mutex_);
  for (auto& voice : voices_) {
    if (voice.handle == handle && voice.playing) {
      voice.gain = clamp_gain(gain);
      voice.pan = clamp_pan(pan);
      if (source_sample_rate) voice.sample_rate = source_sample_rate;
      return true;
    }
  }
  return false;
}

void AudioMixer::stop_all() {
  std::lock_guard<std::mutex> lock(mutex_);
  voices_.clear();
}

bool AudioMixer::is_playing(int handle) const {
  std::lock_guard<std::mutex> lock(mutex_);
  for (const auto& voice : voices_) if (voice.handle == handle) return voice.playing;
  return false;
}

void AudioMixer::set_master_gain(float gain) {
  std::lock_guard<std::mutex> lock(mutex_);
  master_gain_ = clamp_gain(gain);
}

float AudioMixer::master_gain() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return master_gain_;
}

void AudioMixer::mix(float* output, std::size_t frames) {
  if (!output) return;
  std::fill(output, output + frames * output_channels_, 0.0f);
  std::lock_guard<std::mutex> lock(mutex_);
  for (auto& voice : voices_) {
    if (!voice.playing) continue;
    for (std::size_t frame = 0; frame < frames; ++frame) {
      if (voice.cursor >= static_cast<double>(voice.frames)) {
        if (voice.loop) voice.cursor = std::fmod(voice.cursor, static_cast<double>(voice.frames));
        else { voice.playing = false; break; }
      }
      const std::size_t source_frame = static_cast<std::size_t>(voice.cursor);
      const std::size_t next_frame = source_frame + 1 < voice.frames ? source_frame + 1
          : (voice.loop ? 0 : source_frame);
      const float fraction = static_cast<float>(voice.cursor - static_cast<double>(source_frame));
      const std::size_t source = source_frame * voice.channels;
      const std::size_t next = next_frame * voice.channels;
      const float left0 = voice.samples[source];
      const float right0 = voice.channels == 2 ? voice.samples[source + 1] : left0;
      const float left1 = voice.samples[next];
      const float right1 = voice.channels == 2 ? voice.samples[next + 1] : left1;
      float left = left0 + (left1 - left0) * fraction;
      float right = right0 + (right1 - right0) * fraction;
      const float gain = master_gain_ * voice.gain;
      left *= gain;
      right *= gain;
      if (output_channels_ == 1) {
        output[frame] += ((left + right) * 0.5f);
      } else {
        const float left_scale = voice.pan > 0.0f ? 1.0f - voice.pan : 1.0f;
        const float right_scale = voice.pan < 0.0f ? 1.0f + voice.pan : 1.0f;
        output[frame * output_channels_] += left * left_scale;
        output[frame * output_channels_ + 1] += right * right_scale;
      }
      voice.cursor += static_cast<double>(voice.sample_rate) / output_sample_rate_;
    }
    if (!voice.loop && voice.cursor >= static_cast<double>(voice.frames)) voice.playing = false;
  }
  const auto sample_count = frames * output_channels_;
  for (std::size_t i = 0; i < sample_count; ++i) output[i] = std::clamp(output[i], -1.0f, 1.0f);
  voices_.erase(std::remove_if(voices_.begin(), voices_.end(),
                               [](const Voice& voice) { return !voice.playing; }),
                voices_.end());
}

}  // namespace fl_port
