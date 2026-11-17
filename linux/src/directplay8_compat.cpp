#include "linux_port/directplay8_stubs.h"

#include <string>
#include <cstring>
#include <cwchar>
#include <cstdlib>
#include <new>

namespace {
constexpr HRESULT kNoInterface = static_cast<HRESULT>(0x80004002u);

class NativeAddress final : public IDirectPlay8Address {
 public:
  ULONG AddRef() override { return ++refs_; }
  ULONG Release() override { const auto n=--refs_; if(!n)delete this; return n; }
  HRESULT BuildFromURLW(WCHAR* url) override {
    if(!url)return DPNERR_GENERIC;
    url_=url;
    const std::wstring key=L"hostname=";
    const auto begin=url_.find(key);
    if(begin==std::wstring::npos)return DPNERR_GENERIC;
    const auto start=begin+key.size();
    const auto end=url_.find_first_of(L"&/?",start);
    host_=url_.substr(start,end==std::wstring::npos?std::wstring::npos:end-start);
    return host_.empty()?DPNERR_GENERIC:0;
  }
  HRESULT SetSP(const GUID* sp) override { if(!sp)return DPNERR_GENERIC; sp_=*sp; has_sp_=true; return 0; }
  HRESULT AddComponent(const WCHAR* name,const void* data,DWORD size,DWORD type) override {
    if(!name||!data||size==0)return DPNERR_GENERIC;
    if(std::wstring(name)==L"port" && type==DPNA_DATATYPE_DWORD && size>=sizeof(DWORD))
      std::memcpy(&port_,data,sizeof(port_));
    return 0;
  }
  HRESULT GetComponentByName(const WCHAR* name,void* data,DWORD* size,DWORD* type) override {
    if(!name||!size)return DPNERR_GENERIC;
    if(std::wstring(name)==L"hostname") {
      if(host_.empty())return DPNERR_GENERIC;
      const std::wstring& host=host_;
      const DWORD need=static_cast<DWORD>((host.size()+1)*sizeof(WCHAR));
      if(type)*type=DPNA_DATATYPE_STRING;
      const DWORD capacity=*size;
      *size=need;
      if(!data||capacity<need)return DPNERR_GENERIC;
      std::memcpy(data,host.c_str(),need);
      return 0;
    }
    if(std::wstring(name)==L"port") {
      if(type)*type=DPNA_DATATYPE_DWORD;
      if(data&&*size>=sizeof(port_))std::memcpy(data,&port_,sizeof(port_));
      *size=sizeof(port_); return data?0:DPNERR_GENERIC;
    }
    return DPNERR_GENERIC;
  }
  HRESULT GetURLA(char* output,DWORD* size) override {
    if(!size)return DPNERR_GENERIC;
    std::string host(host_.begin(),host_.end());
    std::string url="x-directplay:/hostname="+host;
    if(port_)url+="&port="+std::to_string(port_);
    const DWORD needed=static_cast<DWORD>(url.size()+1);
    const DWORD capacity=*size;
    *size=needed;
    if(!output||capacity<needed)return DPNERR_GENERIC;
    std::memcpy(output,url.c_str(),needed);
    return 0;
  }
 private:
  ULONG refs_{1}; std::wstring url_,host_; GUID sp_{}; bool has_sp_{false}; DWORD port_{2000};
};

class NativeDirectPlayClient final : public IDirectPlay8Client {
 public:
  ULONG AddRef() override { return ++refs_; }
  ULONG Release() override { const auto n=--refs_; if(!n)delete this; return n; }
  HRESULT Initialize(void* context, PFNDPNMESSAGEHANDLER handler, DWORD) override {
    context_=context; callback_=handler; return 0;
  }
  HRESULT SetCaps(DPN_CAPS* caps,DWORD) override { if(caps&&caps->dwSize>=sizeof(DPN_CAPS))return 0;return DPNERR_GENERIC; }
  HRESULT CancelAsyncOperation(DPNHANDLE,DWORD) override { return 0; }
  HRESULT Close(DWORD) override { connected_=false;return 0; }
  HRESULT Connect(const DPN_APPLICATION_DESC*,IDirectPlay8Address*,IDirectPlay8Address*,void*,void*,void*,DWORD,void*,DPNHANDLE*,DWORD) override {
    // A socket provider is required for real Freelancer server interoperability. Do not report
    // success while no compatible transport/session handshake is available.
    return DPNERR_GENERIC;
  }
  HRESULT Send(const DPN_BUFFER_DESC*,DWORD,DWORD,void*,DPNHANDLE*,DWORD) override {
    return connected_?DPNERR_GENERIC:DPNERR_GENERIC;
  }
  HRESULT EnumHosts(const DPN_APPLICATION_DESC*,IDirectPlay8Address*,IDirectPlay8Address*,void*,DWORD,DWORD,DWORD,DWORD,void*,DPNHANDLE*,DWORD) override {
    return DPNERR_GENERIC;
  }
  HRESULT GetSendQueueInfo(DWORD* messages,DWORD* bytes,DWORD) override { if(messages)*messages=0;if(bytes)*bytes=0;return 0; }
  HRESULT GetConnectionInfo(DPN_CONNECTION_INFO* info,DWORD) override { if(info){info->dwRoundTripLatencyMS=0;}return DPNERR_GENERIC; }
  HRESULT ReturnBuffer(DPNHANDLE,DWORD) override { return 0; }
 private:
  ULONG refs_{1}; void* context_{nullptr}; PFNDPNMESSAGEHANDLER callback_{nullptr}; bool connected_{false};
};

class NativeDirectPlayServer final : public IDirectPlay8Server {
 public:
  ULONG AddRef() override { return ++refs_; }
  ULONG Release() override { const auto n=--refs_; if(!n)delete this; return n; }
  HRESULT Initialize(void* context, PFNDPNMESSAGEHANDLER handler, DWORD) override {
    context_=context; callback_=handler; return 0;
  }
  HRESULT SetApplicationDesc(const DPN_APPLICATION_DESC*, DWORD) override { return 0; }
  HRESULT SetCaps(DPN_CAPS* caps, DWORD) override { if(caps&&caps->dwSize>=sizeof(DPN_CAPS))return 0;return DPNERR_GENERIC; }
  HRESULT Close(DWORD) override { hosted_=false; return 0; }
  HRESULT GetLocalHostAddresses(IDirectPlay8Address**, DWORD* count, DWORD) override {
    if(!count) return DPNERR_GENERIC;
    *count = 0; // No network service provider/addresses exposed by this stub.
    return DPNERR_GENERIC;
  }
  HRESULT Host(const DPN_APPLICATION_DESC*, IDirectPlay8Address**, DWORD, void*, void*, void*, DWORD) override {
    return DPNERR_GENERIC; // Real hosting requires DirectPlay transport/protocol implementation.
  }
  HRESULT SendTo(DPNID, const DPN_BUFFER_DESC*, DWORD, DWORD, void*, DPNHANDLE*, DWORD) override { return DPNERR_GENERIC; }
  HRESULT DestroyClient(DPNID, void*, DWORD, DWORD) override { return DPNERR_GENERIC; }
  HRESULT GetConnectionInfo(DPNID, DPN_CONNECTION_INFO* info, DWORD) override { if(info){info->dwRoundTripLatencyMS=0;} return DPNERR_GENERIC; }
  HRESULT GetSendQueueInfo(DPNID, DWORD* messages, DWORD* bytes, DWORD) override { if(messages)*messages=0; if(bytes)*bytes=0; return 0; }
  HRESULT ReturnBuffer(DPNHANDLE, DWORD) override { return 0; }
 private:
  ULONG refs_{1}; void* context_{nullptr}; PFNDPNMESSAGEHANDLER callback_{nullptr}; bool hosted_{false};
};

inline bool guid_eq(const GUID& a,const GUID& b){
  return a.Data1==b.Data1 && a.Data2==b.Data2 && a.Data3==b.Data3 && std::memcmp(a.Data4,b.Data4,8)==0;
}
}

HRESULT fl_port_create_directplay(const GUID& clsid,const GUID& iid,void** out) {
  if(!out)return kNoInterface;
  *out=nullptr;
  if(guid_eq(clsid, FL_CLSID_DirectPlay8Client) && guid_eq(iid, FL_IID_IDirectPlay8Client))
    *out=static_cast<IDirectPlay8Client*>(new(std::nothrow) NativeDirectPlayClient());
  else if(guid_eq(clsid, FL_CLSID_DirectPlay8Address) && guid_eq(iid, FL_IID_IDirectPlay8Address))
    *out=static_cast<IDirectPlay8Address*>(new(std::nothrow) NativeAddress());
  else if(guid_eq(clsid, FL_CLSID_DirectPlay8Server) && guid_eq(iid, FL_IID_IDirectPlay8Server))
    *out=static_cast<IDirectPlay8Server*>(new(std::nothrow) NativeDirectPlayServer());
  return *out?0:kNoInterface;
}
