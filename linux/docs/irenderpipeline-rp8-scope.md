# #4 de-risk: `IRenderPipeline` / `RP8` scope findings (initial)

This note captures an initial scope assessment for the native-renderer path (Level 2/fallback) alongside D3D8-compat feasibility work.

## Key source interfaces
- **`src/include/render/IRenderPipeline.h`**: primary render pipeline contract (large vtable surface). Includes:
  - lifecycle/mode/buffer/device: `Startup`, `ShutdownPipeline`, `SetPipelineState/GetPipelineState`, `SelectMode`, `CreateBuffers`, `DestroyGpuResources`, `ClearBuffers`, `SwapBuffers`, `LockBuffer/UnlockBuffer`, viewport/window/depth/state management.
  - transforms/camera: `Set/Get World/View/Modelview/Projection`, `SetLookat`, `SetOrtho`, `SetPerspective`, `SetWorldN`.
  - lighting/material: `Set/Get/Destroy Light`, enable/update light, `Set/GetMaterial`.
  - textures/render targets: `CreateTexture/DestroyTexture/Lock/Unlock`, format/dim/query, upload/blit, render-target bind/get.
  - draw/state: render/texture-stage state, primitive and indexed draws (UP/VB variants), stream/state reset/verify.
  - related interfaces in same header area: vertex/index buffer management, draw interfaces, gamma/texture helper interfaces (broad subsystem contract beyond a minimal triangle path).
- **`src/include/render/IRenderPipelineEx.h`**, **`src/include/render/RenderPipeline.h`**: extensions/helpers around pipeline usage.
- **`src/RP8/`** (`RP8.cpp`, `RP8.h`, `Mgsdb.cpp/.h`): D3D8-based implementation of the render pipeline stack (startup, mode/device/buffer management, state caches, resource pools, draw paths, config/profile integration). This is a substantial implementation, not a thin adapter.

## Observed complexity signals
- `IRenderPipeline` alone exposes many methods spanning fixed-function-era state, transforms, lighting, textures, buffers, and draw submission (high ABI/behavioral surface).
- `RP8` implementation/state (`FDirectX8` and related caches/pools) indicates tight coupling to D3D8 device behavior, resource lifetime semantics, and engine-specific state expectations.
- Boot-to-first-frame likely requires only a subset, but correctness risk is in subtle state/format/coordinate and resource behavior, not just API count.

## Minimal boot/render subset (hypothesis for feasibility)
To reach window + cleared/presented frame and potentially a minimal scene/UI slice, prioritize:
1. **Device/window/present path:** `Startup`, mode selection/buffer creation, `BeginScene/EndScene`, `ClearBuffers`, `SwapBuffers`, viewport/window/depth basics.
2. **Core state/transforms:** projection/view/world/modelview setup, basic render states/sampler/texture-stage defaults needed by early draw paths.
3. **Resource basics:** texture create/upload/bind (limited formats), vertex/index buffer create/lock/unlock/select/draw for simple geometry.
4. **Draw submission:** a minimal supported subset of `DrawPrimitive*`/`DrawIndexedPrimitive*` paths used by early boot/menu.

## Feasibility comparison (initial)
- **Level 1 (D3D8-compat behavior in native Linux/x86):** attractive for behavioral parity if a compatible backend/integration is viable in-process for ELF i386, but requires careful ABI/toolchain and runtime dependency validation (not assumed drop-in).
- **Level 2 (native backend behind `IRenderPipeline`):** architecturally aligned with preserving engine contracts, but high implementation scope/risk due to breadth of `IRenderPipeline` + `RP8` semantics. Best approached as a **narrow, incremental backend** driven by traced call usage from boot/menu rather than full upfront parity.

## Recommended next step after #4 PoC
1. Capture actual `IRenderPipeline` call traces/requirements from early boot (or instrument wrapper points) to define a concrete "M5-minimal" method/feature subset.
2. Decide per-subsystem: compat translation vs native implementation, based on traced usage and correctness risk.
3. Keep renderer work incremental behind existing interfaces (`IRenderPipeline`/buffer/texture contracts) with behavior tests against known scenes/configs.
