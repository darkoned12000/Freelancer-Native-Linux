#pragma once
// Linux DirectPlay8 compatibility surface. The client transport implementation is supplied by
// directplay8_compat.cpp; these declarations preserve DirectPlay's callback/message contracts.
#include "linux_port/win32_compat.h"

#ifndef CLSCTX_INPROC_SERVER
#define CLSCTX_INPROC_SERVER 1
#endif
#ifndef CLSCTX_INPROC_HANDLER
#define CLSCTX_INPROC_HANDLER 2
#endif
#ifndef CLSCTX_LOCAL_SERVER
#define CLSCTX_LOCAL_SERVER 4
#endif

using DPNHANDLE = DWORD;
#define DPNHANDLE_DEFINED
using DPNID = DWORD;
using DPN_MSGID = DWORD;
constexpr HRESULT DPNSUCCESS_PENDING = 0x00000103;
constexpr HRESULT DPNERR_GENERIC = static_cast<HRESULT>(0x88780001u);
constexpr DWORD DPNA_DATATYPE_STRING = 1;
constexpr DWORD DPNA_DATATYPE_DWORD = 2;
constexpr DWORD DPNCANCEL_ENUM = 2;
constexpr DWORD DPNCANCEL_ALL_OPERATIONS = 0x8000;
constexpr DWORD DPN_MSGID_CONNECT_COMPLETE = 0xFFFF0001u;
constexpr DWORD DPN_MSGID_ASYNC_OP_COMPLETE = 0xFFFF0002u;
constexpr DWORD DPN_MSGID_ENUM_HOSTS_RESPONSE = 0xFFFF0003u;
constexpr DWORD DPN_MSGID_RECEIVE = 0xFFFF0004u;
constexpr DWORD DPN_MSGID_TERMINATE_SESSION = 0xFFFF0005u;
constexpr DWORD DPN_MSGID_CREATE_PLAYER = 0xFFFF0006u;
constexpr DWORD DPN_MSGID_DESTROY_PLAYER = 0xFFFF0007u;
constexpr DWORD DPN_MSGID_ENUM_HOSTS_QUERY = 0xFFFF0008u;
constexpr DWORD DPN_MSGID_INDICATE_CONNECT = 0xFFFF0009u;
constexpr DWORD DPN_MSGID_INDICATED_CONNECT_ABORTED = 0xFFFF000Au;
constexpr DWORD DPN_MSGID_SEND_COMPLETE = 0xFFFF000Bu;

