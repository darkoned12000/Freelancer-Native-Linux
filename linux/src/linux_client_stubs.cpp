// M8.1/M8.4 stub/no-op surface for not-yet-implemented client bring-up calls.
// Keep stubs minimal, deterministic, and logged so missing behavior is visible.
// Expand incrementally as upstream TUs are linked.

#define FL_LINUX_PORT_LAYOUT_RELAXED 1
#include "linux_port/platform.h"

#if !defined(_WIN32)
#include "linux_port/windows.h"
#endif

#include "../../src/freelancer/Client/globals.h"
#include "../../src/freelancer/Client/input_manager.h"
#include "../../src/freelancer/Client/texture_shapes.h"
#include "../../src/freelancer/Client/client_cliobj.h"
#include "../../src/freelancer/Client/engine_light.h"
#include "../../src/freelancer/Client/font_atlas.h"
#include "../../src/freelancer/Client/hud_command_dispatch.h"
#include "../../src/freelancer/Client/input_manager.h"
#include "../../src/freelancer/Client/nn_montage_draw.h"
#include "../../src/freelancer/Client/nn_rollover.h"
#include "../../src/freelancer/Client/nn_tooltip.h"
#include "../../src/freelancer/Client/nn_ui_pulse.h"
#include "../../src/freelancer/Client/screen_quad_state.h"
#include "../../src/freelancer/Client/space_camera.h"
#include "../../src/freelancer/Client/space_scene_obj_draw.h"
#include "../../src/freelancer/Client/star_sphere.h"
#include "../../src/freelancer/Client/win3d_engine_submit.h"
#include "../../src/freelancer/Client/win3d_immediate.h"
#include "../../src/freelancer/Client/client_mesh_load.h"
#include "../../src/include/common_misc.h"
#include "../../src/freelancer/Client/login_mgr.h"
#include "../../src/freelancer/Client/connect_flow.h"
#include "../../src/freelancer/Client/menu_mp_session.h"
#include "../../src/freelancer/Client/nn_error.h"
#include "../../src/freelancer/Client/nn_exit.h"
#include "../../src/freelancer/Client/nn_network_operation.h"
#include "../../src/include/imeui.h"
#include "../../linux/include/dinput.h"

#include <cstdint>
#include <string>

extern "C" {

// Movies / cutscenes (DirectShow/Bink/etc. placeholders)
int fl_stub_movie_play(const char* path) {
  fl_port::log_message(fl_port::LogLevel::Warning,
                       std::string("[stub] movie play requested (bypassed/no-op): ") + (path ? path : "(null)"));
  return 0;
}
int fl_stub_movie_stop(void) {
  fl_port::log_message(fl_port::LogLevel::Debug, "[stub] movie stop (no-op)");
  return 0;
}
int fl_stub_movie_is_playing(void) {
  return 0;
}

// DirectShow/WMV bring-up blockers: deterministic bypass stubs (no playback)
int fl_stub_directshow_playback(const char* path) {
  fl_port::log_message(fl_port::LogLevel::Warning,
                       std::string("[stub] DirectShow playback bypassed (no-op): ") + (path ? path : "(null)"));
  return 0;
}

// IME blockers: stub minimal enable/disable/focus hooks for boot/menu bring-up
int fl_stub_ime_enable(int enable) {
  fl_port::log_message(fl_port::LogLevel::Debug,
                       std::string("[stub] IME ") + (enable ? "enabled" : "disabled") + " (no-op)");
  return 1;
}
int fl_stub_ime_set_focus(int focused) {
  fl_port::log_message(fl_port::LogLevel::Debug,
                       std::string("[stub] IME focus ") + (focused ? "gained" : "lost") + " (no-op)");
  return 1;
}
void* fl_stub_ime_get_context(void* hwnd) {
  (void)hwnd;
  return nullptr;
}
int fl_stub_ime_release_context(void* hwnd, void* himc) {
  (void)hwnd; (void)himc;
  return 1;
}

// EULA / UI gating placeholders
int fl_stub_eula_accepted(void) {
  fl_port::log_message(fl_port::LogLevel::Warning, "[stub] EULA gate bypassed/accepted for bring-up (no-op)");
  return 1;
}

// Registry write placeholders (legacy marker; Win32 Reg* APIs are XDG-backed in win32_registry.h)
long fl_stub_registry_write_noop(void) {
  fl_port::log_message(fl_port::LogLevel::Debug, "[stub] legacy registry write marker (no-op)");
  return 0; // ERROR_SUCCESS-like
}

// Generic not-yet-implemented marker for diagnostics
void fl_stub_not_implemented(const char* what) {
  if (!what) what = "(unknown)";
  fl_port::log_message(fl_port::LogLevel::Warning, std::string("[stub] not implemented: ") + what);
}

} // extern "C"

