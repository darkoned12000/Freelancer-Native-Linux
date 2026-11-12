#pragma once
#include <dacom.h>
struct ISystemContainer : public IDAComponent {
  GENRESULT DACOM_API QueryInterface(const char*, void**) override { return GR_INTERFACE_UNSUPPORTED; }
  unsigned long DACOM_API AddRef() override { return 1; }
  unsigned long DACOM_API Release() override { return 1; }
  // Additional methods referenced by upstream DALib Startup (excluded in core probe builds).
  virtual GENRESULT DACOM_API LoadSystemComponents(...) { return GR_GENERIC; }
};
struct ISystem : public IDAComponent {
  GENRESULT DACOM_API QueryInterface(const char*, void**) override { return GR_INTERFACE_UNSUPPORTED; }
  unsigned long DACOM_API AddRef() override { return 1; }
  unsigned long DACOM_API Release() override { return 1; }
};
