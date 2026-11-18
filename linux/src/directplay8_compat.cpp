#include "linux_port/directplay8_stubs.h"

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <memory>
#include <mutex>
#include <new>
#include <string>
#include <unordered_map>
#include <vector>

namespace {
constexpr HRESULT kNoInterface = static_cast<HRESULT>(0x80004002u);
constexpr HRESULT kGeneric = DPNERR_GENERIC;

inline bool guid_eq(const GUID& a, const GUID& b) {
  return a.Data1 == b.Data1 && a.Data2 == b.Data2 && a.Data3 == b.Data3 &&
         std::memcmp(a.Data4, b.Data4, 8) == 0;
}

class NativeAddress final : public IDirectPlay8Address {
 public:
  ULONG AddRef() override { return ++refs_; }
  ULONG Release() override { const auto n = --refs_; if (!n) delete this; return n; }

  HRESULT BuildFromURLW(WCHAR* url) override {
    if (!url) return kGeneric;
    url_ = url;
    const std::wstring key = L"hostname=";
    const auto begin = url_.find(key);
    if (begin == std::wstring::npos) return kGeneric;
    const auto start = begin + key.size();
    const auto end = url_.find_first_of(L"&/?", start);
    host_ = url_.substr(start, end == std::wstring::npos ? std::wstring::npos : end - start);
    const std::wstring port_key = L"port=";
    const auto pbegin = url_.find(port_key);
    if (pbegin != std::wstring::npos) {
      const auto pstart = pbegin + port_key.size();
      const auto pend = url_.find_first_of(L"&/?", pstart);
      port_ = static_cast<DWORD>(std::wcstoul(url_.substr(pstart, pend - pstart).c_str(), nullptr, 10));
    }
    return host_.empty() ? kGeneric : 0;
  }

  HRESULT SetSP(const GUID* sp) override { if (!sp) return kGeneric; sp_ = *sp; has_sp_ = true; return 0; }

  HRESULT AddComponent(const WCHAR* name, const void* data, DWORD size, DWORD type) override {
    if (!name || !data || size == 0) return kGeneric;
    const std::wstring n = name;
    if (n == L"port" && type == DPNA_DATATYPE_DWORD && size >= sizeof(DWORD)) {
      std::memcpy(&port_, data, sizeof(port_));
    } else if (n == L"hostname" && type == DPNA_DATATYPE_STRING) {
      const auto* w = static_cast<const WCHAR*>(data);
      host_.assign(w, w + wcslen(w));
    }
    return 0;
  }

  HRESULT GetComponentByName(const WCHAR* name, void* data, DWORD* size, DWORD* type) override {
    if (!name || !size) return kGeneric;
    const std::wstring n = name;
    if (n == L"hostname") {
      if (host_.empty()) host_ = L"127.0.0.1";
      const DWORD need = static_cast<DWORD>((host_.size() + 1) * sizeof(WCHAR));
      if (type) *type = DPNA_DATATYPE_STRING;
      const DWORD capacity = *size;
      *size = need;
      if (!data || capacity < need) return kGeneric;
      std::memcpy(data, host_.c_str(), need);
      return 0;
    }
    if (n == L"port") {
      if (type) *type = DPNA_DATATYPE_DWORD;
      const DWORD need = sizeof(port_);
      const DWORD capacity = *size;
      *size = need;
      if (!data || capacity < need) return kGeneric;
      std::memcpy(data, &port_, need);
      return 0;
    }
    return kGeneric;
  }

  HRESULT GetURLA(char* output, DWORD* size) override {
    if (!size) return kGeneric;
    std::string host(host_.begin(), host_.end());
    if (host.empty()) host = "127.0.0.1";
    std::string url = "x-directplay:/hostname=" + host;
    if (port_) url += "&port=" + std::to_string(port_);
    const DWORD needed = static_cast<DWORD>(url.size() + 1);
    const DWORD capacity = *size;
    *size = needed;
    if (!output || capacity < needed) return kGeneric;
    std::memcpy(output, url.c_str(), needed);
    return 0;
  }

  std::wstring host() const { return host_.empty() ? L"127.0.0.1" : host_; }
  DWORD port() const { return port_; }

