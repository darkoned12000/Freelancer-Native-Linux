#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <cstddef>
#include <string>
#include <sys/stat.h>

#include <DALib.h>
#include <dacom.h>
#include <readfile.h>

#include "common_csys.h"
#include "common_id.h"
#include "common_ini.h"
#include "freelancer/Client/client_environment.h"
#include "linux_port/platform.h"

// M8.7 temporary stubs for upstream init/link closure (single-player-safe).
// Replace with real implementations as dependencies are linked.

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

int CallDacomStricmp(const char* Left, const char* Right);
struct option_spec { char const* Name; unsigned long Value; };
#if !defined(_MSC_VER) && !defined(_WIN32)
__attribute__((weak))
#endif
char const* __cdecl malloc_strcpy(char** Dest, char const* Src) {
  if (!Dest) return nullptr;
  if (Src) {
    *Dest = strdup(Src);
    return *Dest;
  }
  *Dest = nullptr;
  return *Dest;
}
#if !defined(_MSC_VER) && !defined(_WIN32)
__attribute__((weak))
#endif
void __cdecl malloc_strfree(char** Str) {
  if (Str && *Str) { free(*Str); *Str = nullptr; }
}
#if !defined(_MSC_VER) && !defined(_WIN32)
__attribute__((weak))
#endif
wchar_t const* __cdecl malloc_wstrcpy(wchar_t** Dest, wchar_t const* Src) {
  if (!Dest) return nullptr;
  if (Src) {
    *Dest = wcsdup(Src);
    return *Dest;
  }
  *Dest = nullptr;
  return *Dest;
}
#if !defined(_MSC_VER) && !defined(_WIN32)
__attribute__((weak))
#endif
void __cdecl malloc_wstrfree(wchar_t** Str) {
  if (Str && *Str) { free(*Str); *Str = nullptr; }
}
#if !defined(_MSC_VER) && !defined(_WIN32)
__attribute__((weak))
#endif
unsigned long __cdecl select_option_from_string(char* String, option_spec const* Options, int Count, char const* /*DefaultName*/) {
  if (!String || !Options || Count <= 0) return 0xfffffffful;
  char* Token = std::strtok(String, ",;/ ");
  (void)Token;
  for (int i = 0; i < Count; ++i) {
    if (CallDacomStricmp(String, Options[i].Name) == 0) return Options[i].Value;
  }
  return 0xfffffffful;
}

// Temporary globals/constants needed by linked upstream Common TUs (replace with real TUs/defs where feasible).
extern unsigned short const SUBOBJ_ID_NONE = 0;
float COLLISION_DAMAGE_FACTOR = 0.25f;
float LOOT_OWNER_SAFE_TIME = 1.0f;
float LOOT_UNSEEN_LIFE_TIME = 60.0f;
float LOOT_UNSEEN_RADIUS = 2500.0f;
float MUSIC_CROSS_FADE_DELAY = 30.0f;
float JETTISONED_CARGO_VELOCITY = 10.0f;
float MUZZLE_CONE_ANGLE = 90.0f;
float FIRE_FAILED_DELAY = 1.0f;
float PLAYER_COLLISION_GROUP_HIT_PTS_SCALE = 1.0f;
float PLAYER_ATTACHED_EQUIP_HIT_PTS_SCALE = 1.0f;
int MAX_PLAYER_AMMO = 50;
float COMM_PLAYER_FAR_DIST = 2000.0f;
float COMM_PLAYER_FAR_DIST_ATTEN = -15.0f;
float CHATTER_MAX_DIST = 2000.0f;
float CHATTER_MAX_DIST_ATTEN = -20.0f;
float CHATTER_START_ATTEN = -5.0f;
int COMM_CONFLICT_PRIORITY_CUTOFF = -2;
float WALLA_MAX_DIST = 2500.0f;
float WALLA_MAX_DIST_ATTEN = -26.0f;
float WALLA_START_ATTEN = -8.0f;
int WALLA_PRIORITY_CUTOFF = -4;
double DaysPerSecond = 1.0 / 60.0;
ID_String SND_CARGO_JETTISONED = {0};
ID_String FIRE_FAILED_SOUND = {0};

