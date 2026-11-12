#pragma once

// M5: scoped native Vulkan backend behind an IRenderPipeline-style contract.
// Implements a minimal but functional subset (lifecycle, device/surface/swapchain,
// clear+present, viewport, transforms, basic render states, draw submission stubs)
// sufficient to render and present a minimal in-engine target/frame on Linux.
// Note: render_frame currently performs a clear-only render pass; vertex buffer /
// pipeline draw submission is a follow-on (M6+). Comments updated to match.
// Unimplemented features return GR_GENERIC / GR_OK as appropriate (no fake success
// for genuinely unsupported operations where diagnostics matter).

#include "dacom.h"
#include "d3d8.h"

#if defined(FL_LINUX_32BIT)
#include "render/IRenderPipeline.h"
#endif

#include <cstdint>

#if defined(FL_LINUX_32BIT)
#define RP_OVERRIDE override
#else
#define RP_OVERRIDE
#endif
#include <functional>
#include <string>
#include <vector>

struct SDL_Window;

namespace fl_port {

class Window;

struct RenderPipelineConfig {
  int initial_width{960};
  int initial_height{540};
  bool vsync{true};
  bool prefer_wayland{true};
  // Clear color (linear-ish 0..1) used by ClearBuffers when color flag set.
  float clear_r{0.08f};
  float clear_g{0.10f};
  float clear_b{0.16f};
  float clear_a{1.0f};
};

#if defined(FL_LINUX_32BIT)
class RenderPipeline : public IRenderPipeline, public IComponentFactory {
#else
class RenderPipeline : public IComponentFactory {
#endif
 public:
  RenderPipeline();
  ~RenderPipeline();

  RenderPipeline(const RenderPipeline&) = delete;
  RenderPipeline& operator=(const RenderPipeline&) = delete;

