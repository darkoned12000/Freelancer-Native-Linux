// Linux M8.7: buildable upstream-derived DALib core TU (data-path/init subset).
// Implements retail-equivalent DALib::OpenData/CloseData, defines DALib globals and
// DALib error hooks, and provides bring-up stubs for Startup/Shutdown so config/DATA
// loading and early client integration exercise real DALib code paths/symbols.
// Full upstream Startup/Shutdown (SystemContainer/Engine/Sound/Streamer/Alchemy/DirectPlay/CDP)
// and networking TUs remain pending Linux porting.

#include "linux_port/windows.h"
#include "linux_port/platform.h"

#include <dacom.h>
#include <readfile.h>

// Forward declarations for DALib subsystem interface globals (kept as nullptr in M8.7 bring-up core).
struct IEngine;
struct IEngine2;
struct IAnimation2;
struct IHardpoint;
struct IDeformable;
struct IChannel;
struct IRenderPipeline;
struct IRPDraw;
struct IRPIndexBuffer;
struct IRPVertexBuffer;
struct IRenderer2;
struct IFileSystem;
struct ISoundManager;
struct IStreamer2;
struct ITextureLibrary2;
struct IMaterialLibrary;
struct IMaterialBatcher2;
struct IVertexBufferManager;
struct IAlchemy;
struct IFxRuntime;
struct IFxEffectLibrary;
struct IMatAnimLibrary;
struct IVMeshLibrary;

#ifndef DALIB_API
#define DALIB_API
#endif

// Global DALib error hooks (dalib_core.h ABI).
using FatalErrorFnType = void(__cdecl*)(unsigned int, const char*);
using CriticalWarningFnType = bool(__cdecl*)(unsigned int, const char*);
DALIB_API FatalErrorFnType FatalErrorFn = nullptr;
DALIB_API CriticalWarningFnType CriticalWarningFn = nullptr;

namespace {

template <typename T>
void ReleaseGlobal(T*& global) {
  if (global != nullptr) {
    global->Release();
    global = nullptr;
  }
}

} // namespace

namespace DALib {

// Core globals (DALib.h). Only Dacom/Data are actively used in M8.7 bring-up data-path.
DALIB_API ICOManager* Dacom = nullptr;
DALIB_API IEngine* Engine = nullptr;
DALIB_API IEngine2* Engine2 = nullptr;
DALIB_API IRenderer2* Renderer = nullptr;
DALIB_API IAnimation2* Anim = nullptr;
DALIB_API IHardpoint* Hardpoint = nullptr;
DALIB_API IDeformable* Deform = nullptr;
DALIB_API IChannel* Channel = nullptr;
DALIB_API IRenderPipeline* Pipeline = nullptr;
DALIB_API IRPDraw* RPDraw = nullptr;
DALIB_API IRPIndexBuffer* RPIndexBuffer = nullptr;
DALIB_API IRPVertexBuffer* RPVertexBuffer = nullptr;
DALIB_API IFileSystem* Data = nullptr;
DALIB_API ISoundManager* Sound = nullptr;
DALIB_API IStreamer2* Streamer = nullptr;
DALIB_API ITextureLibrary2* TextureLib = nullptr;
DALIB_API IMaterialLibrary* MaterialLib = nullptr;
DALIB_API IMaterialBatcher2* MaterialBatcher = nullptr;
DALIB_API IVertexBufferManager* VertexBuffer = nullptr;
DALIB_API IAlchemy* Alchemy = nullptr;
DALIB_API IFxRuntime* FxRuntime = nullptr;
DALIB_API IFxEffectLibrary* FxEffectLibrary = nullptr;
DALIB_API IMatAnimLibrary* MatAnimLibrary = nullptr;
DALIB_API IVMeshLibrary* VMeshLibrary = nullptr;

// Retail-equivalent DALib::OpenData (.text:065c1610): 0x34-byte "FileSystem"/"DOS" descriptor,
// result written straight into DALib::Data.
DALIB_API void __cdecl OpenData(const char* DataPath) {
  if (!Dacom) Dacom = DACOM_Acquire();

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
  const GENRESULT gr = Dacom ? Dacom->CreateInstance(reinterpret_cast<AGGDESC*>(&desc), &inst) : GR_GENERIC;
  if (gr == GR_OK) {
    Data = reinterpret_cast<IFileSystem*>(inst);
    fl_port::log_message(fl_port::LogLevel::Info, "DALib::OpenData (M8.7 upstream-derived core): FileSystem/DOS created for DATA path");
  } else {
    Data = nullptr;
    fl_port::log_message(fl_port::LogLevel::Error,
                         std::string("DALib::OpenData (M8.7 upstream-derived core): failed gr=") + std::to_string((int)gr));
  }
}

// Retail-equivalent DALib::CloseData (.text:065c1690).
DALIB_API void __cdecl CloseData() {
  ReleaseGlobal(Data);
  fl_port::log_message(fl_port::LogLevel::Info, "DALib::CloseData (M8.7 upstream-derived core): DATA filesystem released");
}

// M8.7 bring-up stubs: upstream Startup/Shutdown require SystemContainer/Engine/Sound/Streamer/
// Alchemy/render pipeline/DirectPlay initialization not yet ported to Linux.
DALIB_API bool __cdecl Startup(HWND /*hwnd*/, const char* /*ini_path*/) {
  fl_port::log_message(fl_port::LogLevel::Warning,
                       "DALib::Startup (M8.7 upstream-derived core): subsystem init pending (DACOM acquired, full SystemContainer init not ported)");
  if (!Dacom) Dacom = DACOM_Acquire();
  return Dacom != nullptr;
}

DALIB_API void __cdecl Shutdown(HWND /*hwnd*/) {
  fl_port::log_message(fl_port::LogLevel::Warning,
                       "DALib::Shutdown (M8.7 upstream-derived core): full subsystem teardown pending");
  // Data lifecycle remains explicit via CloseData in bring-up flow.
}

} // namespace DALib
