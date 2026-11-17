#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <vector>

namespace fl_port {

// Small software PCM mixer shared by PulseAudio and ALSA sinks.
class AudioMixer {
 public:
  explicit AudioMixer(unsigned output_channels, float master_gain = 1.0f,
                      std::uint32_t output_sample_rate = 44100);

  int play(const float* interleaved_samples, std::size_t frames, unsigned source_channels,
           bool loop, float gain = 1.0f, float pan = 0.0f,
           std::uint32_t source_sample_rate = 0);
  void stop(int handle);
  bool set_voice(int handle, float gain, float pan, std::uint32_t source_sample_rate = 0);
  void stop_all();
  bool is_playing(int handle) const;
  void set_master_gain(float gain);
  float master_gain() const;
  void mix(float* output, std::size_t frames);

 private:
  struct Voice {
    int handle{};
    std::vector<float> samples;
    std::size_t frames{};
    double cursor{};
    std::uint32_t sample_rate{};
    unsigned channels{};
    bool loop{};
    bool playing{};
    float gain{};
    float pan{};
  };

  unsigned output_channels_;
  std::uint32_t output_sample_rate_;
  float master_gain_;
  int next_handle_{1};
  mutable std::mutex mutex_;
  std::vector<Voice> voices_;
};

}  // namespace fl_port