#if !defined(_WIN32)
// Win32 API stubs commonly needed by early client/bootstrap paths.
// Declarations provided by linux_port/win32_user.h (included via windows.h).
// Use extern "C" to match Win32 C linkage expected by upstream code.

extern "C" {

__attribute__((weak)) BOOL WINAPI ShowCursor(BOOL bShow) {
  static int display_count = 0;
  display_count += bShow ? 1 : -1;
  return display_count;
}

__attribute__((weak)) BOOL WINAPI SetForegroundWindow(HWND /*hWnd*/) { return TRUE; }

__attribute__((weak)) HWND WINAPI SetFocus(HWND hWnd) { return hWnd; }

__attribute__((weak)) BOOL WINAPI UpdateWindow(HWND /*hWnd*/) { return TRUE; }

// Minimal Win32 calls often hit during boot/menu bring-up (no real window manager semantics)
__attribute__((weak)) HWND WINAPI GetFocus(void) { return nullptr; }
__attribute__((weak)) HWND WINAPI GetForegroundWindow(void) { return nullptr; }
__attribute__((weak)) BOOL WINAPI IsWindow(HWND /*hWnd*/) { return TRUE; }
__attribute__((weak)) BOOL WINAPI ShowWindow(HWND /*hWnd*/, int /*nCmdShow*/) { return TRUE; }
__attribute__((weak)) BOOL WINAPI EnableWindow(HWND /*hWnd*/, BOOL /*bEnable*/) { return TRUE; }
__attribute__((weak)) BOOL WINAPI DestroyWindow(HWND /*hWnd*/) { return TRUE; }

} // extern "C"

#endif

// Weak stub for upstream InitializeEngine (src/freelancer/Client/engine_init.cpp) so the
// bring-up client can link/call it before the real upstream TU is closed. Overridden by
// the strong symbol once fl_upstream_client_init_tu is linked.
extern "C" { }
// Weak stub for Common.dll SinglePlayer() (multiplayer-only path; bring-up is single-player).
extern "C" { }
__attribute__((weak)) bool SinglePlayer() { return true; }

__attribute__((weak)) bool InitializeEngine(const FClientCommandLine* CommandLine) {
  (void)CommandLine;
  fl_port::log_message(fl_port::LogLevel::Warning,
                       "[stub] InitializeEngine invoked (upstream TU not linked) - returning true");
  return true;
}

// Weak stubs for remaining upstream client symbols (overridden when their TUs link).
__attribute__((weak)) void ClientInputOnFocusGained() {}
__attribute__((weak)) void ClientInputOnFocusLost() {}
__attribute__((weak)) void ClientInputOnMouseMove() {}
__attribute__((weak)) IMEFocus* IMEFocus_GetSingleton_stub() { return nullptr; }
// Out-of-line definition of the static member (weak via separate translation unit? Provide as normal; weak attribute on function)
__attribute__((weak)) IMEFocus* IMEFocus::GetSingleton() { return nullptr; }
__attribute__((weak)) bool IMEFocus::WndProc(HWND, UINT, WPARAM, LPARAM, int*) { return false; }
__attribute__((weak)) Win3D* NnErrorFind() { return nullptr; }
__attribute__((weak)) void NnExitOpen(bool bImmediate) { (void)bImmediate; }
__attribute__((weak)) bool NnNetworkOperationIsOpen() { return false; }
__attribute__((weak)) void MouseSetEnabled(bool bEnable, int Width, int Height, bool bFullscreen) { (void)bEnable; (void)Width; (void)Height; (void)bFullscreen; }

// Weak DirectInput8 globals/stubs for link closure
extern "C" const GUID IID_IDirectInput8A = {0,0,0,{0,0,0,0,0,0,0,0}};
extern "C" const GUID GUID_SysMouse = {0,0,0,{0,0,0,0,0,0,0,0}};
extern "C" const GUID DIPROP_BUFFERSIZE = {0,0,0,{0,0,0,0,0,0,0,0}};
DIDATAFORMAT c_dfDIMouse2 = {};
extern "C" HRESULT DirectInput8Create(void* hinst, DWORD dwVersion, const GUID& riidltf, void** ppvOut, void* punkOuter) { (void)hinst;(void)dwVersion;(void)riidltf;(void)ppvOut;(void)punkOuter; if (ppvOut) *ppvOut=nullptr; return 0; }