// arch_is_combinable provided by upstream common_goodinfo.cpp when linked.

namespace PhySys {
  float DEFAULT_LINEAR_DAMPING = 1.0f;
  float GOLEM_CHILD_MASS = 1.0f;
  float GOLEM_CHILD_LINEAR_DAMP = 1.0f;
  float GOLEM_MAX_DELTA_POSITION = 10.0f;
  float GOLEM_DELTA_ORIENTATION = 1.5707964f;
  float GOLEM_MAX_TORQUE = 1.0E+12f;
  float GOLEM_FORCE_FACTOR = 1.0f;
  float GOLEM_DAMP_FACTOR = 1.0f;
  float GOLEM_TORQUE_FACTOR = 1.0f;
  float GOLEM_ANGULAR_DAMP_FACTOR = 1.0f;
  float RC_MAX_DELTA_POSITION = 1.0f;
  float RC_MAX_DELTA_ORIENTATION = 1.0f;
  Vector DEFAULT_ANGULAR_DAMPING = {1.0f, 1.0f, 1.0f};
  Vector GOLEM_CHILD_ANGULAR_DAMP = {1.0f, 1.0f, 1.0f};
  Vector GOLEM_MAX_TRANSLATION_FORCE = {1.0e12f, 1.0e12f, 1.0e12f};
}
namespace Collision {
  bool Load(INI_Reader& Reader) {
    // Temporary bring-up stub: consume [CollisionConsts] values to keep INI parsing coherent.
    // Real behavior (collision damage map/max impact speed) is provided by common_physys.cpp when linked.
    while (Reader.read_value()) {
      // no-op
    }
    return true;
  }
}

// --- Common helpers (partial replacements for common_misc subset) ---
// CreateID provided by upstream common_id.cpp when linked.

void DAEngine_create_filesystem(char const* Path, IFileSystem** Out, char const* Implementation) {
  if (Out) *Out = nullptr;
  if (Path == nullptr || Path[0] == '\0' || Out == nullptr) return;

  IComponentFactory* factory = reinterpret_cast<IComponentFactory*>(DALib::Data);
  if (factory == nullptr) {
    factory = reinterpret_cast<IComponentFactory*>(DACOM_Acquire());
  }
  if (factory == nullptr) {
    FDUMP(0x100001, "m87_symbol_stubs.cpp(%d): *** ERROR: Could not load file %s (no factory)\n", 0, Path);
    return;
  }

  FFileSystemDesc Desc{};
  Desc.Size = 0x34;
  Desc.ComponentName = "FileSystem";
  Desc.Implementation = Implementation;
  Desc.Path = Path;
  Desc.Access = 0x80000000;
  Desc.ShareMode = 1;
  Desc.Creation = 3;
  Desc.Flags = 0x8000080;
  Desc.Field30 = 0xFFFFFFFF;

  factory->CreateInstance(reinterpret_cast<AGGDESC*>(&Desc), reinterpret_cast<void**>(Out));
  if (*Out == nullptr) {
    FDUMP(0x100001, "m87_symbol_stubs.cpp(%d): *** ERROR: Could not load file %s\n", 0, Path);
  }
}

void DAEngine_release_filesystem(IFileSystem** Filesys) {
  if (Filesys == nullptr) return;
  IFileSystem* Instance = *Filesys;
  if (Instance == nullptr) return;
  Instance->Release();
  *Filesys = nullptr;
}