struct IDirectPlay8Address;
struct DPN_APPLICATION_DESC {
  DWORD dwSize{};
  DWORD dwFlags{};
  GUID guidInstance{};
  GUID guidApplication{};
  DWORD dwMaxPlayers{};
  DWORD dwCurrentPlayers{};
  WCHAR* pwszSessionName{};
  WCHAR* pwszPassword{};
  void* pvApplicationReservedData{};
  DWORD dwApplicationReservedDataSize{};
  void* pvReservedData{};
  DWORD dwReservedDataSize{};
  IDirectPlay8Address* pHostAddress{};
};
struct DPN_CAPS { DWORD dwSize{}; DWORD dwFlags{}; DWORD dwConnectTimeout{}; DWORD dwConnectRetries{}; DWORD dwMaxSendQueueSize{}; };
struct DPN_BUFFER_DESC { DWORD dwBufferSize{}; BYTE* pBuffer{}; };
struct DPN_CONNECTION_INFO { DWORD dwSize{}; DWORD dwRoundTripLatencyMS{}; DWORD dwThroughputBPS{}; DWORD dwPeakThroughputBPS{}; };
typedef DPN_CONNECTION_INFO _DPN_CONNECTION_INFO;
struct DPNMSG_ASYNC_OP_COMPLETE { DWORD dwSize{}; DPNHANDLE hAsyncOp{}; HRESULT hResultCode{}; void* pvUserContext{}; };
struct DPNMSG_CONNECT_COMPLETE { DWORD dwSize{}; DPNHANDLE hAsyncOp{}; HRESULT hResultCode{}; DPNID dpnidLocal{}; void* pvPlayerContext{}; void* pvUserContext{}; };
struct DPNMSG_ENUM_HOSTS_RESPONSE {
  DWORD dwSize{};
  const DPN_APPLICATION_DESC* pApplicationDescription{};
  IDirectPlay8Address* pAddressSender{};
  IDirectPlay8Address* pAddressDevice{};
  void* pvResponseData{};
  DWORD dwResponseDataSize{};
  DWORD dwRoundTripLatencyMS{};
  void* pvUserContext{};
};
typedef DPNMSG_ENUM_HOSTS_RESPONSE _DPNMSG_ENUM_HOSTS_RESPONSE;
struct DPNMSG_RECEIVE { DWORD dwSize{}; DPNID dpnidSender{}; DPNID dpnidLocal{}; BYTE* pReceiveData{}; DWORD dwReceiveDataSize{}; DPNHANDLE hBufferHandle{}; void* pvPlayerContext{}; };
struct DPNMSG_TERMINATE_SESSION { DWORD dwSize{}; HRESULT hResultCode{}; void* pvTerminateData{}; DWORD dwTerminateDataSize{}; void* pvUserContext{}; };
struct DPNMSG_CREATE_PLAYER { DWORD dwSize{}; DPNID dpnidPlayer{}; void* pvPlayerContext{}; void* pvPlayerInfo{}; IDirectPlay8Address* pAddress{}; };
struct DPNMSG_DESTROY_PLAYER { DWORD dwSize{}; DPNID dpnidPlayer{}; void* pvPlayerContext{}; void* pvPlayerInfo{}; HRESULT hReason{}; };
struct DPNMSG_ENUM_HOSTS_QUERY { DWORD dwSize{}; DPNID dpnidSender{}; IDirectPlay8Address* pAddressSender{}; IDirectPlay8Address* pAddressDevice{}; void* pvReceivedData{}; DWORD dwReceivedDataSize{}; void* pvResponseData{}; DWORD dwResponseDataSize{}; };
struct DPNMSG_INDICATE_CONNECT { DWORD dwSize{}; const DPN_APPLICATION_DESC* pApplicationDescription{}; IDirectPlay8Address* pAddressPlayer{}; IDirectPlay8Address* pAddressDevice{}; BYTE* pvUserConnectData{}; DWORD dwUserConnectDataSize{}; void* pvPlayerContext{}; void* pvReplyData{}; DWORD dwReplyDataSize{}; HRESULT dwRejectReason{}; };
struct DPNMSG_INDICATED_CONNECT_ABORTED { DWORD dwSize{}; void* pvPlayerContext{}; HRESULT hResultCode{}; };
struct DPNMSG_SEND_COMPLETE { DWORD dwSize{}; void* pvUserContext{}; DPNHANDLE hAsyncOp{}; HRESULT hResultCode{}; DWORD dwSendTime{}; };
using PFNDPNMESSAGEHANDLER = long (WINAPI*)(void*, unsigned long, void*);

struct IDirectPlay8Address {
  virtual ULONG AddRef() = 0;
  virtual ULONG Release() = 0;
  virtual HRESULT BuildFromURLW(WCHAR*) = 0;
  virtual HRESULT SetSP(const GUID*) = 0;
  virtual HRESULT AddComponent(const WCHAR*, const void*, DWORD, DWORD) = 0;
  virtual HRESULT GetComponentByName(const WCHAR*, void*, DWORD*, DWORD*) = 0;
  virtual HRESULT GetURLA(char*, DWORD*) = 0;
};

