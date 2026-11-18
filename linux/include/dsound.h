#pragma once

#include "linux_port/win32_compat.h"
#include "mmreg.h"

#ifndef DIRECTSOUND_VERSION
#define DIRECTSOUND_VERSION 0x0800
#endif

struct _DSCAPS { DWORD dwSize, dwFlags, dwMinSecondarySampleRate, dwMaxSecondarySampleRate,
    dwPrimaryBuffers, dwMaxHwMixingAllBuffers, dwMaxHwMixingStaticBuffers,
    dwMaxHwMixingStreamingBuffers, dwFreeHwMixingAllBuffers, dwFreeHwMixingStaticBuffers,
    dwFreeHwMixingStreamingBuffers, dwMaxHw3DAllBuffers, dwMaxHw3DStaticBuffers,
    dwMaxHw3DStreamingBuffers, dwFreeHw3DAllBuffers, dwFreeHw3DStaticBuffers,
    dwFreeHw3DStreamingBuffers, dwTotalHwMemBytes, dwFreeHwMemBytes, dwMaxContigFreeHwMemBytes,
    dwUnlockTransferRateHwBuffers, dwPlayCpuOverheadSwBuffers, dwReserved1, dwReserved2; };
struct _DSBCAPS { DWORD dwSize, dwFlags, dwBufferBytes, dwUnlockTransferRate, dwPlayCpuOverhead; };
struct _DSBUFFERDESC { DWORD dwSize, dwFlags, dwBufferBytes, dwReserved; WAVEFORMATEX* lpwfxFormat; GUID guid3DAlgorithm; };
using DSCAPS = _DSCAPS;
using DSBCAPS = _DSBCAPS;
using DSBUFFERDESC = _DSBUFFERDESC;
using LPDSBUFFERDESC = DSBUFFERDESC*;

struct DSBPOSITIONNOTIFY { DWORD dwOffset; HANDLE hEventNotify; };
using LPDSBPOSITIONNOTIFY = DSBPOSITIONNOTIFY*;
#ifndef FL_PORT_D3DVECTOR_DEFINED
#define FL_PORT_D3DVECTOR_DEFINED
struct D3DVECTOR { float x, y, z; };
#endif
struct DS3DBUFFER {
  DWORD dwSize;
  D3DVECTOR vPosition;
  D3DVECTOR vVelocity;
  DWORD dwInsideConeAngle;
  DWORD dwOutsideConeAngle;
  D3DVECTOR vConeOrientation;
  LONG lConeOutsideVolume;
  float flMinDistance;
  float flMaxDistance;
  DWORD dwMode;
};

#define DS_OK 0
#define DSERR_GENERIC ((HRESULT)0x80004005L)
#define DSBLOCK_ENTIREBUFFER 0x00000002
#define DSBPLAY_LOOPING 0x00000001
#define DSBPLAY_LOCHARDWARE 0x00000002
#define DSBPLAY_LOCSOFTWARE 0x00000004
#define DSBSTATUS_PLAYING 0x00000001
#define DSBSTATUS_LOOPING 0x00000004
#define DSBCAPS_CTRL3D 0x00000010
#define DSBCAPS_CTRLFREQUENCY 0x00000020
#define DSBCAPS_CTRLPAN 0x00000040
#define DSBCAPS_CTRLVOLUME 0x00000080
#define DSBCAPS_GLOBALFOCUS 0x00008000
#define DSBCAPS_PRIMARYBUFFER 0x00000001
#define DSBCAPS_LOCDEFER 0x00004000
#define DSBCAPS_GETCURRENTPOSITION2 0x00010000
#define DSBCAPS_MUTE3DATMAXDISTANCE 0x00020000
#define DSCAPS_EMULDRIVER 0x00000020
#define DSBCAPS_LOCHARDWARE 0x00000002
#define DSBCAPS_LOCSOFTWARE 0x00000008
#define DSSCL_NORMAL 1
#define DSSCL_PRIORITY 2
#define DS3D_DEFERRED 0x00000001
#define DS3D_IMMEDIATE 0x00000000
#define DS3DMODE_NORMAL 0
#define DS3DMODE_HEADRELATIVE 1
#define DS3DMODE_DISABLE 2
#define DS3D_DEFAULTMINDISTANCE 1.0f
#define DS3D_DEFAULTMAXDISTANCE 1000000000.0f
#define DS3D_DEFAULTROLLOFFFACTOR 1.0f
#define DS3D_DEFAULTDOPPLERFACTOR 1.0f
#define DS3D_DEFAULTDISTANCEFACTOR 1.0f
#define DS_CERTIFIED 0x00000001
#define DSBCAPS_CTRLFX 0x00000200
#define DSBFREQUENCY_MIN 100
#define DSBFREQUENCY_MAX 200000
#define DSBSIZE_MIN 4
#define DSBSIZE_MAX 0x0FFFFFFF
#define DSBPAN_LEFT -10000
#define DSBPAN_RIGHT 10000
#define DSBPAN_CENTER 0

