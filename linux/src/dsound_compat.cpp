#include "dsound.h"
#include "linux_port/audio_backend.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#include <vector>

namespace {
constexpr HRESULT kNoInterface = static_cast<HRESULT>(0x80004002u);
constexpr GUID kDs3dBufferIid{0x279AFA86, 0x4981, 0x11CE, {0xA5,0x21,0,0x20,0xAF,0x0B,0xE5,0x60}};
constexpr GUID kDs3dListenerIid{0x279AFA84, 0x4981, 0x11CE, {0xA5,0x21,0,0x20,0xAF,0x0B,0xE5,0x60}};
constexpr GUID kDsNotifyIid{0xB0210783, 0x89CD, 0x11D0, {0xAF,0x08,0,0xA0,0xC9,0x25,0xCD,0x16}};
constexpr GUID kKsPropertyIid{0x31EFAC30, 0x515C, 0x11D0, {0xA9,0xAA,0,0xAA,0,0x61,0xBE,0x93}};
constexpr GUID kDirectSoundIid{0x279AFA83, 0x4981, 0x11CE, {0xA5,0x21,0,0x20,0xAF,0x0B,0xE5,0x60}};

bool guid_equal(const GUID& a, const GUID& b) {
  return a.Data1 == b.Data1 && a.Data2 == b.Data2 && a.Data3 == b.Data3 &&
         std::memcmp(a.Data4, b.Data4, sizeof(a.Data4)) == 0;
}
float ds_volume_gain(long volume) {
  volume = std::clamp(volume, -10000L, 0L);
  return std::pow(10.0f, static_cast<float>(volume) / 2000.0f);
}
struct ListenerState {
  float x{0}, y{0}, z{0};
  float distance_factor{1.0f}, doppler_factor{1.0f}, rolloff_factor{1.0f};
};

class SoftwareBuffer final : public IDirectSoundBuffer,
                             public IDirectSound3DBuffer,
                             public IDirectSound3DListener,
                             public IDirectSoundNotify,
                             public IKsPropertySet {
 public:
  SoftwareBuffer(const DSBUFFERDESC* desc, std::shared_ptr<ListenerState> listener)
      : listener_(std::move(listener)) {
    primary_ = desc && (desc->dwFlags & DSBCAPS_PRIMARYBUFFER) != 0;
    if (desc && desc->lpwfxFormat) format_ = *desc->lpwfxFormat;
    if (desc && desc->dwBufferBytes) bytes_.resize(desc->dwBufferBytes);
    if (bytes_.empty()) bytes_.resize(4);
  }
  HRESULT QueryInterface(const GUID& iid, void** out) override {
    if (!out) return kNoInterface;
    *out = nullptr;
    if (guid_equal(iid, kDirectSoundIid)) *out = static_cast<IDirectSoundBuffer*>(this);
    else if (guid_equal(iid, kDs3dBufferIid) && !primary_) *out = static_cast<IDirectSound3DBuffer*>(this);
    else if (guid_equal(iid, kDs3dListenerIid) && primary_) *out = static_cast<IDirectSound3DListener*>(this);
    else if (guid_equal(iid, kDsNotifyIid)) *out = static_cast<IDirectSoundNotify*>(this);
    else if (guid_equal(iid, kKsPropertyIid)) *out = static_cast<IKsPropertySet*>(this);
    else return kNoInterface;
    AddRef();
    return DS_OK;
  }
  ULONG AddRef() override { return ++refs_; }
  ULONG Release() override { const ULONG n = --refs_; if (!n) delete this; return n; }

  HRESULT GetCaps(DSBCAPS* caps) override {
    if (!caps) return DSERR_GENERIC;
    caps->dwSize = sizeof(*caps); caps->dwFlags = flags_;
    caps->dwBufferBytes = static_cast<DWORD>(bytes_.size());
    caps->dwUnlockTransferRate = 0; caps->dwPlayCpuOverhead = 0; return DS_OK;
  }
  HRESULT GetCurrentPosition(unsigned long* play, unsigned long* write) override {
    DWORD p=0,w=0; const HRESULT hr=GetCurrentPosition(&p,&w);
    if (play) *play=p; if (write) *write=w; return hr;
  }
  HRESULT GetCurrentPosition(DWORD* play, DWORD* write) override {
    DWORD p=0;
    if (playing_ && format_.nAvgBytesPerSec) {
      const auto elapsed = std::chrono::steady_clock::now() - started_;
      const auto us = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
      p = static_cast<DWORD>((static_cast<unsigned long long>(us) * format_.nAvgBytesPerSec / 1000000u) % bytes_.size());
    }
    if (play) *play=p; if (write) *write=p; return DS_OK;
  }
  HRESULT GetFormat(WAVEFORMATEX* out, DWORD size, unsigned long* written) override {
    if (written) *written = sizeof(format_);
    if (out && size >= sizeof(format_)) *out = format_;
    return DS_OK;
  }
  HRESULT GetVolume(long* out) override { if (!out) return DSERR_GENERIC; *out=volume_; return DS_OK; }
  HRESULT GetPan(long* out) override { if (!out) return DSERR_GENERIC; *out=pan_; return DS_OK; }
  HRESULT GetFrequency(unsigned long* out) override { if (!out) return DSERR_GENERIC; *out=frequency_; return DS_OK; }
  HRESULT GetFrequency(DWORD* out) override { if (!out) return DSERR_GENERIC; *out=frequency_; return DS_OK; }
  HRESULT GetStatus(unsigned long* out) override { if (!out) return DSERR_GENERIC; *out=current_status(); return DS_OK; }
  HRESULT GetStatus(DWORD* out) override { if (!out) return DSERR_GENERIC; *out=current_status(); return DS_OK; }
  HRESULT Initialize(void*, DSBUFFERDESC* desc) override {
    if (!desc) return DSERR_GENERIC;
    primary_ = (desc->dwFlags & DSBCAPS_PRIMARYBUFFER) != 0;
    flags_ = desc->dwFlags;
    if (desc->lpwfxFormat) format_ = *desc->lpwfxFormat;
    if (desc->dwBufferBytes) bytes_.resize(desc->dwBufferBytes);
    return DS_OK;
  }
  HRESULT Lock(DWORD offset, DWORD requested, void** p1, DWORD* n1, void** p2, DWORD* n2, DWORD flags) override {
    (void)flags;
    if (!p1 || !n1 || offset >= bytes_.size()) return DSERR_GENERIC;
    if (requested == 0 || requested > bytes_.size()) requested = static_cast<DWORD>(bytes_.size());
    *p1 = bytes_.data() + offset;
    *n1 = std::min<DWORD>(requested, static_cast<DWORD>(bytes_.size() - offset));
    const DWORD rest = requested - *n1;
    if (p2) *p2 = rest ? bytes_.data() : nullptr;
    if (n2) *n2 = rest;
    return DS_OK;
  }
  HRESULT Play(DWORD, DWORD, DWORD flags) override {
    loop_ = (flags & DSBPLAY_LOOPING) != 0;
    playing_ = true; started_ = std::chrono::steady_clock::now();
    auto* backend = fl_port::default_audio_backend();
    if (!primary_ && backend && format_.nChannels && format_.nBlockAlign && !bytes_.empty()) {
      auto pcm = decode();
      const std::size_t frames = pcm.size() / format_.nChannels;
      const std::size_t start_frame = frames ? std::min<std::size_t>(cursor_ / format_.nBlockAlign, frames - 1) : 0;
      if (frames && start_frame) {
        const auto split = pcm.begin() + static_cast<std::ptrdiff_t>(start_frame * format_.nChannels);
        if (loop_) std::rotate(pcm.begin(), split, pcm.end());
        else pcm.erase(pcm.begin(), split);
      }
      const float gain = ds_volume_gain(volume_) * distance_gain();
      const float pan = mode_ == DS3DMODE_DISABLE ? static_cast<float>(pan_) / 10000.0f : spatial_pan();
      voice_ = backend->play_pcm(pcm.data(), pcm.size()/format_.nChannels, format_.nChannels,
                                 gain, pan,
                                 loop_, format_.nSamplesPerSec);
    }
    return DS_OK;
  }
  HRESULT SetCurrentPosition(DWORD pos) override { cursor_=pos; return DS_OK; }
  HRESULT SetFormat(WAVEFORMATEX* f) override { if (!f) return DSERR_GENERIC; format_=*f; return DS_OK; }
  HRESULT SetVolume(LONG volume) override { volume_=volume; refresh_voice(); return DS_OK; }
  HRESULT SetPan(LONG pan) override { pan_=pan; refresh_voice(); return DS_OK; }
  HRESULT SetFrequency(DWORD frequency) override { frequency_=frequency; refresh_voice(); return DS_OK; }
  HRESULT Stop() override {
    if (voice_ >= 0) if (auto* b=fl_port::default_audio_backend()) b->stop_sound(voice_);
    voice_=-1; playing_=false; return DS_OK;
  }
  HRESULT Unlock(void*, DWORD, void*, DWORD) override { return DS_OK; }
  HRESULT Restore() override { return DS_OK; }
  HRESULT SetFX(DWORD, void*, DWORD*) override { return kNoInterface; }
  HRESULT GetObjectInPath(const GUID&, DWORD, const GUID&, void**) override { return kNoInterface; }

  // 3D source methods.
  HRESULT GetAllParameters(DS3DBUFFER* p) override { if (!p) return DSERR_GENERIC; p->dwSize=sizeof(*p); p->vPosition={x_,y_,z_}; p->vVelocity={vx_,vy_,vz_}; p->flMinDistance=min_distance_; p->flMaxDistance=max_distance_; p->dwMode=mode_; return DS_OK; }
  HRESULT GetConeAngles(DWORD* a,DWORD* b) override { if(a)*a=cone_inside_; if(b)*b=cone_outside_; return DS_OK; }
  HRESULT GetConeOrientation(float* x,float* y,float* z) override { if(x)*x=cone_x_; if(y)*y=cone_y_; if(z)*z=cone_z_; return DS_OK; }
  HRESULT GetConeOutsideVolume(LONG* v) override { if(v)*v=cone_volume_; return DS_OK; }
  HRESULT GetMaxDistance(float* v) override { if(v)*v=max_distance_; return DS_OK; }
  HRESULT GetMinDistance(float* v) override { if(v)*v=min_distance_; return DS_OK; }
  HRESULT GetMode(DWORD* v) override { if(v)*v=mode_; return DS_OK; }
  HRESULT GetPosition(float* x,float* y,float* z) override { if(primary_){if(x)*x=listener_->x;if(y)*y=listener_->y;if(z)*z=listener_->z;}else{if(x)*x=x_;if(y)*y=y_;if(z)*z=z_;}return DS_OK; }
  HRESULT GetVelocity(float* x,float* y,float* z) override { if(x)*x=vx_;if(y)*y=vy_;if(z)*z=vz_;return DS_OK; }
  HRESULT SetAllParameters(const DS3DBUFFER* p,DWORD) override { if(!p)return DSERR_GENERIC;x_=p->vPosition.x;y_=p->vPosition.y;z_=p->vPosition.z;min_distance_=p->flMinDistance;max_distance_=p->flMaxDistance;mode_=p->dwMode;refresh_voice();return DS_OK; }
  HRESULT SetConeAngles(DWORD a,DWORD b,DWORD) override { cone_inside_=a;cone_outside_=b;return DS_OK; }
  HRESULT SetConeOrientation(float x,float y,float z,DWORD) override { cone_x_=x;cone_y_=y;cone_z_=z;return DS_OK; }
  HRESULT SetConeOutsideVolume(LONG v,DWORD) override { cone_volume_=v;return DS_OK; }
  HRESULT SetMaxDistance(float v,DWORD) override { max_distance_=v;refresh_voice();return DS_OK; }
  HRESULT SetMinDistance(float v,DWORD) override { min_distance_=v;refresh_voice();return DS_OK; }
  HRESULT SetMode(DWORD v,DWORD) override { mode_=v;refresh_voice();return DS_OK; }
  HRESULT SetPosition(float x,float y,float z,DWORD) override { if(primary_){listener_->x=x;listener_->y=y;listener_->z=z;}else{x_=x;y_=y;z_=z;refresh_voice();}return DS_OK; }
  HRESULT SetVelocity(float x,float y,float z,DWORD) override { if(!primary_){vx_=x;vy_=y;vz_=z;}return DS_OK; }

  // Listener methods used by the primary buffer.
  HRESULT GetAllParameters(void*) override { return DS_OK; }
  HRESULT GetDistanceFactor(float* v) override { if(v)*v=listener_->distance_factor;return DS_OK; }
  HRESULT GetDopplerFactor(float* v) override { if(v)*v=listener_->doppler_factor;return DS_OK; }
  HRESULT GetOrientation(float* fx,float* fy,float* fz,float* ux,float* uy,float* uz) override { if(fx)*fx=0;if(fy)*fy=0;if(fz)*fz=1;if(ux)*ux=0;if(uy)*uy=1;if(uz)*uz=0;return DS_OK; }
  HRESULT GetRolloffFactor(float* v) override { if(v)*v=listener_->rolloff_factor;return DS_OK; }
  HRESULT SetAllParameters(const void*,DWORD) override { return DS_OK; }
  HRESULT SetDistanceFactor(float v,DWORD) override { listener_->distance_factor=v;refresh_voice();return DS_OK; }
  HRESULT SetDopplerFactor(float v,DWORD) override { listener_->doppler_factor=v;return DS_OK; }
  HRESULT SetOrientation(float,float,float,float,float,float,DWORD) override { return DS_OK; }
  HRESULT SetRolloffFactor(float v,DWORD) override { listener_->rolloff_factor=v;refresh_voice();return DS_OK; }
  HRESULT CommitDeferredSettings() override { return DS_OK; }

  HRESULT SetNotificationPositions(DWORD, DSBPOSITIONNOTIFY*) override { return DS_OK; }
  HRESULT Get(const GUID&,DWORD,void*,DWORD,void*,DWORD,unsigned long*) override { return kNoInterface; }
  HRESULT Set(const GUID&,DWORD,void*,DWORD,void*,DWORD) override { return kNoInterface; }
  HRESULT QuerySupport(const GUID&,DWORD,DWORD* support) override { if(support)*support=0; return kNoInterface; }

 private:
  DWORD current_status() const {
    if (!playing_) return 0;
    if (voice_ >= 0) {
      if (auto* backend=fl_port::default_audio_backend())
        return backend->is_playing(voice_) ? (DSBSTATUS_PLAYING | (loop_ ? DSBSTATUS_LOOPING : 0)) : 0;
    }
    return DSBSTATUS_PLAYING | (loop_ ? DSBSTATUS_LOOPING : 0);
  }
  std::vector<float> decode() const {
    std::vector<float> out;
    const unsigned bits = format_.wBitsPerSample;
    if (format_.wFormatTag != WAVE_FORMAT_PCM && format_.wFormatTag != WAVE_FORMAT_IEEE_FLOAT) return out;
    const unsigned bytes_per_sample = bits / 8;
    if (bytes_per_sample == 0) return out;
    const std::size_t count = bytes_.size() / bytes_per_sample;
    out.reserve(count);
    for (std::size_t i=0;i<count;++i) {
      const auto* p=reinterpret_cast<const unsigned char*>(bytes_.data()+i*bytes_per_sample);
      float s=0;
      if(format_.wFormatTag==WAVE_FORMAT_IEEE_FLOAT && bits==32) std::memcpy(&s,p,4);
      else if(bits==8) s=static_cast<float>(static_cast<int>(p[0])-128)/128.0f;
      else if(bits==16) { std::int16_t v; std::memcpy(&v,p,2); s=static_cast<float>(v)/32768.0f; }
      else if(bits==24) { std::int32_t v=p[0]|(p[1]<<8)|(p[2]<<16);if(v&0x800000)v|=0xff000000;s=static_cast<float>(v)/8388608.0f; }
      else if(bits==32) { std::int32_t v;std::memcpy(&v,p,4);s=static_cast<float>(static_cast<double>(v)/2147483648.0); }
      out.push_back(std::clamp(s,-1.0f,1.0f));
    }
    return out;
  }
  void refresh_voice() {
    if (voice_ < 0) return;
    if (auto* b=fl_port::default_audio_backend()) {
      const float gain=ds_volume_gain(volume_)*distance_gain();
      const float pan=mode_==DS3DMODE_DISABLE?static_cast<float>(pan_)/10000.0f:spatial_pan();
      b->update_pcm(voice_,gain,pan,frequency_);
    }
  }
  float distance_gain() const {
    const float dx=x_-listener_->x,dy=y_-listener_->y,dz=z_-listener_->z;
    const float dist=std::sqrt(dx*dx+dy*dy+dz*dz)*listener_->distance_factor;
    if(dist<=min_distance_)return 1.0f;
    if(dist>=max_distance_)return 0.0f;
    const float span=std::max(1e-5f,max_distance_-min_distance_);
    return std::clamp(1.0f-listener_->rolloff_factor*(dist-min_distance_)/span,0.0f,1.0f);
  }
  float spatial_pan() const {
    const float dx=x_-listener_->x,dz=z_-listener_->z;
    const float distance=std::sqrt(dx*dx+dz*dz);
    return distance>1e-5f?std::clamp(dx/distance,-1.0f,1.0f):0.0f;
  }

  ULONG refs_{1};
  bool primary_{false}, playing_{false}, loop_{false};
  DWORD flags_{0}, frequency_{0}, cursor_{0};
  long volume_{0}, pan_{0};
  int voice_{-1};
  WAVEFORMATEX format_{};
  std::vector<unsigned char> bytes_;
  std::shared_ptr<ListenerState> listener_;
  std::chrono::steady_clock::time_point started_{};
  float x_{0},y_{0},z_{0},vx_{0},vy_{0},vz_{0};
  float min_distance_{1.0f},max_distance_{1.0e9f};
  DWORD mode_{DS3DMODE_NORMAL},cone_inside_{360},cone_outside_{360};
  float cone_x_{0},cone_y_{0},cone_z_{1};
  long cone_volume_{0};
};

class SoftwareDirectSound final : public IDirectSound {
 public:
  SoftwareDirectSound(): listener_(std::make_shared<ListenerState>()) {}
  HRESULT QueryInterface(const GUID&,void** out) override { if(!out)return kNoInterface;*out=static_cast<IDirectSound*>(this);AddRef();return DS_OK; }
  ULONG AddRef() override { return ++refs_; }
  ULONG Release() override { ULONG n=--refs_;if(!n)delete this;return n; }
  HRESULT CreateSoundBuffer(DSBUFFERDESC* desc,IDirectSoundBuffer** out,void*) override { if(!desc||!out)return DSERR_GENERIC;*out=new(std::nothrow) SoftwareBuffer(desc,listener_);return *out?DS_OK:DSERR_GENERIC; }
  HRESULT GetCaps(DSCAPS* caps) override { if(!caps)return DSERR_GENERIC;caps->dwFlags=DSCAPS_EMULDRIVER;return DS_OK; }
  HRESULT DuplicateSoundBuffer(IDirectSoundBuffer* src,IDirectSoundBuffer** out) override { if(!src||!out)return DSERR_GENERIC;DSBCAPS c{};src->GetCaps(&c);DSBUFFERDESC d{};d.dwSize=sizeof(d);d.dwFlags=c.dwFlags;d.dwBufferBytes=c.dwBufferBytes;WAVEFORMATEX f{};src->GetFormat(&f,sizeof(f),nullptr);d.lpwfxFormat=&f;auto* b=new(std::nothrow) SoftwareBuffer(&d,listener_);if(!b)return DSERR_GENERIC;void *sp=nullptr,*dp=nullptr;DWORD sn=0,dn=0;if(src->Lock(0,0,&sp,&sn,nullptr,nullptr,DSBLOCK_ENTIREBUFFER)>=0&&b->Lock(0,0,&dp,&dn,nullptr,nullptr,DSBLOCK_ENTIREBUFFER)>=0){std::memcpy(dp,sp,std::min(sn,dn));src->Unlock(sp,sn,nullptr,0);b->Unlock(dp,dn,nullptr,0);}*out=b;return DS_OK; }
  HRESULT SetCooperativeLevel(HWND,DWORD) override { return DS_OK; }
  HRESULT Compact() override { return DS_OK; }
  HRESULT GetSpeakerConfig(DWORD* c) override { if(c)*c=DSSPEAKER_STEREO;return DS_OK; }
  HRESULT SetSpeakerConfig(DWORD) override { return DS_OK; }
  HRESULT Initialize(const GUID*) override { return DS_OK; }
 private: ULONG refs_{1}; std::shared_ptr<ListenerState> listener_;
};
}

HRESULT fl_port_create_directsound(const GUID& iid, void** out) {
  (void)iid;
  if(!out)return kNoInterface;
  *out=static_cast<IDirectSound*>(new(std::nothrow) SoftwareDirectSound());
  return *out?DS_OK:DSERR_GENERIC;
}
