#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>

namespace fl_port {

struct AudioPcmData {
  std::uint32_t sample_rate{0};
  std::uint16_t channels{0};
  std::vector<float> samples;  // interleaved, normalized to [-1, 1]
};

bool decode_wav(const std::uint8_t* bytes, std::size_t size, AudioPcmData& out);

}  // namespace fl_port