 private:
  ULONG refs_{1};
  std::wstring url_, host_;
  GUID sp_{};
  bool has_sp_{false};
  DWORD port_{0};
};

// ---- Loopback transport (same-process, Linux bring-up/testing) ----
// Not compatible with stock FLServer/DirectPlay wire protocol. Provides session,
// connect, and in-order delivery semantics for CDPClient/CDPServer integration
// testing and single-host development workflows.

struct LoopbackServer;
struct LoopbackClient {
  DPNID player_id{};
  void* player_context{};
  std::weak_ptr<LoopbackServer> server;
  void* context{};
  PFNDPNMESSAGEHANDLER callback{};
  bool connected{false};
};

struct LoopbackServer : std::enable_shared_from_this<LoopbackServer> {
  void* context{};
  PFNDPNMESSAGEHANDLER callback{};
  DPN_APPLICATION_DESC app{};
  std::wstring session_name;
  GUID app_guid{};
  bool hosting{false};
  DWORD next_player{2};
  std::mutex mutex;
  std::unordered_map<DPNID, std::shared_ptr<LoopbackClient>> clients;
};

class LoopbackRegistry {
 public:
  static LoopbackRegistry& instance() { static LoopbackRegistry r; return r; }

  void register_server(const std::shared_ptr<LoopbackServer>& server) {
    std::lock_guard<std::mutex> lock(mutex_);
    servers_.push_back(server);
  }
  void unregister_server(const std::shared_ptr<LoopbackServer>& server) {
    std::lock_guard<std::mutex> lock(mutex_);
    servers_.erase(std::remove_if(servers_.begin(), servers_.end(),
                                  [&](const std::weak_ptr<LoopbackServer>& w) {
                                    auto s = w.lock();
                                    return !s || s.get() == server.get();
                                  }),
                   servers_.end());
  }
  std::shared_ptr<LoopbackServer> find(const GUID& app_guid) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto it = servers_.begin(); it != servers_.end();) {
      auto s = it->lock();
      if (!s) { it = servers_.erase(it); continue; }
      if (s->hosting && guid_eq(s->app_guid, app_guid)) return s;
      ++it;
    }
    return nullptr;
  }

 private:
  std::mutex mutex_;
  std::vector<std::weak_ptr<LoopbackServer>> servers_;
};

static std::atomic<DPNHANDLE> g_next_buffer_handle{1};
static std::mutex g_buffer_mutex;
static std::unordered_map<DPNHANDLE, std::vector<BYTE>> g_buffers;

static DPNHANDLE retain_buffer(const BYTE* data, DWORD size) {
  const DPNHANDLE handle = g_next_buffer_handle.fetch_add(1, std::memory_order_relaxed);
  std::lock_guard<std::mutex> lock(g_buffer_mutex);
  g_buffers.emplace(handle, std::vector<BYTE>(data, data + size));
  return handle;
}
static void release_buffer(DPNHANDLE handle) {
  if (!handle) return;
  std::lock_guard<std::mutex> lock(g_buffer_mutex);
  g_buffers.erase(handle);
}
static BYTE* buffer_ptr(DPNHANDLE handle) {
  std::lock_guard<std::mutex> lock(g_buffer_mutex);
  auto it = g_buffers.find(handle);
  return it == g_buffers.end() ? nullptr : it->second.data();
}

class NativeDirectPlayClient final : public IDirectPlay8Client {
 public:
  ULONG AddRef() override { return ++refs_; }
  ULONG Release() override {
    const auto n = --refs_;
    if (!n) { Close(0); delete this; }
    return n;
  }

  HRESULT Initialize(void* context, PFNDPNMESSAGEHANDLER handler, DWORD) override {
    context_ = context;
    callback_ = handler;
    return 0;
  }

  HRESULT SetCaps(DPN_CAPS* caps, DWORD) override {
    return (caps && caps->dwSize >= sizeof(DPN_CAPS)) ? 0 : kGeneric;
  }

  HRESULT CancelAsyncOperation(DPNHANDLE, DWORD) override { return 0; }

  HRESULT Close(DWORD) override {
    auto client = client_;
    auto server = client ? client->server.lock() : std::shared_ptr<LoopbackServer>();
    if (client && server) {
      bool removed = false;
      {
        std::lock_guard<std::mutex> lock(server->mutex);
        removed = server->clients.erase(client->player_id) != 0;
      }
      if (removed && server->callback) {
        DPNMSG_DESTROY_PLAYER destroy{};
        destroy.dwSize = sizeof(destroy);
        destroy.dpnidPlayer = client->player_id;
        destroy.pvPlayerContext = client->player_context;
        server->callback(server->context, DPN_MSGID_DESTROY_PLAYER, &destroy);
      }
    }
    if (client) client->connected = false;
    client_.reset();
    if (connected_ && callback_) {
      DPNMSG_TERMINATE_SESSION msg{};
      msg.dwSize = sizeof(msg);
      callback_(context_, DPN_MSGID_TERMINATE_SESSION, &msg);
    }
    connected_ = false;
    return 0;
  }

