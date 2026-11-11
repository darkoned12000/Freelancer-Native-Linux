#include "linux_port/platform.h"
#include "linux_port/win32_compat.h"

#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

// #1 ABI/module-init harness (spike):
// - validates basic calling-convention macro usage compiles on Linux toolchains
// - models DACOM-style factory registration without DllMain side effects
// - exercises a simple component create/query flow using REFIID/GUID comparison

namespace {

using DacomResult = long;

static const GUID IID_ITestComponent = {0x11111111u, 0x2222u, 0x3333u, {4,5,6,7,8,9,10,11}};

struct ITestComponent {
  virtual ~ITestComponent() = default;
  virtual DacomResult DACOM_API QueryInterface(REFIID riid, void** out) = 0;
  virtual unsigned long DACOM_API AddRef() = 0;
  virtual unsigned long DACOM_API Release() = 0;
};

struct ITestFactory {
  virtual ~ITestFactory() = default;
  virtual const char* name() const = 0;
  virtual DacomResult DACOM_API CreateInstance(void** out) = 0;
};

class SampleComponent final : public ITestComponent {
 public:
  DacomResult DACOM_API QueryInterface(REFIID riid, void** out) override {
    if (!out) return -1;
    if (IsEqualIID(riid, IID_ITestComponent)) {
      ++refs_;
      *out = static_cast<ITestComponent*>(this);
      return 0;
    }
    *out = nullptr;
    return -2;
  }
  unsigned long DACOM_API AddRef() override { return ++refs_; }
  unsigned long DACOM_API Release() override {
    if (refs_ == 0) return 0;
    --refs_;
    const auto r = refs_;
    if (r == 0) delete this;
    return r;
  }
 private:
  unsigned long refs_ = 1;
};

class SampleFactory final : public ITestFactory {
 public:
  const char* name() const override { return "SampleFactory"; }
  DacomResult DACOM_API CreateInstance(void** out) override {
    if (!out) return -1;
    auto* c = new SampleComponent();
    *out = static_cast<ITestComponent*>(c);
    return 0;
  }
};

// Simulated explicit module init export (replacement strategy for DllMain registration).
using ModuleInitFn = int(CDECL*)(void* registrar);

struct Registrar {
  std::vector<std::string> load_order;
  std::unordered_map<std::string, ITestFactory*> factories;

  void register_factory(ITestFactory* f) {
    if (!f) return;
    factories[f->name()] = f;
    load_order.push_back(f->name());
  }
};

int CDECL sample_module_init(void* opaque_registrar) {
  auto* reg = static_cast<Registrar*>(opaque_registrar);
  static SampleFactory factory;
  reg->register_factory(&factory);
  return 0;
}

} // namespace

int main() {
  fl_port::log_message(fl_port::LogLevel::Info, "abi_module_harness starting");

  // Module init/registration model (no DllMain side effects).
  Registrar reg;
  ModuleInitFn init = &sample_module_init;
  const int init_rc = init(&reg);
  std::cout << "module init rc: " << init_rc << "\n";
  std::cout << "registered factories: " << reg.load_order.size() << "\n";

  // Factory -> component creation/query flow.
  auto it = reg.factories.find("SampleFactory");
  if (it == reg.factories.end()) {
    std::cerr << "missing SampleFactory\n";
    return 1;
  }
  void* inst = nullptr;
  const auto cr = it->second->CreateInstance(&inst);
  std::cout << "CreateInstance rc: " << cr << "\n";
  auto* comp = static_cast<ITestComponent*>(inst);
  void* qi = nullptr;
  const auto qr = comp->QueryInterface(IID_ITestComponent, &qi);
  std::cout << "QueryInterface rc: " << qr << "\n";
  if (qi) static_cast<ITestComponent*>(qi)->Release();
  comp->Release();

  std::cout << "ABI/module-init harness: ok\n";
  return 0;
}