// Weak texture shape helpers
char const kShapeDbHeaderFull[] = "stub";
__attribute__((weak)) FTexShape* FShapeDB::Find(char const* Name, char const* SourceFile) { (void)Name;(void)SourceFile; return nullptr; }

// Weak input helpers
__attribute__((weak)) void KeyMapRouteInputMessage(unsigned int a, unsigned int b, unsigned int c) { (void)a;(void)b;(void)c; }
__attribute__((weak)) bool IMEFocus::IgnoreHotKey(tagMSG const*) const { return false; }

// Weak stubs for remaining Win3D/UI/camera/hud/mouse helpers
__attribute__((weak)) Archetype::FCliMaterialEntry* Archetype::ResolveMaterialEntryFromFs(IFileSystem*, char const*, bool, unsigned int) { return nullptr; }
__attribute__((weak)) int Archetype::ReleaseMaterialEntrySlot(Archetype::FCliMaterialEntry**) { return 0; }
__attribute__((weak)) void EngineLightClearRegisteredList() {}
__attribute__((weak)) unsigned FontAtlasMaterialKey() { return 0; }
__attribute__((weak)) double GetClientElapsedTime() { return 0; }
__attribute__((weak)) unsigned char GetClientMouseCursorDrawFlag() { return 0; }
__attribute__((weak)) int GetClientMouseWheel() { return 0; }
__attribute__((weak)) void GetClientSoftwareCursor(int*, int*, int*) {}
__attribute__((weak)) FSpaceCamera* GetWin3DSceneCamera() { return nullptr; }
__attribute__((weak)) FHudCmdRecord* HudCommandRecord() { return nullptr; }
__attribute__((weak)) void HudDestroyEventHandler(FHudCmdHandler*) {}
__attribute__((weak)) void HudRegisterCommandHandler(FHudCmdHandler*, unsigned int) {}
__attribute__((weak)) void HudUnregisterCommandHandler(FHudCmdHandler*) {}
__attribute__((weak)) void ImeFocusDrawPass() {}
__attribute__((weak)) void LoadButtonMontage() {}
__attribute__((weak)) void MouseFrameUpdate() {}
__attribute__((weak)) void MouseInitCursors() {}
__attribute__((weak)) void MouseShutdown() {}
__attribute__((weak)) void NnMontageFlushButtons() {}
__attribute__((weak)) void NnRolloverTickGlobal(double) {}
__attribute__((weak)) void NnToolTipTickGlobal(double) {}
__attribute__((weak)) void NnUiPulseAdvance(double) {}
__attribute__((weak)) void ScaleVec3(float*, float const*, float) {}
__attribute__((weak)) void ScreenQuadFlush(IRenderPipeline*, unsigned long) {}
__attribute__((weak)) void SpaceCameraSetHalfFovX(FSpaceCamera*, float) {}
__attribute__((weak)) void SpaceCameraSetNearFar(FSpaceCamera*, float, float) {}
__attribute__((weak)) bool SpaceCameraSetViewport(FSpaceCamera*, const int*) { return false; }
__attribute__((weak)) void SpaceCameraSetViewportEx(FSpaceCamera*, int const*, int) {}
__attribute__((weak)) void Win3DBindModelview(void*, float const*) {}
__attribute__((weak)) void Win3DModelCacheClear() {}
__attribute__((weak)) void Win3DSetViewport(ICamera*, char const*, int) {}
__attribute__((weak)) unsigned CopyBoundedString(char*, unsigned int, char const*) { return 0; }
__attribute__((weak)) Vector RandomVector(float) { Vector v{}; return v; }
__attribute__((weak)) bool G_bButtonMontageLoaded = false;

// Weak stubs for remaining client init/login/connect helpers
__attribute__((weak)) bool ConnectAndLogin(char const* IniPath) { (void)IniPath; return false; }
__attribute__((weak)) void File::close() {}
__attribute__((weak)) File::~File() {}
__attribute__((weak)) File::File() {}
__attribute__((weak)) void File::get_info() {}
__attribute__((weak)) unsigned GetPlayerBaseIdRetail() { return 0; }
__attribute__((weak)) bool MenuMpSessionIsReady(FServerInfo const* Server) { (void)Server; return false; }
// g_display global (DisplaySettings) if not linked
DisplaySettings g_display = {};
