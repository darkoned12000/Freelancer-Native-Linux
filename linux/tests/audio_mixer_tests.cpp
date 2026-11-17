#include "linux_port/audio_mixer.h"

#include <cmath>
#include <iostream>
#include <vector>

namespace {
bool near(float actual, float expected) { return std::fabs(actual - expected) < 1e-5f; }
}

int main() {
  fl_port::AudioMixer mixer(2, 1.0f);
  const std::vector<float> mono{0.5f, -0.5f};
  const int handle = mixer.play(mono.data(), mono.size(), 1, false, 1.0f, 0.0f);
  if (handle <= 0) return 1;

  std::vector<float> out(4);
  mixer.mix(out.data(), 2);
  if (!near(out[0], 0.5f) || !near(out[1], 0.5f) ||
      !near(out[2], -0.5f) || !near(out[3], -0.5f)) {
    std::cerr << "mono source should be centered and duplicated to stereo\n";
    return 2;
  }
  if (mixer.is_playing(handle)) {
    std::cerr << "non-looping source should finish at end of samples\n";
    return 3;
  }

  const int loop = mixer.play(mono.data(), mono.size(), 1, true, 0.5f, -1.0f);
  mixer.mix(out.data(), 3);
  if (!mixer.is_playing(loop) || !near(out[0], 0.25f) || !near(out[1], 0.0f)) {
    std::cerr << "looping source should wrap and apply gain plus hard-left pan\n";
    return 4;
  }

  mixer.stop(loop);
  if (mixer.is_playing(loop)) return 5;

  fl_port::AudioMixer resampler(2, 1.0f, 44100);
  const int half_rate = resampler.play(mono.data(), mono.size(), 1, false, 1.0f, 0.0f, 22050);
  if (half_rate <= 0) return 6;
  std::vector<float> resampled(8);
  resampler.mix(resampled.data(), 4);
  if (!near(resampled[0], 0.5f) || !near(resampled[2], 0.0f) ||
      !near(resampled[4], -0.5f) || !near(resampled[6], -0.5f)) {
    std::cerr << "half-rate samples should be resampled at output rate\n";
    return 7;
  }

  fl_port::AudioMixer live(2, 1.0f);
  const int live_handle = live.play(mono.data(), mono.size(), 1, true, 1.0f, 0.0f);
  if (!live.set_voice(live_handle, 0.5f, -1.0f)) return 8;
  float live_out[2]{};
  live.mix(live_out, 1);
  if (!near(live_out[0], 0.25f) || !near(live_out[1], 0.0f)) {
    std::cerr << "live source controls should update gain and pan\n";
    return 9;
  }

  fl_port::AudioMixer respeed(2, 1.0f, 44100);
  const int speed_handle = respeed.play(mono.data(), mono.size(), 1, true);
  if (!respeed.set_voice(speed_handle, 1.0f, 0.0f, 22050)) return 10;
  float speed_out[4]{};
  respeed.mix(speed_out, 2);
  if (!near(speed_out[0], 0.5f) || !near(speed_out[2], 0.0f)) {
    std::cerr << "live frequency changes should alter source resampling rate\n";
    return 11;
  }

  return 0;
}