void GetFileSysPath(char* Dest, char const* RelOrAbs, IFileSystem* Filesys) {
  if (Dest == nullptr) return;
  Dest[0] = '\0';
  char root[260] = {};
  if (Filesys != nullptr) {
    Filesys->GetFileSysPath(root, sizeof(root));
  } else if (DALib::Data != nullptr) {
    DALib::Data->GetFileSysPath(root, sizeof(root));
  }
  if (RelOrAbs == nullptr) {
    std::snprintf(Dest, 260, "%s", root);
    return;
  }
  // Minimal join; real impl uses _makepath/_fullpath semantics.
  if (root[0] != '\0') {
    const size_t rl = std::strlen(root);
    if (rl > 0 && (root[rl - 1] == '/' || root[rl - 1] == '\\'))
      std::snprintf(Dest, 260, "%s%s", root, RelOrAbs);
    else
      std::snprintf(Dest, 260, "%s/%s", root, RelOrAbs);
  } else {
    std::snprintf(Dest, 260, "%s", RelOrAbs);
  }
}

bool GetUserLocalDataPath(char* const dest) {
  if (!dest) return false;
  const char* xdg = std::getenv("XDG_STATE_HOME");
  if (xdg && *xdg) {
    std::snprintf(dest, 260, "%s/freelancer", xdg);
  } else {
    const char* home = std::getenv("HOME");
    if (home && *home) std::snprintf(dest, 260, "%s/.local/state/freelancer", home);
    else std::snprintf(dest, 260, ".freelancer");
  }
  // best-effort mkdir -p
  char tmp[260];
  std::snprintf(tmp, sizeof(tmp), "%s", dest);
  for (char* p = tmp + 1; *p; ++p) {
    if (*p == '/') { *p = '\0'; ::mkdir(tmp, 0755); *p = '/'; }
  }
  ::mkdir(tmp, 0755);
  return true;
}

// --- Client init/bootstrap hooks ---
// ComputeInitialEnvironment: provided by real src/freelancer/Client/intro_connect.cpp (Linux-guarded)
// EnterClientState/GetIntroFocusState: attempting real client_state.cpp integration (Task 1/2)
extern "C++" {
// placeholders for other state singletons until their real TUs are linked
}
namespace {
struct FlLinuxDummyEnvironment final : IEnvironment {
  int type = 0;
  explicit FlLinuxDummyEnvironment(int t = 0) : type(t) {}
  bool Slot00(unsigned, unsigned, unsigned, unsigned) override { return false; }
  bool Slot01(unsigned, unsigned, unsigned) override { return false; }
  bool Slot02(unsigned, unsigned, unsigned long) override { return false; }
  bool Slot03(unsigned, unsigned, unsigned, unsigned long) override { return false; }
  bool Slot04(unsigned, unsigned, unsigned) override { return false; }
  void Slot05() override {}
  void Slot06(unsigned) override {}
  int  GetEnvironmentType() override { return type; }
  void PreInit() override {}
  void Shutdown() override {}
  void Slot10() override {}
  void Resize(int, int) override {}
  bool Init(int) override { return true; }
  void Destroy() override {}
  int  Update(float) override { return 0; }
};
FlLinuxDummyEnvironment g_fl_dummy_space{0};
FlLinuxDummyEnvironment g_fl_dummy_gf{1};
FlLinuxDummyEnvironment g_fl_dummy_briefing{3};
}
// LoadStartupConfig/ApplyDisplayCmdlineOverrides provided by upstream startup_config.cpp when linked.
#if !defined(_MSC_VER) && !defined(_WIN32)
__attribute__((weak))
#endif
void ApplyChangedPerfOptions() {}
#if !defined(_MSC_VER) && !defined(_WIN32)
__attribute__((weak))
#endif
void LoadPerfOptions(char const*, FPerfDisplaySettings*) {}
#if !defined(_MSC_VER) && !defined(_WIN32)
__attribute__((weak))
#endif
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

