#include "linux_port/audio_wav.h"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

namespace {
void put16(std::vector<std::uint8_t>& b, std::uint16_t v) {
  b.push_back(static_cast<std::uint8_t>(v)); b.push_back(static_cast<std::uint8_t>(v >> 8));
}
void put32(std::vector<std::uint8_t>& b, std::uint32_t v) {
  put16(b, static_cast<std::uint16_t>(v)); put16(b, static_cast<std::uint16_t>(v >> 16));
}
void tag(std::vector<std::uint8_t>& b, const char* s) { for (int i = 0; i < 4; ++i) b.push_back(static_cast<std::uint8_t>(s[i])); }
}

int main() {
  std::vector<std::uint8_t> wav;
  tag(wav, "RIFF"); put32(wav, 40); tag(wav, "WAVE");
  tag(wav, "fmt "); put32(wav, 16); put16(wav, 1); put16(wav, 1);
  put32(wav, 22050); put32(wav, 44100); put16(wav, 2); put16(wav, 16);
  tag(wav, "data"); put32(wav, 4); put16(wav, 16384); put16(wav, 0xC000);

  fl_port::AudioPcmData pcm;
  if (!fl_port::decode_wav(wav.data(), wav.size(), pcm)) {
    std::cerr << "valid PCM16 WAV should decode\n";
    return 1;
  }
  if (pcm.sample_rate != 22050 || pcm.channels != 1 || pcm.samples.size() != 2 ||
      std::fabs(pcm.samples[0] - 0.5f) > 1e-5f || std::fabs(pcm.samples[1] + 0.5f) > 1e-5f) {
    std::cerr << "decoded metadata or PCM samples are incorrect\n";
    return 2;
  }
  if (fl_port::decode_wav(wav.data(), 8, pcm)) {
    std::cerr << "truncated WAV should fail safely\n";
    return 3;
  }
  return 0;
}
