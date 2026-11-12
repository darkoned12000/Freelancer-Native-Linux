# M5 — Graphics bring-up (status)

**Status:** completed in working tree (pending code review, not committed).  
**Acceptance target:** A Freelancer scene/frame (or minimal in-engine render target) can render and present.

## Summary

M5 adds a scoped native Vulkan backend behind a render-pipeline abstraction for Linux bring-up, wired to the M4 window. It implements a minimal but functional subset (lifecycle, device/surface/swapchain, clear+present, viewport, transforms, basic render states, draw submission stubs) sufficient to render and present a minimal in-engine frame on Linux (Wayland-first). A harness runs an engine-like frame loop that clears + presents 120 frames.

## Implemented

### Scoped native Vulkan backend (`fl_port::RenderPipeline`)
- `linux/include/linux_port/render_pipeline.h`, `linux/src/render_pipeline.cpp`.
- Implements a broad `IRenderPipeline`-style method surface (lifecycle, mode/buffer, transforms, lighting/material, textures/render-targets, draw submission, state) as a standalone class (not inheriting the upstream `IRenderPipeline` interface due to 32-bit/DirectX-header constraints; method names match for future wiring).
- **Vulkan (scoped native backend):** instance/surface (via SDL_Vulkan), device/queue selection, swapchain, image views, render pass (clear+store), framebuffers, command pool/buffers, sync primitives.
- **Lifecycle:** `Startup`, `ShutdownPipeline`/`shutdown()`, `CreateBuffers` (initializes Vulkan from M4 window handle), `DestroyGpuResources`.
- **Frame operations:** `ClearBuffers`, `BeginScene`/`EndScene`, `SwapBuffers`, `render_frame(t)` (acquire, clear, present).
- **Viewport/window/transforms:** `SetViewportRect`/`GetViewportRect`, `SetWindow`/`GetWindow`, `SetWorld`/`GetWorld`, `SetView`/`GetView`, `SetModelview`/`GetModelview`, `SetProjection`/`GetProjection`, `SetLookat`, `SetOrtho`, `SetPerspective`, `SetDepthRange`, `SetWorldN`.
- **State:** `SetPipelineState`/`GetPipelineState`, `SetRenderState`/`GetRenderState`, texture-stage state/transform/texture, render-target set/get.
- **Draw submission:** `DrawPrimitive`, `DrawIndexedPrimitiveUp`, `DrawPrimitiveVb`, `DrawIndexedPrimitiveVb` (return OK to keep engine call paths unblocked; full VB/UP draw routing is follow-on).
- **Textures/lights/materials:** return `GR_GENERIC` for unsupported (no fake success).
- **Resize:** `resize(w,h)` recreates swapchain/image views/framebuffers.

### Integration with M4 window
- `init_for_window(Window*, config)` initializes the backend from the M4 window's adapter handle and SDL Vulkan surface.
- Harness creates an M4 window (Vulkan flag) and renders/presents frames.

### Harness
- **Target:** `m5_graphics_harness` (built when usable target SDL3 and Vulkan libraries are available)
- **Source:** `linux/tests/m5_graphics_harness.cpp`
- **CMake:** `fl_linux_port_render` static library + `m5_graphics_harness`.
- **Behavior:** creates native Wayland window, initializes render pipeline, runs engine-like frame loop (pump input, render+present), auto-exits after 120 frames or 10s, reports presented frame count.

## Validation

- **64-bit preset (`linux-x64`, Wayland):** `m5_graphics_harness` → **PASS** (native window + Vulkan swapchain, 120/120 frames presented, clean shutdown).
- **32-bit preset (`linux-x86`):** `m5_graphics_harness` → **PASS** (120/120 frames presented on host with 32-bit SDL3/Vulkan).
- Other harnesses (M1–M4) remain passing on both presets.

## M5 review fixes applied

- **Removed Wine system include contamination** from CMake (no `/usr/include/wine` or `/usr/include/wine/windows`); added minimal `D3DCOLOR`/`D3DVIEWPORT8` to `linux/include/linux_port/d3d8.h` shim.
- **Fixed double event pumping** in `m5_graphics_harness.cpp` (removed redundant `window.pump_events(cb)` inside `run_loop` tick).
- **Fixed partial-initialization state leak** in `CreateBuffers`: reset partial state via `shutdown()` on entry and on every error path.
- **Fixed swapchain sharing mode**: `VK_SHARING_MODE_CONCURRENT` with both queue families when `graphics_family != present_family`, else `VK_SHARING_MODE_EXCLUSIVE`.
- **Per-swapchain-image present semaphores**: `render_finished` is now a `std::vector<VkSemaphore>` indexed by `image_index`.
- **HiDPI pixel dimensions & extent clamping**: `SDL_GetWindowSizeInPixels` + clamp to `[minImageExtent, maxImageExtent]` (both CreateBuffers and resize).
- **SetOrtho/SetLookat**: implemented D3D8 `OrthoOffCenterLH` and `LookAtLH` (previously identity stubs).
- **GetWindow**: 3rd output now returns `tracked_x_ + tracked_w_` (right edge) instead of `tracked_y_`.
- **Code vs comment alignment**: updated headers/comments to reflect clear-only render pass.
- **IRenderPipeline inheritance**: `RenderPipeline` now inherits `IRenderPipeline` on 32-bit builds (`FL_LINUX_32BIT`) with `RP_OVERRIDE` macro for vtable parity enforcement; standalone on 64-bit for portability.

## Follow-on (M6+)

- Implement full draw submission routing (VB/IB lock/unlock/select/copy, vertex/index buffer managers) and texture upload/bind for real scene rendering.
- Wire `IRenderPipeline` interface inheritance on 32-bit when DirectX 8 type shim is complete (currently standalone for portability).
- Integrate with engine frame loop and render manager (`RendComp`, `Shading`, `flmaterials`).
- Gamma control, multi-adapter/mode selection, fullscreen/windowed transitions.