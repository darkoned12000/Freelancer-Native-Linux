#pragma once
#include <dacom.h>
struct ISoundManager : public IDAComponent {
  GENRESULT DACOM_API QueryInterface(const char*, void**) override { return GR_INTERFACE_UNSUPPORTED; }
  unsigned long DACOM_API AddRef() override { return 1; }
  unsigned long DACOM_API Release() override { return 1; }
  virtual GENRESULT DACOM_API StartupDevice(void*, unsigned long) { return GR_OK; }
  virtual GENRESULT DACOM_API ShutdownDevice() { return GR_OK; }
  virtual HRESULT DACOM_API GetDirectSound(void**) { return 0; }
};
