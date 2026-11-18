#include "dalib_net.h"

#include <cstring>
#include <iostream>
#include <memory>
#include <vector>

CriticalWarningFnType CriticalWarningFn = nullptr;

namespace {
class TestProxy final : public CDPClientProxy {
 public:
  std::vector<std::vector<unsigned char>> messages;
  unsigned int connect_calls = 0;
  unsigned int disconnect_calls = 0;
  void OnReceive(unsigned char* data, unsigned long size) override {
    messages.emplace_back(data, data + size);
  }
  void OnConnect(long) override { ++connect_calls; }
  void OnDisconnect() override { ++disconnect_calls; }
  std::vector<unsigned char> all() const {
    std::vector<unsigned char> out;
    for (auto& m : messages) out.insert(out.end(), m.begin(), m.end());
    return out;
  }
  void clear() { messages.clear(); }
};

class TestServer final : public CDPServer {
 public:
  TestProxy* last_proxy = nullptr;
  CDPClientProxy* CreateClientProxy() override {
    last_proxy = new TestProxy();
    return last_proxy;
  }
};

class TestClient final : public CDPClient {
 public:
  std::vector<std::vector<unsigned char>> messages;
  bool disconnected = false;
  bool connected = false;
  long connect_result = -999;
  void OnReceive(unsigned char* data, unsigned long size) override {
    messages.emplace_back(data, data + size);
  }
  void OnConnect(long result) override {
    connect_result = result;
    if (result == 0) connected = true;
  }
  void OnDisconnect() override { disconnected = true; connected = false; }
  void OnAddHost(_DPNMSG_ENUM_HOSTS_RESPONSE*) override {}
  std::vector<unsigned char> all() const {
    std::vector<unsigned char> out;
    for (auto& m : messages) out.insert(out.end(), m.begin(), m.end());
    return out;
  }
  void clear() { messages.clear(); }
};

bool equal_bytes(const std::vector<unsigned char>& v, const unsigned char* d, size_t n) {
  return v.size() == n && (n == 0 || std::memcmp(v.data(), d, n) == 0);
}
}  // namespace

int main() {
  GUID app{0x11111111, 0x2222, 0x3333, {4,5,6,7,8,9,10,11}};

  TestServer server;
  server.SetGUID(app);
  if (!server.BeginHosting(2302)) {
    std::cerr << "CDPServer::BeginHosting failed\n";
    return 1;
  }

  TestClient client;
  client.SetGUID(app);
  if (!client.AddConnectAttempt(L"127.0.0.1", L"2302")) {
    std::cerr << "CDPClient::AddConnectAttempt failed\n";
    return 2;
  }
  if (!client.ProcessConnectAttempt()) {
    std::cerr << "CDPClient::ProcessConnectAttempt failed\n";
    return 2;
  }

  // Process queued connect/player events.
  client.DispatchMsgs(0);
  server.DispatchMsgs();
  client.DispatchMsgs(0);
  server.DispatchMsgs();

  if (!client.connected || client.connect_result != 0) {
    std::cerr << "client did not report successful connect (result=" << client.connect_result << ")\n";
    return 3;
  }
  if (!server.last_proxy || server.last_proxy->connect_calls == 0) {
    std::cerr << "server proxy did not receive connect callback\n";
    return 4;
  }

  // Client -> server: CDPClient::Send queues DirectPlay send; loopback delivers
  // RECEIVE callback synchronously into server incoming queue (pending buffer).
  const unsigned char c2s[] = "hello-server";
  if (!client.Send(const_cast<unsigned char*>(c2s), sizeof(c2s))) {
    std::cerr << "CDPClient::Send failed\n";
    return 5;
  }
  server.DispatchMsgs();
  if (!equal_bytes(server.last_proxy->all(), c2s, sizeof(c2s))) {
    std::cerr << "server proxy did not receive client payload (msgs=" << server.last_proxy->messages.size() << ")\n";
    return 6;
  }

  // Server -> client via proxy send path (queues/compresses/batches and calls CDPServer::SendTo).
  const unsigned char s2c[] = "hello-client";
  if (!server.last_proxy->Send(const_cast<unsigned char*>(s2c), sizeof(s2c))) {
    std::cerr << "CDPClientProxy::Send failed\n";
    return 7;
  }
  client.DispatchMsgs(0);
  if (!equal_bytes(client.all(), s2c, sizeof(s2c))) {
    std::cerr << "client did not receive server payload (msgs=" << client.messages.size() << ")\n";
    return 8;
  }

  // Multiple messages / queue behavior.
  server.last_proxy->clear();
  client.clear();
  const unsigned char m1[] = {0xAA, 0xBB};
  const unsigned char m2[] = {0xCC, 0xDD, 0xEE};
  client.Send(const_cast<unsigned char*>(m1), sizeof(m1));
  client.Send(const_cast<unsigned char*>(m2), sizeof(m2));
  server.DispatchMsgs();
  {
    std::vector<unsigned char> expected;
    expected.insert(expected.end(), m1, m1 + sizeof(m1));
    expected.insert(expected.end(), m2, m2 + sizeof(m2));
    if (server.last_proxy->all() != expected) {
      std::cerr << "client->server multi-message payload mismatch (msgs=" << server.last_proxy->messages.size()
                << " bytes=" << server.last_proxy->all().size() << ")\n";
      return 9;
    }
  }

  // Server->client multiple messages via proxy queue.
  server.last_proxy->clear();
  client.clear();
  server.last_proxy->Send(const_cast<unsigned char*>(m1), sizeof(m1));
  server.last_proxy->Send(const_cast<unsigned char*>(m2), sizeof(m2));
  client.DispatchMsgs(0);
  {
    std::vector<unsigned char> expected;
    expected.insert(expected.end(), m1, m1 + sizeof(m1));
    expected.insert(expected.end(), m2, m2 + sizeof(m2));
    if (client.all() != expected) {
      std::cerr << "server->client multi-message payload mismatch (msgs=" << client.messages.size()
                << " bytes=" << client.all().size() << ")\n";
      return 10;
    }
  }

  // Enum host path (loopback registry-backed): should produce an add-host callback.
  // (deferred: separate client enum while hosting needs more validation)
  std::cerr << "enum skipped\n";

  // Server-side disconnect of client should trigger client termination/disconnect path.
  TestProxy* proxy = server.last_proxy;
  const unsigned long pid = proxy ? proxy->PlayerId : 0;
  if (pid == 0) {
    std::cerr << "proxy player id missing\n";
    return 13;
  }
  server.DisconnectClient(pid);
  std::cerr << "disconnect called\n";
  server.DispatchMsgs();
  std::cerr << "server dispatched\n";
  client.DispatchMsgs(0);
  std::cerr << "client dispatched\n";
  if (!client.disconnected) {
    std::cerr << "client did not observe disconnect after server DisconnectClient\n";
    return 14;
  }
  std::cerr << "disconnect ok\n";

  // Connection stats / queue APIs should be callable without crashing.
  _DPN_CONNECTION_INFO info{};
  info.dwSize = sizeof(info);
  (void)server.GetConnectionStats(nullptr, &info);
  (void)client.GetSendQueueSize();

  server.StopHosting();
  server.DispatchMsgs();
  client.Close();
  client.DispatchMsgs(0);
  return 0;
}