  // Engine-facing (IRenderPipeline) lifecycle.
  GENRESULT DACOM_API Startup(const char* SectionName) RP_OVERRIDE;
  GENRESULT DACOM_API ShutdownPipeline() RP_OVERRIDE;
  GENRESULT DACOM_API SetPipelineState(int Id, int Value) RP_OVERRIDE;
  GENRESULT DACOM_API GetPipelineState(int Id, int* Out) RP_OVERRIDE;
  GENRESULT DACOM_API GetDeviceInfo(void* Out520) RP_OVERRIDE;
  GENRESULT DACOM_API QueryDeviceAbility(unsigned int Index, unsigned long* Out) RP_OVERRIDE;
  GENRESULT DACOM_API GetDisplayModeCount(unsigned int* Out) RP_OVERRIDE;
  GENRESULT DACOM_API GetDisplayModeEnum(unsigned int Index, void* Out) RP_OVERRIDE;
  GENRESULT DACOM_API SelectMode(int* Mode, unsigned int* OutIndex) RP_OVERRIDE;
  GENRESULT DACOM_API CreateBuffers(HWND Hwnd, int* Mode, int* OutMode) RP_OVERRIDE;
  GENRESULT DACOM_API GetModeBlob(void* OutAdapter, void* OutMode10Dwords) RP_OVERRIDE;
  GENRESULT DACOM_API DestroyGpuResources() RP_OVERRIDE;
  GENRESULT DACOM_API ClearBuffers(unsigned long Flags, const void* Rects) RP_OVERRIDE;
  GENRESULT DACOM_API SwapBuffers() RP_OVERRIDE;
  GENRESULT DACOM_API LockBuffer(void* Out) RP_OVERRIDE;
  GENRESULT DACOM_API UnlockBuffer() RP_OVERRIDE;
  GENRESULT DACOM_API GetBufferInterface(void* Out, void* Unused) RP_OVERRIDE;
  GENRESULT DACOM_API GetDeviceStats(void* Out) RP_OVERRIDE;
  GENRESULT DACOM_API SetViewportRect(int X, int Y, int W, int H) RP_OVERRIDE;
  GENRESULT DACOM_API GetViewportRect(int* OutX, int* OutY, int* OutW, int* OutH) RP_OVERRIDE;
  GENRESULT DACOM_API SetDepthRange(float MinZ, float MaxZ) RP_OVERRIDE;
  GENRESULT DACOM_API GetDepthRange(float* OutMinZ, float* OutMaxZ) RP_OVERRIDE;
  GENRESULT DACOM_API SetWindow(HWND Hwnd, int X, int Y, int W, int H) RP_OVERRIDE;
  GENRESULT DACOM_API GetWindow(HWND* OutHwnd, int* OutLeft, int* OutRight, int* OutW,
                                int* OutH); // no RP_OVERRIDE: OutRight semantics (right edge) differ from upstream naming
  GENRESULT DACOM_API SetWorld(const float* M12) RP_OVERRIDE;
  GENRESULT DACOM_API GetWorld(float* Out12) RP_OVERRIDE;
  GENRESULT DACOM_API SetView(const float* M12) RP_OVERRIDE;
  GENRESULT DACOM_API GetView(float* Out12) RP_OVERRIDE;
  GENRESULT DACOM_API SetModelview(const float* M12) RP_OVERRIDE;
  GENRESULT DACOM_API GetModelview(float* Out12) RP_OVERRIDE;
  GENRESULT DACOM_API SetProjection(const float* M16) RP_OVERRIDE;
  GENRESULT DACOM_API GetProjection(float* Out16) RP_OVERRIDE;
  GENRESULT DACOM_API SetLookat(float EyeX, float EyeY, float EyeZ, float AtX, float AtY,
                                float AtZ, float UpX, float UpY, float UpZ); // no RP_OVERRIDE: matches upstream signature
  GENRESULT DACOM_API SetOrtho(float Left, float Right, float Bottom, float Top,
                               float NearPlane, float FarPlane); // no RP_OVERRIDE: matches upstream signature
  GENRESULT DACOM_API SetPerspective(float FovDeg, float Aspect, float NearPlane,
                                     float FarPlane); // no RP_OVERRIDE: matches upstream signature
  GENRESULT DACOM_API SetLight(void* Handle, const D3DLIGHT8* Light) RP_OVERRIDE;
  GENRESULT DACOM_API DestroyLight(void* Handle) RP_OVERRIDE;
  GENRESULT DACOM_API GetLight(void* Handle, D3DLIGHT8* OutLight) RP_OVERRIDE;
  GENRESULT DACOM_API SetLightEnable(void* Handle, int Enable) RP_OVERRIDE;
  GENRESULT DACOM_API GetLightEnable(void* Handle, int* OutEnable) RP_OVERRIDE;
  GENRESULT DACOM_API SetMaterial(const D3DMATERIAL8* Material) RP_OVERRIDE;
  GENRESULT DACOM_API GetMaterial(D3DMATERIAL8* Material) RP_OVERRIDE;
  GENRESULT DACOM_API CreateTexture(int Width, int Height, unsigned int* Format,
                                    unsigned int Levels, unsigned int Flags,
                                    void** OutHandle); // no RP_OVERRIDE: stub (unsupported)
  GENRESULT DACOM_API DestroyTexture(void* Handle) RP_OVERRIDE;
  GENRESULT DACOM_API IsTexture(void* Handle) RP_OVERRIDE;
  GENRESULT DACOM_API LockTexture(void* Handle, int Level, void* Out) RP_OVERRIDE;
  GENRESULT DACOM_API UnlockTexture(void* Handle, int Level) RP_OVERRIDE;
  GENRESULT DACOM_API GetTextureFormat(void* Handle, int* OutFormat) RP_OVERRIDE;
  GENRESULT DACOM_API GetTextureDim(void* Handle, unsigned int* OutW, unsigned int* OutH,
                                    int* OutLevels);
  GENRESULT DACOM_API GetTextureInterface(void* Handle, void* UnusedA, void* UnusedB) RP_OVERRIDE;
  GENRESULT DACOM_API SetTextureLevelData(void* Handle, int Level, int SrcW, int SrcH,
                                          unsigned int Pitch, unsigned int* SrcFormat,
                                          const void* Pixels, void* A, void* B);
  GENRESULT DACOM_API BlitTexture(void* DstHandle, int DstLevel, void* SrcHandle, int SrcLevel) RP_OVERRIDE;
  GENRESULT DACOM_API SetRenderTarget(void* Handle, void* UnusedA, void* UnusedB) RP_OVERRIDE;
  GENRESULT DACOM_API GetRenderTarget(void* Handle) RP_OVERRIDE;
  GENRESULT DACOM_API BeginScene() RP_OVERRIDE;
  GENRESULT DACOM_API EndScene() RP_OVERRIDE;
  GENRESULT DACOM_API ResetRenderStatesToDefaults() RP_OVERRIDE;
  GENRESULT DACOM_API SetRenderState(D3DRENDERSTATETYPE State, int Value) RP_OVERRIDE;
  GENRESULT DACOM_API GetRenderState(D3DRENDERSTATETYPE State, int* Out) RP_OVERRIDE;
  GENRESULT DACOM_API SetTextureStageState(int Stage, int Type, int Value) RP_OVERRIDE;
  GENRESULT DACOM_API GetTextureStageState(int Stage, int Type, int* Out) RP_OVERRIDE;
  GENRESULT DACOM_API SetTextureStageTransform(int Stage, const float* Matrix16) RP_OVERRIDE;
  GENRESULT DACOM_API GetTextureStageTransform(int Stage, float* OutMatrix16) RP_OVERRIDE;
  GENRESULT DACOM_API SetTextureStageTexture(int Stage, void* Handle) RP_OVERRIDE;
  GENRESULT DACOM_API GetTextureStageTexture(int Stage, void** OutHandle) RP_OVERRIDE;
  GENRESULT DACOM_API VerifyState() RP_OVERRIDE;
  GENRESULT DACOM_API DrawPrimitive(unsigned long Prim, unsigned int Fvf, const void* Verts,
                                    unsigned int Count, void* Unused); // no RP_OVERRIDE: stub (unsupported)
  GENRESULT DACOM_API DrawIndexedPrimitiveUp(unsigned long Prim, unsigned int Fvf,
                                             const void* Verts, unsigned int VertCount,
                                             const void* Indices, unsigned int IndexCount,
                                             unsigned long Unused);
  GENRESULT DACOM_API DrawPrimitiveVb(unsigned long Prim, void* Vb, unsigned int StartVertex,
                                      unsigned int Count, void* Unused);
  GENRESULT DACOM_API DrawIndexedPrimitiveVb(unsigned long Prim, void* Vb, unsigned int BaseVertex,
                                             unsigned int NumVerts, const void* Indices,
                                             unsigned int IndexCount, unsigned long Unused);
  GENRESULT DACOM_API AddLight(void* Handle) RP_OVERRIDE;
  GENRESULT DACOM_API RemoveLight(void* Handle) RP_OVERRIDE;
  GENRESULT DACOM_API UpdateLight(void* Handle) RP_OVERRIDE;
  GENRESULT DACOM_API SetWorldN(int WorldIndex, const float* M12) RP_OVERRIDE;

