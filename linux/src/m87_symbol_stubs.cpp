#include <cwchar>
#include <cstddef>

// M8.7 temporary stubs for upstream init/link closure (single-player-safe).
// Replace with real implementations as dependencies are linked.

struct IEnvironment;
struct FClientCommandLine;
struct DisplaySettings;
struct Win3D;
struct FHudWideText;
struct IFileSystem;
struct FPerfDisplaySettings;
struct IServer;
namespace Archetype { struct ICliObjFactory; class Root; int FlushMaterialCache(); }

// Globals
// G_bFpExceptionTrap provided by upstream startup_config.cpp when linked.
struct CLoginMgr;
alignas(8) unsigned char G_LoginMgr[8192];
IServer* G_Server = nullptr;

// --- misc/util ---
void FormatGroupedNumber(wchar_t* Out, int /*Value*/, int /*PrefixIds*/) {
  if (Out) Out[0] = L'\0';
}

// --- Common helpers (temporary) ---
unsigned int CreateID(char const*) { return 0; }
void DAEngine_create_filesystem(char const*, IFileSystem** out, char const*) { if (out) *out = nullptr; }
void GetFileSysPath(char* dest, char const*, IFileSystem*) { if (dest) dest[0] = '\0'; }
bool GetUserLocalDataPath(char* const dest) { if (dest) { dest[0] = '.'; dest[1] = '\0'; } return true; }

// --- Client init/bootstrap hooks ---
int ComputeInitialEnvironment() { return 0; }
IEnvironment* EnterClientState(int, IEnvironment*) { return nullptr; }
// LoadStartupConfig/ApplyDisplayCmdlineOverrides provided by upstream startup_config.cpp when linked.
void ApplyChangedPerfOptions() {}
void LoadPerfOptions(char const*, FPerfDisplaySettings*) {}
void SavePerfOptions(char const*) {}
// ClientNoteEngineReady/ClientPresentAllowed provided by window.cpp when linked.
void BeginClientRenderFrame() {}
void EndClientRenderFrame() {}
void EndClientRenderScene() {}
void PresentClientRenderFrame() {}
void UpdateClientTiming() {}
void TickClientFrameTimer() {}
void TickClientFrame(double) {}
double GetClientFrameTimeCapped() { return 1.0 / 60.0; }
int PumpMessages(int* quit) { if (quit) *quit = 0; return 0; }

// --- Multiplayer/single-player safety stubs ---
unsigned GetPlayerConnectionId() { return 0; }

// --- Teardown/shutdown stubs ---
int AleLibShutdown() { return 0; }

// --- UI/menu/chat/input/render stubs ---
void EnsureClientIMEFocus() {}
void ReleaseClientIMEFocus() {}
void RegisterAcceleratorGroup(char const*) {}
void MouseDispatchEvents() {}
void MenuUiInitToolTipAndRollover() {}
void NnErrorRunModalMessage(char const*) {}
void NnRolloverDestroyGlobal() {}
void NnSystemShutdown() {}
void ChatInit() {}
void ChatRegisterTextStyles() {}
void ChatShutdown() {}
void ClearButtonMontage() {}
void InitializeRichText(int) {}
void ReinitializeRichTextFonts() {}
void ShutdownRichText() {}
void WideTextFree(FHudWideText*) {}
void WideTextReserve(FHudWideText*, unsigned int) {}
void UiScrollClearShapes() {}
void LoadingScreenRegister() {}
void LoadingScreenReset() {}
void LoadingScreenSetImage(char const*) {}
void LoadingScreenShutdown() {}
void LoadingScreenStart() {}
void InitUiSoundTable() {}
void ShutdownUiSoundTable() {}
void StopAllClientSounds() {}
void ClearClientVoices() {}
void ShutdownClientSoundArch() {}
void Win3DStartupManager(int, int) {}
void Win3DShutdownManager() {}
void Win3DDeleteTree(Win3D*) {}
Win3D* Win3DGetFirstRegistered() { return nullptr; }
void Win3DResetState() {}
void Win3DSetPipelineClearColor(float, float, float, float) {}
void Win3DSetViewportRect(int, int, int, int, char const*, int) {}
void* WinFactoryCreate(char const*, char const*, Win3D*, void*) { return nullptr; }
void WarmUpEffectShapes() {}
void ClearEffectShapesFiles() {}
void EffectSystemShutdown() {}
void EffectArchShutdown() {}
void GraphLibraryShutdown() {}
void FontShutdown() {}
void FontDeviceReset() {}
void ShutdownClientTickProfile() {}
void ShutdownDebrisArchDb() {}
void ClearWin3DBuffers(unsigned int) {}
void StringShutdown() {}
void LoadClientIniData(char const*) {}
void ShutdownCharacterInfo() {}
void ShutdownServerConnection() {}
void* GetNnToolTipSingleton() { return nullptr; }
void* GetIntroFocusState() { return nullptr; }
void* GetGameflowState() { return nullptr; }
void* GetSpaceState() { return nullptr; }
void* GetMissionBriefingState() { return nullptr; }
bool GetSkipMachineWarnings() { return true; }
void SetSkipMachineWarnings(bool) {}

// C++ class/namespace stubs
namespace DataConfig {
  struct ProgressCB { virtual ~ProgressCB() = default; virtual int operator()(char const*, int, int) = 0; };
  int startup(char const*, Archetype::ICliObjFactory*, ProgressCB*) { return 0; }
  void shutdown() {}
}
namespace Reputation { namespace Vibe { unsigned int Count() { return 0; } } }
namespace Archetype { int FlushMaterialCache() { return 0; } }
namespace CGunWrapper { void Shutdown() {} }
namespace NnPreferences { void SeedGraphicsDefaults() {} }

struct Image { Image(); ~Image(); void Load(char const*); void Release(); void Draw(); };
Image::Image() {} Image::~Image() {} void Image::Load(char const*) {} void Image::Release() {} void Image::Draw() {}

struct DeviceTRC { static void set_image_root(char const*); };
void DeviceTRC::set_image_root(char const*) {}

struct CFLIDMaker { static void Shutdown(); };
void CFLIDMaker::Shutdown() {}

struct CDPClient { void Cleanup(); };
void CDPClient::Cleanup() {}

// Archetype factory global placeholder (type-unsafe, but symbol-only for now)
namespace Archetype { void* GCliObjFactory = nullptr; }

// LoginMgr method stubs via extern "C" won't match; provide as if CLoginMgr exists
struct CHARACTER_ID { unsigned char data[16]; };
struct CLoginMgr {
  void LoadConfig(char const*);
  void Reset();
  void DestroyGun();
  bool AddDirectServer(char const*);
  void Cleanup();
};
void CLoginMgr::LoadConfig(char const*) {}
void CLoginMgr::Reset() {}
void CLoginMgr::DestroyGun() {}
bool CLoginMgr::AddDirectServer(char const*) { return false; }
void CLoginMgr::Cleanup() {}

// CDPServer::CrashCleanup provided by linux/src/cdp_stubs.cpp (or real DALib TU when linked).

// G_LoginMgr is provided above as a byte-array placeholder (symbol: G_LoginMgr).

// Minimal Win32 message stub needed by upstream engine_init low-spec dialog path.
using HWND__ = struct HWND__ { int unused; };
using HWND = HWND__*;
using UINT = unsigned int;
using WPARAM = unsigned int;
using LPARAM = long;
using LRESULT = long;
extern "C" __attribute__((stdcall)) LRESULT SendMessageA(HWND, UINT, WPARAM, LPARAM) { return 0; }