// Stubs for real client_state.cpp integration (Linux bring-up, Task 1/2)
bool G_bNnScreenUp = false;
void LoadingScreenSetClear(unsigned char) {}
// ResetGameflowRoomLetterbox: provided by real src/freelancer/Client/gameflow.cpp (Linux bring-up guarded)
void SetClientQuitRequested(bool) {}
struct Win3D;
Win3D* Win3DFindByName(char const*) { return nullptr; }
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
// StringShutdown provided by upstream common_stringcache.cpp when linked.
void ShutdownCharacterInfo() {}
void ShutdownServerConnection() {}
void* GetNnToolTipSingleton() { return nullptr; }
// GetIntroFocusState/EnterClientState: provided by real src/freelancer/Client/client_state.cpp (when linked)
// GetGameflowState: provided by real src/freelancer/Client/gameflow.cpp (Linux bring-up guarded)
// GetSpaceState: provided by real src/freelancer/Client/space_state.cpp (Linux bring-up guarded)
// GetMissionBriefingState: provided by real src/freelancer/Client/mission_briefing.cpp (Linux bring-up guarded)
#if !defined(_MSC_VER) && !defined(_WIN32)
__attribute__((weak))
#endif
bool GetSkipMachineWarnings() { return true; }
#if !defined(_MSC_VER) && !defined(_WIN32)
__attribute__((weak))
#endif
void SetSkipMachineWarnings(bool) {}

// C++ class/namespace stubs
// --- DataConfig loader/subsystem stubs (temporary, partial). Real upstream TUs linked for:
// RtcSlider, pub::StateGraph, NewChar::TheDB, Loadout, costumes, constants, reputation,
// and Archetype loaders/core (partial deps stubbed below).
// Universe::Startup/Shutdown provided by upstream common_universe.cpp when linked (with deps).

#if !defined(_MSC_VER) && !defined(_WIN32)
__attribute__((weak))
#endif
bool __cdecl IsMPServer() { return false; }
// Reputation::Startup/Shutdown provided by upstream common_reputation.cpp when linked (with dep stubs).
// ReadConstants provided by upstream common_constants.cpp when linked (with temporary dep stubs below).
// BaseDataList_load provided by upstream common_basedata.cpp when linked.
// BaseDataList_destroy provided by upstream common_basedata.cpp when linked.
// BaseDataList_load_market_data provided by upstream common_basedata.cpp when linked.
// GoodList_* provided by upstream common_goodinfo.cpp when linked.
// Costume/bodypart description functions provided by upstream common_costume.cpp when linked.
// PetalDB_*, FuseDB::*, WeaponModDB_* provided by upstream common_fuses.cpp (linked).
// Loadout::Load/Free provided by upstream common_loadout.cpp when linked.
// Reputation::Vibe::Count provided by upstream common_vibe.cpp when linked.
namespace Archetype {
#if !defined(_MSC_VER) && !defined(_WIN32)
__attribute__((weak))
#endif
int FlushMaterialCache() { return 0; }
}
namespace CGunWrapper { void Shutdown() {} }
namespace NnPreferences { void SeedGraphicsDefaults() {} }

#if !defined(_MSC_VER) && !defined(_WIN32)
__attribute__((weak))
#endif
bool GetUserDataPath(char* const dest) {
  if (!dest) return false;
  const char* xdg = std::getenv("XDG_DATA_HOME");
  if (xdg && *xdg) {
    std::snprintf(dest, 260, "%s/freelancer", xdg);
  } else {
    const char* home = std::getenv("HOME");
    if (home && *home) std::snprintf(dest, 260, "%s/.local/share/freelancer", home);
    else std::snprintf(dest, 260, ".freelancer");
  }
  char tmp[260];
  std::snprintf(tmp, sizeof(tmp), "%s", dest);
  for (char* p = tmp + 1; *p; ++p) {
    if (*p == '/') { *p = '\0'; ::mkdir(tmp, 0755); *p = '/'; }
  }
  ::mkdir(tmp, 0755);
  return true;
}

struct Image { Image(); ~Image(); void Load(char const*); void Release(); void Draw(); };
Image::Image() {} Image::~Image() {} void Image::Load(char const*) {} void Image::Release() {} void Image::Draw() {}

