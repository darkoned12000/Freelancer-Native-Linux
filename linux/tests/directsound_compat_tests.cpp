#include "dsound.h"
#include "linux_port/audio_backend.h"

#include <cstring>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

class CapturingBackend final : public fl_port::AudioBackend {
 public:
  bool init(const fl_port::AudioConfig&) override { initialized=true; return true; }
  void shutdown() override { initialized=false; }
  bool is_initialized() const override { return initialized; }
  int play_sound(const std::string&,float,bool) override { return -1; }
  int play_pcm(const float* p,std::size_t frames,unsigned ch,float volume,float pan,bool,std::uint32_t rate) override {
    captured.assign(p,p+frames*ch); channels=ch; sample_rate=rate; played_gain=volume;played_pan=pan;return 7;
  }
  bool update_pcm(int handle,float volume,float pan,std::uint32_t rate=0) override { updated_handle=handle;updated_gain=volume;updated_pan=pan;updated_rate=rate;return true; }
  void stop_sound(int h) override { stopped=h; }
  void stop_all() override {}
  bool is_playing(int) const override { return true; }
  void set_master_volume(float) override {}
  float master_volume() const override { return 1.0f; }
  void update() override {}
  bool initialized{true};
  int stopped{-1},updated_handle{-1}; unsigned channels{0},sample_rate{0},updated_rate{0};
  float updated_gain{0},updated_pan{0},played_gain{0},played_pan{0}; std::vector<float> captured;
};

int main() {
  CapturingBackend backend;
  fl_port::set_default_audio_backend(&backend);
  const GUID iid{};
  void* raw = nullptr;
  if (fl_port_create_directsound(iid, &raw) < 0 || !raw) {
    std::cerr << "software DirectSound factory should create a device\n";
    return 1;
  }
  auto* ds = static_cast<IDirectSound*>(raw);
  WAVEFORMATEX format{};
  format.wFormatTag = WAVE_FORMAT_PCM;
  format.nChannels = 1;
  format.nSamplesPerSec = 44100;
  format.nAvgBytesPerSec = 88200;
  format.nBlockAlign = 2;
  format.wBitsPerSample = 16;
  DSBUFFERDESC desc{};
  desc.dwSize = sizeof(desc);
  desc.dwBufferBytes = 8;
  desc.lpwfxFormat = &format;
  IDirectSoundBuffer* buffer = nullptr;
  if (ds->CreateSoundBuffer(&desc, &buffer, nullptr) < 0 || !buffer) return 2;
  void* first = nullptr;
  DWORD first_bytes = 0;
  if (buffer->Lock(0, 0, &first, &first_bytes, nullptr, nullptr, DSBLOCK_ENTIREBUFFER) < 0 || !first || first_bytes != 8) return 3;
  const std::int16_t samples[4] = {16384, -16384, 8192, -8192};
  std::memcpy(first, samples, sizeof(samples));
  if (buffer->Unlock(first, first_bytes, nullptr, 0) < 0) return 4;
  unsigned long required = 0;
  if (buffer->GetFormat(nullptr, 0, &required) < 0 || required != sizeof(WAVEFORMATEX)) return 5;
  if (buffer->Play(0, 0, DSBPLAY_LOOPING) < 0 || backend.captured.size()!=4 ||
      backend.channels!=1 || backend.sample_rate!=44100 || std::fabs(backend.captured[0]-0.5f)>1e-5f) return 6;
  buffer->SetVolume(-6000);
  if(backend.updated_handle!=7 || std::fabs(backend.updated_gain-0.001f)>0.0002f) return 7;
  buffer->Stop();
  if(backend.stopped!=7) return 8;
  buffer->SetCurrentPosition(4);
  if(buffer->Play(0,0,0)<0 || backend.captured.empty() ||
     std::fabs(backend.captured[0]-0.25f)>1e-5f) return 9;
  buffer->Stop();
  void* raw3d=nullptr;
  if(buffer->QueryInterface(IID_IDirectSound3DBuffer,&raw3d)<0 || !raw3d) return 10;
  auto* spatial=static_cast<IDirectSound3DBuffer*>(raw3d);
  spatial->SetPosition(10.0f,0.0f,10.0f,DS3D_IMMEDIATE);
  if(buffer->Play(0,0,0)<0 || std::fabs(backend.played_pan-0.7071067f)>0.01f) return 11;
  spatial->Release();
  buffer->Stop();
  buffer->Release();
  ds->Release();
  fl_port::set_default_audio_backend(nullptr);
  return 0;
}