  // IComponentFactory
  GENRESULT DACOM_API CreateInstance(AGGDESC* Desc, void** Instance) RP_OVERRIDE;
  GENRESULT DACOM_API QueryInterface(const char* iid, void** Out) RP_OVERRIDE;
  unsigned long DACOM_API AddRef() { return 1; }
  unsigned long DACOM_API Release() { return 1; }

  // Bring-up helpers (not part of IRenderPipeline).
  bool init_for_window(Window* window, const RenderPipelineConfig& config = {});
  void shutdown();
  bool is_initialized() const { return initialized_; }
  // Runs a full frame: begin scene, clear, draw triangle, end scene, present.
  // Returns false on fatal error.
  bool render_frame(float t_seconds);
  // Performance / frame pacing helpers.
  double last_frame_ms() const { return last_frame_ms_; }
  double avg_frame_ms() const { return avg_frame_ms_; }
  void reset_frame_stats() { frame_count_ = 0; avg_frame_ms_ = 0.0; last_frame_ms_ = 0.0; }
  // Resize handling (recreate swapchain).
  bool resize(int w, int h);

  // Status for diagnostics.
  std::string last_error() const { return last_error_; }
  int swapchain_image_count() const;

 private:
  // Vulkan state (scoped native backend).
  struct VkState;
  VkState* vk_{nullptr};

  Window* window_{nullptr};
  RenderPipelineConfig config_{};
  bool initialized_{false};
  std::string last_error_{};
  // Frame timing.
  double last_frame_ms_{0.0};
  double avg_frame_ms_{0.0};
  std::uint64_t frame_count_{0};

  // Stored transforms/state (minimal).
  float world_[12]{};
  float view_[12]{};
  float modelview_[12]{};
  float projection_[16]{};
  int viewport_[4]{0, 0, 0, 0};
  float depth_range_[2]{0.0f, 1.0f};
  int pipeline_state_[64]{};
  int render_state_cache_[256]{};

  // Window handle tracking for CreateBuffers/SetWindow.
  HWND tracked_hwnd_{nullptr};
  int tracked_x_{0}, tracked_y_{0}, tracked_w_{0}, tracked_h_{0};

  void set_error(const std::string& msg);
  void identity_matrices();
};

}  // namespace fl_port
