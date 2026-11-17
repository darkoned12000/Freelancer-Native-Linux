#pragma once
// Minimal DirectInput8 shim for Linux bring-up (compile support only).
#include "linux_port/win32_compat.h"

#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0800
#endif

typedef LONG HRESULT;
#ifndef S_OK
#define S_OK 0
#endif
#ifndef DI_OK
#define DI_OK S_OK
#endif
#ifndef S_FALSE
#define S_FALSE 1
#endif

#define DIENUM_CONTINUE 0
#define DIENUM_STOP 1

#define DIEDFL_ATTACHEDONLY 0x00000001
#define DIDFT_ALL 0x00000000

#define DISCL_FOREGROUND 0x00000001
#define DISCL_NONEXCLUSIVE 0x00000002
#define DISCL_BACKGROUND 0x00000004
#define DISCL_EXCLUSIVE 0x00000008

extern "C" const GUID DIPROP_BUFFERSIZE;

#define DIMOFS_X 0
#define DIMOFS_Y 4
#define DIMOFS_Z 8
#define DIMOFS_BUTTON0 12
#define DIMOFS_BUTTON1 13
#define DIMOFS_BUTTON2 14
#define DIMOFS_BUTTON3 15
#define DIMOFS_BUTTON4 16
#define DIMOFS_BUTTON5 17
#define DIMOFS_BUTTON6 18
#define DIMOFS_BUTTON7 19

#define DIERR_INPUTLOST 0x8007001E

#ifndef CURSOR_SHOWING
#define CURSOR_SHOWING 0x00000001
#endif

#define DIPH_DEVICE 0
#define DIPH_BYOFFSET 1

typedef struct _DIPROPHEADER {
  DWORD dwSize;
  DWORD dwHeaderSize;
  DWORD dwObj;
  DWORD dwHow;
} DIPROPHEADER;
typedef struct _DIPROPDWORD {
  DIPROPHEADER diph;
  DWORD dwData;
} DIPROPDWORD;

typedef struct _DIDEVICEOBJECTDATA {
  DWORD dwOfs;
  DWORD dwData;
  DWORD dwTimeStamp;
  DWORD dwSequence;
  DWORD dwPad; // port: retail stride 0x14
} DIDEVICEOBJECTDATA, *LPDIDEVICEOBJECTDATA;

typedef struct _DIMOUSESTATE2 {
  LONG lX;
  LONG lY;
  LONG lZ;
  BYTE rgbButtons[8];
} DIMOUSESTATE2, *LPDIMOUSESTATE2;

typedef struct _DIOBJECTDATAFORMAT {
  const GUID* pguid;
  DWORD dwOfs;
  DWORD dwType;
  DWORD dwFlags;
} DIOBJECTDATAFORMAT, *LPDIOBJECTDATAFORMAT;

typedef struct _DIDATAFORMAT {
  DWORD dwSize;
  DWORD dwObjSize;
  DWORD dwFlags;
  DWORD dwNumObjs;
  LPDIOBJECTDATAFORMAT rgodf;
} DIDATAFORMAT, *LPDIDATAFORMAT;

extern DIDATAFORMAT c_dfDIMouse2;

typedef struct _DIDEVCAPS {
  DWORD dwSize;
  DWORD dwFlags;
  DWORD dwDevType;
  DWORD dwAxes;
  DWORD dwButtons;
  DWORD dwPOVs;
  DWORD dwFFSamplePeriod;
  DWORD dwFFMinTimeResolution;
  DWORD dwFirmwareRevision;
  DWORD dwHardwareRevision;
  DWORD dwFFDriverVersion;
} DIDEVCAPS, *LPDIDEVCAPS;

// GUIDs
extern "C" const GUID IID_IDirectInput8A;
extern "C" const GUID GUID_SysMouse;

// Forward
struct IDirectInputDevice8A;
struct IDirectInput8A;

typedef struct IDirectInput8A IDirectInput8A;
typedef struct IDirectInputDevice8A IDirectInputDevice8A;

typedef IDirectInput8A* LPDIRECTINPUT8A;
typedef IDirectInputDevice8A* LPDIRECTINPUTDEVICE8A;

// Vtable-style minimal C++ interfaces (methods used by input_manager)
struct IDirectInputDevice8A {
  virtual HRESULT QueryInterface(const GUID&, void**) { return 0; }
  virtual ULONG AddRef() { return 1; }
  virtual ULONG Release() { return 1; }
  virtual HRESULT GetCapabilities(DIDEVCAPS*) { return 0; }
  virtual HRESULT EnumObjects(void*, void*, DWORD) { return 0; }
  virtual HRESULT GetProperty(const GUID*, DIPROPHEADER*) { return 0; }
  virtual HRESULT SetProperty(const GUID&, const DIPROPHEADER*) { return 0; }
  virtual HRESULT Acquire() { return 0; }
  virtual HRESULT Unacquire() { return 0; }
  virtual HRESULT GetDeviceState(DWORD, void*) { return 0; }
  virtual HRESULT GetDeviceData(DWORD, DIDEVICEOBJECTDATA*, DWORD*, DWORD) { return 0; }
  virtual HRESULT SetDataFormat(const DIDATAFORMAT*) { return 0; }
  virtual HRESULT SetEventNotification(void*) { return 0; }
  virtual HRESULT SetCooperativeLevel(void*, DWORD) { return 0; }
  virtual HRESULT Poll() { return 0; }
  virtual HRESULT SendDeviceData(DWORD, const DIDEVICEOBJECTDATA*, DWORD*, DWORD) { return 0; }
  virtual HRESULT EnumCreatedEffectObjects(void*, void*, DWORD) { return 0; }
  virtual HRESULT Escape(void*) { return 0; }
  virtual HRESULT GetObjectInfo(void*, DWORD, DWORD) { return 0; }
  virtual HRESULT GetDeviceInfo(void*) { return 0; }
  virtual HRESULT RunControlPanel(void*, DWORD) { return 0; }
  virtual HRESULT Initialize(void*, DWORD, DWORD) { return 0; }
};

struct IDirectInput8A {
  virtual HRESULT QueryInterface(const GUID&, void**) { return 0; }
  virtual ULONG AddRef() { return 1; }
  virtual ULONG Release() { return 1; }
  virtual HRESULT CreateDevice(const GUID&, IDirectInputDevice8A**, void*) { return 0; }
  virtual HRESULT EnumDevices(DWORD, void*, void*, DWORD) { return 0; }
  virtual HRESULT GetDeviceStatus(const GUID&) { return 0; }
  virtual HRESULT RunControlPanel(void*, DWORD) { return 0; }
  virtual HRESULT Initialize(void*, DWORD) { return 0; }
};

extern "C" HRESULT DirectInput8Create(void* hinst, DWORD dwVersion, const GUID& riidltf, void** ppvOut, void* punkOuter);