struct DeviceTRC { static void set_image_root(char const*); };
void DeviceTRC::set_image_root(char const*) {}

// CFLIDMaker provided by upstream common_id.cpp when linked.

struct CDPClient { void Cleanup(); };
void CDPClient::Cleanup() {}

// Archetype::GCliObjFactory provided by linux/src/m87_cliobj_factory_bridge.cpp (compatible bring-up bridge).

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
extern "C" LRESULT WINAPI SendMessageA(HWND, UINT, WPARAM, LPARAM) { return 0; }

// Bring-up weak stubs for client data/config loaders not yet linked on Linux.
// These allow the real upstream dataconfig_progress.cpp LoadClientIniData path to link
// incrementally; when real loader TUs are integrated they should replace these stubs.
#if !defined(_MSC_VER) && !defined(_WIN32)
__attribute__((weak)) int LoadFonts(char const*, IFileSystem*) { return 0; }
__attribute__((weak)) void LoadRichFontsFromIni(char const*, IFileSystem*) {}
__attribute__((weak)) int LoadClientSoundArchIni(char const*, bool) { return 0; }
__attribute__((weak)) int LoadClientVoicesIni(char const*) { return 0; }
__attribute__((weak)) int EffectArchLoadIni(char const*, bool) { return 0; }
__attribute__((weak)) void AddEffectShapesFile(char const*) {}
__attribute__((weak)) int LoadDebrisArchFile(char const*, bool) { return 0; }
__attribute__((weak)) void SetHudIniPath(char const*) {}
__attribute__((weak)) int GraphLibraryLoad(char const*) { return 0; }
__attribute__((weak)) void LoadClientSoundCfgIni(char const*) {}
#endif



// --- Weak stubs for client_cliobj.cpp dependency closure ---
struct FEffectArch;
#if !defined(_MSC_VER) && !defined(_WIN32)
__attribute__((weak))
#endif
FEffectArch* FindEffectArch(unsigned int const*) { return nullptr; }
#if !defined(_MSC_VER) && !defined(_WIN32)
__attribute__((weak))
#endif
void EffectArchAddRef(FEffectArch*, unsigned int) {}
#if !defined(_MSC_VER) && !defined(_WIN32)
__attribute__((weak))
#endif
void EffectArchRelease(FEffectArch*) {}
#if !defined(_MSC_VER) && !defined(_WIN32)
__attribute__((weak))
#endif
float ShipCameraClampTurnAngle(float Degrees) {
  if (Degrees < 0.0f) return 0.0f;
  if (Degrees > 90.0f) return 90.0f;
  return Degrees;
}

// --- Weak stubs for perf_options.cpp / Linux bring-up dependency closure ---
// These satisfy symbols referenced by src/freelancer/Client/perf_options.cpp when linked
// into the upstream init TU, without pulling heavy runtime subsystems. Real TUs override.

#if !defined(_MSC_VER) && !defined(_WIN32)
#define FL_M87_WEAK __attribute__((weak))
#else
#define FL_M87_WEAK
#endif

FL_M87_WEAK float G_SunEffectQuality = 0.0f;

FL_M87_WEAK bool GfScriptSequenceSkipLatch() { return false; }
FL_M87_WEAK void GfScriptSequenceSetSkipOption(float) {}

FL_M87_WEAK void SetClientSoundLod(int) {}
FL_M87_WEAK void SetClientAudioMusicVolume(float) {}
FL_M87_WEAK void SetClientAudioAmbientVolume(float) {}
FL_M87_WEAK void SetClientAudioSfxVolume(float) {}
FL_M87_WEAK void SetClientAudioInterfaceVolume(float) {}
FL_M87_WEAK void SetClientAudioVoiceVolume(float) {}

