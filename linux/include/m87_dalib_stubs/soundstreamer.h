#pragma once
#include <dacom.h>
struct FStreamerDesc { unsigned long Size; void* DirectSound; unsigned long BufferBytes; unsigned long ChannelCount; unsigned long Flags; unsigned long SampleRate; };
struct IStreamer2 : public IDAComponent {
  GENRESULT DACOM_API QueryInterface(const char*, void**) override { return GR_INTERFACE_UNSUPPORTED; }
  unsigned long DACOM_API AddRef() override { return 1; }
  unsigned long DACOM_API Release() override { return 1; }
  virtual int DACOM_API InitializeDevice(FStreamerDesc*) { return 0; }
  virtual GENRESULT DACOM_API ShutdownDevice() { return GR_OK; }
};
