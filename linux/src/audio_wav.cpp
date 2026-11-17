#include "linux_port/audio_wav.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace fl_port {
namespace {
std::uint16_t u16(const std::uint8_t* p) {
  return static_cast<std::uint16_t>(p[0] | (static_cast<std::uint16_t>(p[1]) << 8));
}
std::uint32_t u32(const std::uint8_t* p) {
  return static_cast<std::uint32_t>(p[0]) |
         (static_cast<std::uint32_t>(p[1]) << 8) |
         (static_cast<std::uint32_t>(p[2]) << 16) |
         (static_cast<std::uint32_t>(p[3]) << 24);
}
bool is_tag(const std::uint8_t* p, const char* tag) { return std::memcmp(p, tag, 4) == 0; }
}

bool decode_wav(const std::uint8_t* bytes, std::size_t size, AudioPcmData& out) {
  out = {};
  if (!bytes || size < 12 || !is_tag(bytes, "RIFF") || !is_tag(bytes + 8, "WAVE")) return false;
  const std::uint64_t riff_end64 = static_cast<std::uint64_t>(u32(bytes + 4)) + 8;
  if (riff_end64 > size || riff_end64 < 12) return false;
  const std::size_t riff_end = static_cast<std::size_t>(riff_end64);
  std::uint16_t format = 0, channels = 0, bits = 0;
  std::uint32_t rate = 0;
  const std::uint8_t* data = nullptr;
  std::size_t data_size = 0;
  for (std::size_t pos = 12; pos + 8 <= riff_end;) {
    const std::uint32_t chunk_size32 = u32(bytes + pos + 4);
    const std::size_t chunk_size = chunk_size32;
    const std::size_t payload = pos + 8;
    if (chunk_size > riff_end - payload) return false;
    if (is_tag(bytes + pos, "fmt ")) {
      if (chunk_size < 16) return false;
      format = u16(bytes + payload);
      channels = u16(bytes + payload + 2);
      rate = u32(bytes + payload + 4);
      bits = u16(bytes + payload + 14);
    } else if (is_tag(bytes + pos, "data")) {
      data = bytes + payload;
      data_size = chunk_size;
    }
    const std::uint64_t next = static_cast<std::uint64_t>(payload) + chunk_size + (chunk_size & 1u);
    if (next > riff_end) return false;
    pos = static_cast<std::size_t>(next);
  }
  if (!data || channels == 0 || channels > 2 || rate == 0 || (format != 1 && format != 3)) return false;
  if ((format == 1 && bits != 8 && bits != 16 && bits != 24 && bits != 32) ||
      (format == 3 && bits != 32)) return false;
  const std::size_t bytes_per_sample = bits / 8;
  if (data_size % (bytes_per_sample * channels) != 0) return false;
  const std::size_t sample_count = data_size / bytes_per_sample;
  if (sample_count > out.samples.max_size()) return false;
  out.sample_rate = rate;
  out.channels = channels;
  out.samples.reserve(sample_count);
  for (std::size_t i = 0; i < sample_count; ++i) {
    const std::uint8_t* p = data + i * bytes_per_sample;
    float sample = 0.0f;
    if (format == 3) {
      std::uint32_t raw = u32(p);
      std::memcpy(&sample, &raw, sizeof(sample));
      if (!std::isfinite(sample)) return false;
    } else if (bits == 8) {
      sample = static_cast<float>(static_cast<int>(p[0]) - 128) / 128.0f;
    } else if (bits == 16) {
      const auto value = static_cast<std::int16_t>(u16(p));
      sample = static_cast<float>(value) / 32768.0f;
    } else if (bits == 24) {
      std::int32_t value = static_cast<std::int32_t>(p[0]) |
                           (static_cast<std::int32_t>(p[1]) << 8) |
                           (static_cast<std::int32_t>(p[2]) << 16);
      if (value & 0x800000) value |= static_cast<std::int32_t>(0xff000000);
      sample = static_cast<float>(value) / 8388608.0f;
    } else {
      const auto value = static_cast<std::int32_t>(u32(p));
      sample = static_cast<float>(static_cast<double>(value) / 2147483648.0);
    }
    out.samples.push_back(std::clamp(sample, -1.0f, 1.0f));
  }
  return true;
}

}  // namespace fl_port