struct IDirectSoundBuffer;
struct IDirectSound3DBuffer;
struct IDirectSound3DListener;
struct IKsPropertySet;
struct IDirectSoundNotify {
  virtual HRESULT QueryInterface(const GUID&, void**) = 0;
  virtual ULONG AddRef() = 0;
  virtual ULONG Release() = 0;
  virtual HRESULT SetNotificationPositions(DWORD, DSBPOSITIONNOTIFY*) = 0;
};
inline const GUID IID_IDirectSound{0x279AFA83,0x4981,0x11CE,{0xA5,0x21,0,0x20,0xAF,0x0B,0xE5,0x60}};
inline const GUID IID_IDirectSound3DBuffer{0x279AFA86,0x4981,0x11CE,{0xA5,0x21,0,0x20,0xAF,0x0B,0xE5,0x60}};
inline const GUID IID_IDirectSound3DListener{0x279AFA84,0x4981,0x11CE,{0xA5,0x21,0,0x20,0xAF,0x0B,0xE5,0x60}};

struct IDirectSoundBuffer {
  virtual HRESULT QueryInterface(const GUID&, void**) = 0;
  virtual ULONG AddRef() = 0;
  virtual ULONG Release() = 0;
  virtual HRESULT GetCaps(DSBCAPS*) = 0;
  virtual HRESULT GetCurrentPosition(unsigned long*, unsigned long*) = 0;
  virtual HRESULT GetCurrentPosition(DWORD*, DWORD*) = 0;
  virtual HRESULT GetFormat(WAVEFORMATEX*, DWORD, unsigned long*) = 0;
  virtual HRESULT GetVolume(long*) = 0;
  virtual HRESULT GetPan(long*) = 0;
  virtual HRESULT GetFrequency(unsigned long*) = 0;
  virtual HRESULT GetFrequency(DWORD*) = 0;
  virtual HRESULT GetStatus(unsigned long*) = 0;
  virtual HRESULT GetStatus(DWORD*) = 0;
  virtual HRESULT Initialize(void*, DSBUFFERDESC*) = 0;
  virtual HRESULT Lock(DWORD, DWORD, void**, DWORD*, void**, DWORD*, DWORD) = 0;
  virtual HRESULT Play(DWORD, DWORD, DWORD) = 0;
  virtual HRESULT SetCurrentPosition(DWORD) = 0;
  virtual HRESULT SetFormat(WAVEFORMATEX*) = 0;
  virtual HRESULT SetVolume(LONG) = 0;
  virtual HRESULT SetPan(LONG) = 0;
  virtual HRESULT SetFrequency(DWORD) = 0;
  virtual HRESULT Stop() = 0;
  virtual HRESULT Unlock(void*, DWORD, void*, DWORD) = 0;
  virtual HRESULT Restore() = 0;
  virtual HRESULT SetFX(DWORD, void*, DWORD*) = 0;
  virtual HRESULT GetObjectInPath(const GUID&, DWORD, const GUID&, void**) = 0;
};