  HRESULT Connect(const DPN_APPLICATION_DESC* app, IDirectPlay8Address*, IDirectPlay8Address*,
                  void*, void*, void*, DWORD, void*, DPNHANDLE*, DWORD) override {
    if (!callback_ || !app) return kGeneric;
    auto server = LoopbackRegistry::instance().find(app->guidApplication);
    if (!server) {
      DPNMSG_CONNECT_COMPLETE msg{};
      msg.dwSize = sizeof(msg);
      msg.hResultCode = kGeneric;
      callback_(context_, DPN_MSGID_CONNECT_COMPLETE, &msg);
      return DPNSUCCESS_PENDING;
    }

    auto client = std::make_shared<LoopbackClient>();
    client->context = context_;
    client->callback = callback_;
    client->server = server;
    client->connected = true;

    DPNID player_id = 0;
    void* player_context = nullptr;
    {
      std::lock_guard<std::mutex> lock(server->mutex);
      player_id = server->next_player++;
      client->player_id = player_id;
      DPNMSG_INDICATE_CONNECT indicate{};
      indicate.dwSize = sizeof(indicate);
      indicate.pApplicationDescription = &server->app;
      if (server->callback) server->callback(server->context, DPN_MSGID_INDICATE_CONNECT, &indicate);
      player_context = indicate.pvPlayerContext;
      client->player_context = player_context;
      server->clients[player_id] = client;
    }
    client_ = client;
    connected_ = true;

    if (server->callback) {
      DPNMSG_CREATE_PLAYER create{};
      create.dwSize = sizeof(create);
      create.dpnidPlayer = player_id;
      create.pvPlayerContext = player_context;
      server->callback(server->context, DPN_MSGID_CREATE_PLAYER, &create);
    }

    DPNMSG_CONNECT_COMPLETE complete{};
    complete.dwSize = sizeof(complete);
    complete.hResultCode = 0;
    complete.dpnidLocal = player_id;
    callback_(context_, DPN_MSGID_CONNECT_COMPLETE, &complete);
    return DPNSUCCESS_PENDING;
  }

  HRESULT Send(const DPN_BUFFER_DESC* buffers, DWORD count, DWORD, void*, DPNHANDLE*, DWORD) override {
    if (!connected_ || !client_ || !buffers || count == 0) return kGeneric;
    auto server = client_->server.lock();
    if (!server || !server->callback) return kGeneric;
    for (DWORD i = 0; i < count; ++i) {
      if (!buffers[i].pBuffer || buffers[i].dwBufferSize == 0) continue;
      const DPNHANDLE handle = retain_buffer(buffers[i].pBuffer, buffers[i].dwBufferSize);
      DPNMSG_RECEIVE recv{};
      recv.dwSize = sizeof(recv);
      recv.dpnidSender = client_->player_id;
      recv.dpnidLocal = 1;
      recv.pReceiveData = buffer_ptr(handle);
      recv.dwReceiveDataSize = buffers[i].dwBufferSize;
      recv.pvPlayerContext = client_->player_context;
      recv.hBufferHandle = handle;
      const HRESULT hr = server->callback(server->context, DPN_MSGID_RECEIVE, &recv);
      if (hr != DPNSUCCESS_PENDING) release_buffer(handle);
    }
    return 0;
  }

  HRESULT EnumHosts(const DPN_APPLICATION_DESC* app, IDirectPlay8Address*, IDirectPlay8Address*,
                    void*, DWORD, DWORD, DWORD, DWORD, void* user, DPNHANDLE*, DWORD) override {
    if (!callback_ || !app) return kGeneric;
    auto server = LoopbackRegistry::instance().find(app->guidApplication);
    if (!server) return kGeneric;
    NativeAddress sender;
    WCHAR host[] = L"127.0.0.1";
    sender.AddComponent(L"hostname", host, static_cast<DWORD>((wcslen(host) + 1) * sizeof(WCHAR)), DPNA_DATATYPE_STRING);
    DPN_APPLICATION_DESC desc = server->app;
    desc.dwSize = sizeof(desc);
    desc.pHostAddress = &sender;
    DPNMSG_ENUM_HOSTS_RESPONSE response{};
    response.dwSize = sizeof(response);
    response.pApplicationDescription = &desc;
    response.pAddressSender = &sender;
    response.pvUserContext = user;
    response.dwRoundTripLatencyMS = 0;
    callback_(context_, DPN_MSGID_ENUM_HOSTS_RESPONSE, &response);
    return 0;
  }

