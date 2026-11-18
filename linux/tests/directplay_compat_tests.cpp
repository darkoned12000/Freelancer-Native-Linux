#include "linux_port/directplay8_stubs.h"

#include <cstring>
#include <cwchar>
#include <iostream>
#include <string>

namespace {
struct ClientState {
  bool connect_complete{false};
  HRESULT connect_result{DPNERR_GENERIC};
  bool received{false};
  unsigned char payload{0};
};
struct ServerState {
  bool indicate{false};
  bool player_created{false};
  bool received{false};
  unsigned char payload{0};
  void* player_context{nullptr};
  DPNID player_id{0};
};

long WINAPI client_cb(void* ctx, unsigned long id, void* data) {
  auto* s = static_cast<ClientState*>(ctx);
  if (id == DPN_MSGID_CONNECT_COMPLETE) {
    auto* m = static_cast<DPNMSG_CONNECT_COMPLETE*>(data);
    s->connect_complete = true;
    s->connect_result = m->hResultCode;
  } else if (id == DPN_MSGID_RECEIVE) {
    auto* m = static_cast<DPNMSG_RECEIVE*>(data);
    if (m && m->pReceiveData && m->dwReceiveDataSize >= 1) {
      s->received = true;
      s->payload = m->pReceiveData[0];
    }
  }
  return 0;
}

long WINAPI server_cb(void* ctx, unsigned long id, void* data) {
  auto* s = static_cast<ServerState*>(ctx);
  if (id == DPN_MSGID_INDICATE_CONNECT) {
    auto* m = static_cast<DPNMSG_INDICATE_CONNECT*>(data);
    s->indicate = true;
    s->player_context = reinterpret_cast<void*>(0x1234);
    m->pvPlayerContext = s->player_context;
  } else if (id == DPN_MSGID_CREATE_PLAYER) {
    auto* m = static_cast<DPNMSG_CREATE_PLAYER*>(data);
    s->player_created = true;
    s->player_id = m->dpnidPlayer;
  } else if (id == DPN_MSGID_RECEIVE) {
    auto* m = static_cast<DPNMSG_RECEIVE*>(data);
    if (m && m->pReceiveData && m->dwReceiveDataSize >= 1) {
      s->received = true;
      s->payload = m->pReceiveData[0];
    }
  }
  return 0;
}
}  // namespace

int main() {
  void* raw = nullptr;
  if (fl_port_create_directplay(CLSID_DirectPlay8Address, IID_IDirectPlay8Address, &raw) < 0 || !raw) return 1;
  auto* address = static_cast<IDirectPlay8Address*>(raw);
  WCHAR url[] = L"x-directplay:/hostname=192.0.2.42&port=2302";
  if (address->BuildFromURLW(url) < 0) return 2;
  WCHAR host[64]{};
  DWORD size = sizeof(host), type = DPNA_DATATYPE_STRING;
  if (address->GetComponentByName(L"hostname", host, &size, &type) < 0 ||
      std::wcscmp(host, L"192.0.2.42") != 0) {
    std::cerr << "DirectPlay URL hostname component should round-trip\n";
    return 3;
  }
  DWORD port = 0; size = sizeof(port); type = DPNA_DATATYPE_DWORD;
  if (address->GetComponentByName(L"port", &port, &size, &type) < 0 || port != 2302) {
    std::cerr << "DirectPlay URL port component should round-trip\n";
    return 31;
  }
  address->Release();

  // Loopback transport: host server, connect client, exchange messages.
  ServerState ss{};
  ClientState cs{};
  raw = nullptr;
  if (fl_port_create_directplay(CLSID_DirectPlay8Server, IID_IDirectPlay8Server, &raw) < 0 || !raw) return 4;
  auto* server = static_cast<IDirectPlay8Server*>(raw);
  if (server->Initialize(&ss, server_cb, 0) < 0) return 41;

  DPN_APPLICATION_DESC app{};
  app.dwSize = sizeof(app);
  app.guidApplication = {0x11111111, 0x2222, 0x3333, {4,5,6,7,8,9,10,11}};
  WCHAR session[] = L"Loopback";
  app.pwszSessionName = session;
  if (server->Host(&app, nullptr, 0, nullptr, nullptr, nullptr, 0) < 0) {
    std::cerr << "loopback DirectPlay host should succeed for local development\n";
    return 5;
  }

  raw = nullptr;
  if (fl_port_create_directplay(CLSID_DirectPlay8Client, IID_IDirectPlay8Client, &raw) < 0 || !raw) return 6;
  auto* client = static_cast<IDirectPlay8Client*>(raw);
  if (client->Initialize(&cs, client_cb, 0) < 0) return 61;

  // No matching server for different GUID should report failed connect.
  DPN_APPLICATION_DESC missing = app;
  missing.guidApplication = {0xAAAAAAAA, 0, 0, {0,0,0,0,0,0,0,0}};
  ClientState missing_cs{};
  client->Initialize(&missing_cs, client_cb, 0);
  client->Connect(&missing, nullptr, nullptr, nullptr, nullptr, nullptr, 0, nullptr, nullptr, 0);
  if (!missing_cs.connect_complete || missing_cs.connect_result == 0) {
    std::cerr << "connect without matching hosted session must fail\n";
    return 7;
  }

  client->Initialize(&cs, client_cb, 0);
  if (client->Connect(&app, nullptr, nullptr, nullptr, nullptr, nullptr, 0, nullptr, nullptr, 0) == DPNERR_GENERIC &&
      !cs.connect_complete) {
    std::cerr << "loopback connect to hosted session should complete\n";
    return 8;
  }
  if (!cs.connect_complete || cs.connect_result != 0 || !ss.indicate || !ss.player_created) return 9;

  unsigned char ping = 0x42;
  DPN_BUFFER_DESC buf{1, &ping};
  if (client->Send(&buf, 1, 0, nullptr, nullptr, 0) < 0 || !ss.received || ss.payload != 0x42) return 10;

  unsigned char pong = 0x24;
  DPN_BUFFER_DESC sbuf{1, &pong};
  const DPNID target = ss.player_id ? ss.player_id : 2;
  if (server->SendTo(target, &sbuf, 1, 0, nullptr, nullptr, 0) < 0 || !cs.received || cs.payload != 0x24) return 11;

  client->Close(0);
  server->Close(0);
  client->Release();
  server->Release();
  return 0;
}
