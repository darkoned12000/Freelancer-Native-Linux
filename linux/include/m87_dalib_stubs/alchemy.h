#pragma once
// M8.7 Linux bring-up stub for upstream DALib core probe compilation.
#include <dacom.h>
struct IAlchemy : public IDAComponent {
  GENRESULT DACOM_API QueryInterface(const char*, void**) override { return GR_INTERFACE_UNSUPPORTED; }
  unsigned long DACOM_API AddRef() override { return 1; }
  unsigned long DACOM_API Release() override { return 1; }
  virtual GENRESULT DACOM_API CreateVertexBuffer() { return GR_OK; }
};
