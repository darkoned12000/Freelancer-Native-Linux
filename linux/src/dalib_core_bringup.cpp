// Minimal Linux bring-up implementation of DALib core data-path APIs (M8.2 integration step).
// Provides DALib::OpenData/CloseData and required globals so the bring-up client can exercise
// config/DATA loading through real DALib code paths without porting full DALib::Startup/Shutdown
// (SystemContainer/Engine/Audio/Render subsystems) yet.

#include "linux_port/windows.h"
#include "linux_port/platform.h"

#include <dacom.h>
#include <readfile.h>

#ifndef DALIB_API
#define DALIB_API
#endif

namespace DALib {

DALIB_API ICOManager* Dacom = nullptr;
DALIB_API IFileSystem* Data = nullptr;

DALIB_API void __cdecl OpenData(const char* DataPath) {
  if (!Dacom || !DataPath) {
    fl_port::log_message(fl_port::LogLevel::Error, "DALib::OpenData (bring-up): missing DACOM manager or data path");
    Data = nullptr;
    return;
  }

  // Matches retail DALib::OpenData descriptor layout/values (0x34 "FileSystem"/"DOS").
  static_assert(sizeof(FFileSystemDesc) >= 0x34, "FFileSystemDesc must cover DOS/UTF 0x34 prefix");
  static_assert(offsetof(FFileSystemDesc, Field30) == 0x30, "FFileSystemDesc.Field30 offset mismatch");

  FFileSystemDesc desc{};
  desc.Size = 0x34;
  desc.ComponentName = "FileSystem";
  desc.Implementation = "DOS";
  desc.Path = DataPath;
  desc.Access = GENERIC_READ;
  desc.ShareMode = FILE_SHARE_READ;
  desc.Security = nullptr;
  desc.Creation = OPEN_EXISTING;
  desc.Flags = 0x08000080;
  desc.TemplateFile = nullptr;
  desc.Parent = nullptr;
  desc.Extra = 0;
  desc.Field30 = 0xFFFFFFFFu;

  void* inst = nullptr;
  const GENRESULT gr = Dacom->CreateInstance(reinterpret_cast<AGGDESC*>(&desc), &inst);
  if (gr == GR_OK) {
    Data = reinterpret_cast<IFileSystem*>(inst);
    fl_port::log_message(fl_port::LogLevel::Info, "DALib::OpenData (bring-up): FileSystem/DOS created for DATA path");
  } else {
    Data = nullptr;
    fl_port::log_message(fl_port::LogLevel::Error,
                         std::string("DALib::OpenData (bring-up): CreateInstance(FileSystem/DOS) failed gr=") + std::to_string((int)gr));
  }
}

DALIB_API void __cdecl CloseData() {
  if (Data) {
    Data->Release();
    Data = nullptr;
    fl_port::log_message(fl_port::LogLevel::Info, "DALib::CloseData (bring-up): released DATA filesystem");
  }
}

} // namespace DALib