struct IDirectSound3DBuffer {
  virtual HRESULT QueryInterface(const GUID&, void**) = 0;
  virtual ULONG AddRef() = 0;
  virtual ULONG Release() = 0;
  virtual HRESULT GetAllParameters(DS3DBUFFER*) = 0;
  virtual HRESULT GetConeAngles(DWORD*, DWORD*) = 0;
  virtual HRESULT GetConeOrientation(float*, float*, float*) = 0;
  virtual HRESULT GetConeOutsideVolume(LONG*) = 0;
  virtual HRESULT GetMaxDistance(float*) = 0;
  virtual HRESULT GetMinDistance(float*) = 0;
  virtual HRESULT GetMode(DWORD*) = 0;
  virtual HRESULT GetPosition(float*, float*, float*) = 0;
  virtual HRESULT GetVelocity(float*, float*, float*) = 0;
  virtual HRESULT SetAllParameters(const DS3DBUFFER*, DWORD) = 0;
  virtual HRESULT SetConeAngles(DWORD, DWORD, DWORD) = 0;
  virtual HRESULT SetConeOrientation(float, float, float, DWORD) = 0;
  virtual HRESULT SetConeOutsideVolume(LONG, DWORD) = 0;
  virtual HRESULT SetMaxDistance(float, DWORD) = 0;
  virtual HRESULT SetMinDistance(float, DWORD) = 0;
  virtual HRESULT SetMode(DWORD, DWORD) = 0;
  virtual HRESULT SetPosition(float, float, float, DWORD) = 0;
  virtual HRESULT SetVelocity(float, float, float, DWORD) = 0;
};

struct IDirectSound3DListener {
  virtual HRESULT QueryInterface(const GUID&, void**) = 0;
  virtual ULONG AddRef() = 0;
  virtual ULONG Release() = 0;
  virtual HRESULT GetAllParameters(void*) = 0;
  virtual HRESULT GetDistanceFactor(float*) = 0;
  virtual HRESULT GetDopplerFactor(float*) = 0;
  virtual HRESULT GetOrientation(float*, float*, float*, float*, float*, float*) = 0;
  virtual HRESULT GetPosition(float*, float*, float*) = 0;
  virtual HRESULT GetRolloffFactor(float*) = 0;
  virtual HRESULT SetAllParameters(const void*, DWORD) = 0;
  virtual HRESULT SetDistanceFactor(float, DWORD) = 0;
  virtual HRESULT SetDopplerFactor(float, DWORD) = 0;
  virtual HRESULT SetOrientation(float, float, float, float, float, float, DWORD) = 0;
  virtual HRESULT SetPosition(float, float, float, DWORD) = 0;
  virtual HRESULT SetVelocity(float, float, float, DWORD) = 0;
  virtual HRESULT SetRolloffFactor(float, DWORD) = 0;
  virtual HRESULT CommitDeferredSettings() = 0;
};

struct IDirectSound {
  virtual HRESULT QueryInterface(const GUID&, void**) = 0;
  virtual ULONG AddRef() = 0;
  virtual ULONG Release() = 0;
  virtual HRESULT CreateSoundBuffer(DSBUFFERDESC*, IDirectSoundBuffer**, void*) = 0;
  virtual HRESULT GetCaps(DSCAPS*) = 0;
  virtual HRESULT DuplicateSoundBuffer(IDirectSoundBuffer*, IDirectSoundBuffer**) = 0;
  virtual HRESULT SetCooperativeLevel(HWND, DWORD) = 0;
  virtual HRESULT Compact() = 0;
  virtual HRESULT GetSpeakerConfig(DWORD*) = 0;
  virtual HRESULT SetSpeakerConfig(DWORD) = 0;
  virtual HRESULT Initialize(const GUID*) = 0;
};

struct IKsPropertySet {
  virtual HRESULT QueryInterface(const GUID&, void**) = 0;
  virtual ULONG AddRef() = 0;
  virtual ULONG Release() = 0;
  virtual HRESULT Get(const GUID&, DWORD, void*, DWORD, void*, DWORD, unsigned long*) = 0;
  virtual HRESULT Set(const GUID&, DWORD, void*, DWORD, void*, DWORD) = 0;
  virtual HRESULT QuerySupport(const GUID&, DWORD, DWORD*) = 0;
};

using LPDIRECTSOUND = IDirectSound*;
using LPDIRECTSOUNDBUFFER = IDirectSoundBuffer*;
using LPDIRECTSOUND3DBUFFER = IDirectSound3DBuffer*;
using LPDIRECTSOUND3DLISTENER = IDirectSound3DListener*;

HRESULT fl_port_create_directsound(const GUID& iid, void** out);
