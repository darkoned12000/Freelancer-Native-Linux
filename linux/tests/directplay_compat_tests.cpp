#include "linux_port/directplay8_stubs.h"

#include <cwchar>
#include <iostream>

int main() {
  void* raw = nullptr;
  if (fl_port_create_directplay(CLSID_DirectPlay8Address, IID_IDirectPlay8Address, &raw) < 0 || !raw) return 1;
  auto* address = static_cast<IDirectPlay8Address*>(raw);
  WCHAR url[] = L"x-directplay:/hostname=192.0.2.42";
  if (address->BuildFromURLW(url) < 0) return 2;
  WCHAR host[64]{};
  DWORD size = sizeof(host), type = DPNA_DATATYPE_STRING;
  if (address->GetComponentByName(L"hostname", host, &size, &type) < 0 ||
      std::wcscmp(host, L"192.0.2.42") != 0) {
    std::cerr << "DirectPlay URL hostname component should round-trip\n";
    return 3;
  }
  address->Release();

  raw = nullptr;
  if (fl_port_create_directplay(CLSID_DirectPlay8Client, IID_IDirectPlay8Client, &raw) < 0 || !raw) return 4;
  auto* client = static_cast<IDirectPlay8Client*>(raw);
  if (client->Connect(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 0, nullptr, nullptr, 0) >= 0) {
    std::cerr << "unavailable DirectPlay transport must fail closed, not report connected\n";
    return 5;
  }
  client->Release();

  raw = nullptr;
  if (fl_port_create_directplay(CLSID_DirectPlay8Server, IID_IDirectPlay8Server, &raw) < 0 || !raw) return 6;
  auto* server = static_cast<IDirectPlay8Server*>(raw);
  if (server->Host(nullptr, nullptr, 0, nullptr, nullptr, nullptr, 0) >= 0) {
    std::cerr << "unavailable DirectPlay server transport must fail closed\n";
    return 7;
  }
  server->Release();
  return 0;
}