  HRESULT GetSendQueueInfo(DWORD* messages, DWORD* bytes, DWORD) override {
    if (messages) *messages = 0;
    if (bytes) *bytes = 0;
    return 0;
  }

  HRESULT GetConnectionInfo(DPN_CONNECTION_INFO* info, DWORD) override {
    if (!info) return kGeneric;
    std::memset(info, 0, sizeof(*info));
    info->dwSize = sizeof(*info);
    info->dwRoundTripLatencyMS = connected_ ? 1 : 0;
    return connected_ ? 0 : kGeneric;
  }

  HRESULT ReturnBuffer(DPNHANDLE handle, DWORD) override { release_buffer(handle); return 0; }

 private:
  ULONG refs_{1};
  void* context_{nullptr};
  PFNDPNMESSAGEHANDLER callback_{nullptr};
  bool connected_{false};
  std::shared_ptr<LoopbackClient> client_;
};

class NativeDirectPlayServer final : public IDirectPlay8Server {
 public:
  ULONG AddRef() override { return ++refs_; }
  ULONG Release() override {
    const auto n = --refs_;
    if (!n) { Close(0); delete this; }
    return n;
  }

  HRESULT Initialize(void* context, PFNDPNMESSAGEHANDLER handler, DWORD) override {
    context_ = context;
    callback_ = handler;
    return 0;
  }

  HRESULT SetApplicationDesc(const DPN_APPLICATION_DESC* desc, DWORD) override {
    if (!server_) server_ = std::make_shared<LoopbackServer>();
    if (desc) {
      server_->app = *desc;
      server_->app_guid = desc->guidApplication;
      if (desc->pwszSessionName) server_->session_name = desc->pwszSessionName;
      server_->app.pwszSessionName = server_->session_name.empty() ? nullptr : const_cast<WCHAR*>(server_->session_name.c_str());
    }
    return 0;
  }

  HRESULT SetCaps(DPN_CAPS* caps, DWORD) override {
    return (caps && caps->dwSize >= sizeof(DPN_CAPS)) ? 0 : kGeneric;
  }

  HRESULT Close(DWORD) override {
    if (server_) {
      std::unordered_map<DPNID, std::shared_ptr<LoopbackClient>> clients;
      {
        std::lock_guard<std::mutex> lock(server_->mutex);
        clients.swap(server_->clients);
        server_->hosting = false;
      }
      LoopbackRegistry::instance().unregister_server(server_);
      for (auto& kv : clients) {
        auto& c = kv.second;
        if (callback_) {
          DPNMSG_DESTROY_PLAYER destroy{};
          destroy.dwSize = sizeof(destroy);
          destroy.dpnidPlayer = c ? c->player_id : kv.first;
          destroy.pvPlayerContext = c ? c->player_context : nullptr;
          callback_(context_, DPN_MSGID_DESTROY_PLAYER, &destroy);
        }
        if (c && c->callback) {
          DPNMSG_TERMINATE_SESSION msg{};
          msg.dwSize = sizeof(msg);
          c->callback(c->context, DPN_MSGID_TERMINATE_SESSION, &msg);
        }
        if (c) c->connected = false;
      }
    }
    hosted_ = false;
    return 0;
  }

  HRESULT GetLocalHostAddresses(IDirectPlay8Address** addrs, DWORD* count, DWORD) override {
    if (!count) return kGeneric;
    if (!addrs) { *count = 1; return kGeneric; }
    if (*count < 1) { *count = 1; return kGeneric; }
    auto* addr = new (std::nothrow) NativeAddress();
    if (!addr) return kGeneric;
    WCHAR host[] = L"127.0.0.1";
    addr->AddComponent(L"hostname", host, static_cast<DWORD>((wcslen(host) + 1) * sizeof(WCHAR)), DPNA_DATATYPE_STRING);
    addrs[0] = addr;
    *count = 1;
    return 0;
  }

  HRESULT Host(const DPN_APPLICATION_DESC* desc, IDirectPlay8Address**, DWORD, void*, void*, void*, DWORD) override {
    SetApplicationDesc(desc, 0);
    if (!server_) server_ = std::make_shared<LoopbackServer>();
    server_->context = context_;
    server_->callback = callback_;
    server_->hosting = true;
    LoopbackRegistry::instance().register_server(server_);
    if (callback_) {
      DPNMSG_CREATE_PLAYER self{};
      self.dwSize = sizeof(self);
      self.dpnidPlayer = 1;
      self.pvPlayerContext = nullptr;
      callback_(context_, DPN_MSGID_CREATE_PLAYER, &self);
    }
    hosted_ = true;
    return 0;
  }