struct IDirectPlay8Client {
  virtual ULONG AddRef() = 0;
  virtual ULONG Release() = 0;
  virtual HRESULT Initialize(void*, PFNDPNMESSAGEHANDLER, DWORD) = 0;
  virtual HRESULT SetCaps(DPN_CAPS*, DWORD) = 0;
  virtual HRESULT CancelAsyncOperation(DPNHANDLE, DWORD) = 0;
  virtual HRESULT Close(DWORD) = 0;
  virtual HRESULT Connect(const DPN_APPLICATION_DESC*, IDirectPlay8Address*, IDirectPlay8Address*,
                          void*, void*, void*, DWORD, void*, DPNHANDLE*, DWORD) = 0;
  virtual HRESULT Send(const DPN_BUFFER_DESC*, DWORD, DWORD, void*, DPNHANDLE*, DWORD) = 0;
  virtual HRESULT EnumHosts(const DPN_APPLICATION_DESC*, IDirectPlay8Address*, IDirectPlay8Address*,
                            void*, DWORD, DWORD, DWORD, DWORD, void*, DPNHANDLE*, DWORD) = 0;
  virtual HRESULT GetSendQueueInfo(DWORD*, DWORD*, DWORD) = 0;
  virtual HRESULT GetConnectionInfo(DPN_CONNECTION_INFO*, DWORD) = 0;
  virtual HRESULT ReturnBuffer(DPNHANDLE, DWORD) = 0;
};
struct IDirectPlay8Server;
struct IDirectPlay8Peer;
struct IDirectPlay8Server {
  virtual ULONG AddRef() = 0;
  virtual ULONG Release() = 0;
  virtual HRESULT Initialize(void*, PFNDPNMESSAGEHANDLER, DWORD) = 0;
  virtual HRESULT SetApplicationDesc(const DPN_APPLICATION_DESC*, DWORD) = 0;
  virtual HRESULT SetCaps(DPN_CAPS*, DWORD) = 0;
  virtual HRESULT Close(DWORD) = 0;
  virtual HRESULT GetLocalHostAddresses(IDirectPlay8Address**, DWORD*, DWORD) = 0;
  virtual HRESULT Host(const DPN_APPLICATION_DESC*, IDirectPlay8Address**, DWORD, void*, void*, void*, DWORD) = 0;
  virtual HRESULT SendTo(DPNID, const DPN_BUFFER_DESC*, DWORD, DWORD, void*, DPNHANDLE*, DWORD) = 0;
  virtual HRESULT DestroyClient(DPNID, void*, DWORD, DWORD) = 0;
  virtual HRESULT GetConnectionInfo(DPNID, DPN_CONNECTION_INFO*, DWORD) = 0;
  virtual HRESULT GetSendQueueInfo(DPNID, DWORD*, DWORD*, DWORD) = 0;
  virtual HRESULT ReturnBuffer(DPNHANDLE, DWORD) = 0;
};

inline const GUID FL_CLSID_DirectPlay8Client{0x743B5D60,0x1A5F,0x11D3,{0x8A,0xE4,0x00,0x60,0x97,0xB0,0x14,0x11}};
inline const GUID FL_IID_IDirectPlay8Client{0x5102DACE,0x241B,0x11D3,{0xAE,0xA7,0x00,0x60,0x97,0xB0,0x14,0x11}};
inline const GUID FL_CLSID_DirectPlay8Address{0x934A9523,0xA3CA,0x4BC5,{0xAD,0xA0,0xD6,0xD5,0x8B,0xBD,0xB8,0xD6}};
inline const GUID FL_IID_IDirectPlay8Address{0x83783300,0x4063,0x4C8A,{0x9D,0xB3,0x82,0x83,0x0A,0x7F,0xEB,0x31}};
inline const GUID FL_CLSID_DirectPlay8Server{0xDA825E1B,0x6830,0x43D7,{0x83,0x5D,0x0B,0x5A,0xE4,0x1D,0xB2,0xC5}};
inline const GUID FL_IID_IDirectPlay8Server{0x0D22EEA0,0x4A46,0x4A0D,{0x89,0x8C,0xC7,0x0A,0xF8,0x6A,0x2E,0x50}};
inline const GUID CLSID_DP8SP_TCPIP{0xEBFE7BA0,0x628D,0x11D2,{0xAE,0x0F,0x00,0x60,0x97,0xB0,0x14,0x11}};
#define CLSID_DirectPlay8Client FL_CLSID_DirectPlay8Client
#define IID_IDirectPlay8Client FL_IID_IDirectPlay8Client
#define CLSID_DirectPlay8Address FL_CLSID_DirectPlay8Address
#define IID_IDirectPlay8Address FL_IID_IDirectPlay8Address
#define CLSID_DirectPlay8Server FL_CLSID_DirectPlay8Server
#define IID_IDirectPlay8Server FL_IID_IDirectPlay8Server

HRESULT fl_port_create_directplay(const GUID& clsid, const GUID& iid, void** out);
