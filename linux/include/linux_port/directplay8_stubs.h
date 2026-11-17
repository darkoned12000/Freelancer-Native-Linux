#pragma once
// Minimal DirectPlay8 stubs for Linux bring-up (single-player, no networking).
// Provides enough types/interfaces for CDP*.cpp to compile.

#include "linux_port/win32_compat.h"

#ifndef CLSCTX_INPROC_SERVER
#define CLSCTX_INPROC_SERVER 1
#endif

// GUID stubs for DirectPlay8
struct GUID_DirectPlay8Client { static constexpr unsigned long Data1 = 0; };
#define CLSID_DirectPlay8Client GUID_DirectPlay8Client{}
#define IID_IDirectPlay8Client GUID_DirectPlay8Client{}

// DPN types
struct DPN_CAPS { unsigned long dwSize; };
struct DPN_APPLICATION_DESC { unsigned long dwSize; };
struct DPN_BUFFER_DESC { void* pBuffer; unsigned long dwBufferSize; };
struct DPN_MSGID { unsigned long dwType; };
struct DPN_CONNECTION_INFO { unsigned long dwSize; };

// IDirectPlay8Address stub
struct IDirectPlay8Address {
  virtual unsigned long AddRef() { return 1; }
  virtual unsigned long Release() { return 1; }
  virtual HRESULT Initialize() { return 0; }
};

// IDirectPlay8Client stub
struct IDirectPlay8Client {
  virtual unsigned long AddRef() { return 1; }
  virtual unsigned long Release() { return 1; }
  virtual HRESULT Initialize(void*, void*, unsigned long) { return 0; }
  virtual HRESULT SetCaps(void*, unsigned long) { return 0; }
  virtual HRESULT CancelAsyncOperation(unsigned long, unsigned long) { return 0; }
  virtual HRESULT Close(unsigned long) { return 0; }
  virtual HRESULT Connect(void*, void*, void*, void*, void*, void*, unsigned long, void*, void*, unsigned long) { return 0; }
  virtual HRESULT Send(void*, unsigned long, unsigned long, void*, void*, unsigned long) { return 0; }
  virtual HRESULT EnumHosts(void*, void*, void*, void*, unsigned long, unsigned long, unsigned long, void*, void*, unsigned long) { return 0; }
  virtual HRESULT GetSendQueueInfo(unsigned long*, void*, unsigned long) { return 0; }
  virtual HRESULT GetConnectionInfo(void*, unsigned long) { return 0; }
  virtual HRESULT ReturnBuffer(void*, unsigned long) { return 0; }
};

// IDirectPlay8Server stub
struct IDirectPlay8Server {
  virtual unsigned long AddRef() { return 1; }
  virtual unsigned long Release() { return 1; }
  virtual HRESULT Initialize(void*, void*, unsigned long) { return 0; }
  virtual HRESULT Close(unsigned long) { return 0; }
};

// IDirectPlay8Peer stub
struct IDirectPlay8Peer {
  virtual unsigned long AddRef() { return 1; }
  virtual unsigned long Release() { return 1; }
  virtual HRESULT Initialize(void*, void*, unsigned long) { return 0; }
  virtual HRESULT Close(unsigned long) { return 0; }
};

// CoCreateInstance override for DirectPlay
inline HRESULT CoCreateInstance_DirectPlay8Client(const GUID&, void*, DWORD, const GUID&, void** ppv) {
  if (ppv) *ppv = new IDirectPlay8Client();
  return 0;
}