  HRESULT SendTo(DPNID dpnid, const DPN_BUFFER_DESC* buffers, DWORD count, DWORD, void*, DPNHANDLE*, DWORD) override {
    if (!server_ || !buffers) return kGeneric;
    std::shared_ptr<LoopbackClient> client;
    {
      std::lock_guard<std::mutex> lock(server_->mutex);
      auto it = server_->clients.find(dpnid);
      if (it == server_->clients.end()) return kGeneric;
      client = it->second;
    }
    if (!client || !client->callback) return kGeneric;
    for (DWORD i = 0; i < count; ++i) {
      if (!buffers[i].pBuffer || buffers[i].dwBufferSize == 0) continue;
      const DPNHANDLE handle = retain_buffer(buffers[i].pBuffer, buffers[i].dwBufferSize);
      DPNMSG_RECEIVE recv{};
      recv.dwSize = sizeof(recv);
      recv.dpnidSender = 1; // server
      recv.dpnidLocal = dpnid;
      recv.pReceiveData = buffer_ptr(handle);
      recv.dwReceiveDataSize = buffers[i].dwBufferSize;
      recv.hBufferHandle = handle;
      const HRESULT hr = client->callback(client->context, DPN_MSGID_RECEIVE, &recv);
      if (hr != DPNSUCCESS_PENDING) release_buffer(handle);
    }
    return 0;
  }

  HRESULT DestroyClient(DPNID dpnid, void*, DWORD, DWORD) override {
    if (!server_) return kGeneric;
    std::shared_ptr<LoopbackClient> client;
    {
      std::lock_guard<std::mutex> lock(server_->mutex);
      auto it = server_->clients.find(dpnid);
      if (it == server_->clients.end()) return kGeneric;
      client = it->second;
      server_->clients.erase(it);
    }
    if (client) {
      if (callback_) {
        DPNMSG_DESTROY_PLAYER destroy{};
        destroy.dwSize = sizeof(destroy);
        destroy.dpnidPlayer = dpnid;
        destroy.pvPlayerContext = client->player_context;
        callback_(context_, DPN_MSGID_DESTROY_PLAYER, &destroy);
      }
      if (client->callback) {
        DPNMSG_TERMINATE_SESSION term{};
        term.dwSize = sizeof(term);
        client->callback(client->context, DPN_MSGID_TERMINATE_SESSION, &term);
      }
      client->connected = false;
    }
    return 0;
  }

  HRESULT GetConnectionInfo(DPNID, DPN_CONNECTION_INFO* info, DWORD) override {
    if (!info) return kGeneric;
    std::memset(info, 0, sizeof(*info));
    info->dwSize = sizeof(*info);
    info->dwRoundTripLatencyMS = 0;
    return 0;
  }

  HRESULT GetSendQueueInfo(DPNID, DWORD* messages, DWORD* bytes, DWORD) override {
    if (messages) *messages = 0;
    if (bytes) *bytes = 0;
    return 0;
  }

  HRESULT ReturnBuffer(DPNHANDLE handle, DWORD) override { release_buffer(handle); return 0; }

 private:
  ULONG refs_{1};
  void* context_{nullptr};
  PFNDPNMESSAGEHANDLER callback_{nullptr};
  bool hosted_{false};
  std::shared_ptr<LoopbackServer> server_;
};

}  // namespace

HRESULT fl_port_create_directplay(const GUID& clsid, const GUID& iid, void** out) {
  if (!out) return kNoInterface;
  *out = nullptr;
  if (guid_eq(clsid, FL_CLSID_DirectPlay8Client) && guid_eq(iid, FL_IID_IDirectPlay8Client))
    *out = static_cast<IDirectPlay8Client*>(new (std::nothrow) NativeDirectPlayClient());
  else if (guid_eq(clsid, FL_CLSID_DirectPlay8Address) && guid_eq(iid, FL_IID_IDirectPlay8Address))
    *out = static_cast<IDirectPlay8Address*>(new (std::nothrow) NativeAddress());
  else if (guid_eq(clsid, FL_CLSID_DirectPlay8Server) && guid_eq(iid, FL_IID_IDirectPlay8Server))
    *out = static_cast<IDirectPlay8Server*>(new (std::nothrow) NativeDirectPlayServer());
  return *out ? 0 : kNoInterface;
}