FL_M87_WEAK void ApplyMouseSpeedOption(float) {}
FL_M87_WEAK void NebulaApplyGlobalDetail(float) {}
FL_M87_WEAK void CliAsteroidApplyGlobalDetail(float) {}
FL_M87_WEAK void StarSphereApplyDetail(float) {}
FL_M87_WEAK void SolarSpheresApplyDetail(float) {}

struct FStarSphereList;
FL_M87_WEAK FStarSphereList* StarSphereList() { return nullptr; }

class IObjRW;
class CShip;
FL_M87_WEAK void* GetPlayerObjRW() { return nullptr; }
FL_M87_WEAK CShip* HudShipOfObjRw(IObjRW*) { return nullptr; }

namespace Archetype {
struct ICliObj;
FL_M87_WEAK void CliObjSetArchLodGlobals(unsigned char, float) {}
FL_M87_WEAK void CliObjRecomputeLodPick(ICliObj*, float const*) {}
}

#undef FL_M87_WEAK

// LoadClientIniData is provided by upstream src/freelancer/Client/dataconfig_progress.cpp
// (linked in fl_upstream_client_init_tu for Linux bring-up). Weak client-loader stubs above
// satisfy missing subsystem dependencies incrementally. If this TU is ever unlinked, restore
// a weak fallback implementation here.

// IVP memory helpers (bring-up stubs; real IVP runtime not linked).
void* ivp_malloc_aligned(unsigned int size, unsigned int alignment) {
  void* p = nullptr;
  if (alignment < sizeof(void*)) alignment = sizeof(void*);
  if (posix_memalign(&p, alignment, size ? size : 1) != 0) return nullptr;
  return p;
}
void ivp_free_aligned(void* p) { free(p); }

// Geometry helpers needed by upstream common_zone.cpp/common_pathfind probe code.
// Implementations derived from src/Common/common_pathfind.cpp (FUN_062dbb90/FUN_062dba00)
// to satisfy universe/zone link deps without pulling the full PathfindManager TU.
#include "x86math.h"
#include <cmath>

void MathTransposeTransformVia3DMathEngine(Vector* Out, Matrix const* M, Vector const* Src)
{
  I3DMathEngine* const Engine = Acquire3DMathEngine();
  if (!Engine || !Out || !M || !Src)
  {
    if (Out) *Out = Vector();
    return;
  }
  FVector Scratch;
  Engine->TransposeTransform(Scratch,
                             *reinterpret_cast<FMatrix const*>(M),
                             *reinterpret_cast<FVector const*>(Src));
  *Out = *reinterpret_cast<Vector*>(&Scratch);
}

bool ClampPointToBoxSurface(unsigned char* Box, Vector const* Src, Vector* Dst)
{
  if (!Box || !Src || !Dst)
    return false;
  float const* const HalfExtent = reinterpret_cast<float const*>(Box + 0x30);
  float const* const In = reinterpret_cast<float const*>(Src);
  float* const OutAxis = reinterpret_cast<float*>(Dst);

  float Overshoot[3];
  bool bWasOutside = false;

  for (int Axis = 0; Axis < 3; ++Axis)
  {
    Overshoot[Axis] = std::fabs(In[Axis]) - HalfExtent[Axis];
    if (Overshoot[Axis] >= 0.0f)
    {
      OutAxis[Axis] = static_cast<float>(std::copysign(HalfExtent[Axis], In[Axis]));
      bWasOutside = true;
    }
    else
    {
      OutAxis[Axis] = In[Axis];
    }
  }

  if (!bWasOutside)
  {
    int NearestAxis = 2;
    if (Overshoot[0] >= Overshoot[1])
    {
      if (Overshoot[0] >= Overshoot[2])
        NearestAxis = 0;
    }
    else if (Overshoot[1] >= Overshoot[2])
    {
      NearestAxis = 1;
    }
    OutAxis[NearestAxis] = static_cast<float>(std::copysign(HalfExtent[NearestAxis], In[NearestAxis]));
  }

  return bWasOutside;
}
