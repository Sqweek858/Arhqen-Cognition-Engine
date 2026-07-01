# ACE-PERF2 / ACE-GPUCOMPOSE0 - GPU Composited Viewport


> PERF2R1 corrective note: the GPU-composited block-glyph overlay remains documented as an experiment, but it is no longer the normal Environment 3D shell path. The default path preserves D2D/DWrite UI with `ui_layer=D2D_RETAINED_OVERLAY` until ACE has a real DComp/D2D overlay visual.

## Scope

ACE-PERF2 moves the Environment 3D viewport toward the same architectural idea used by UE/Slate: the viewport is a render layer, and UI that belongs to that viewport is composed above the scene without routing the frame through CPU readback first.

This is not a full Slate renderer clone. It is the smallest ACE-native step that removes the normal `DX12_COMBINED_READBACK` path when the Environment 3D viewport is visible and only viewport-local overlays are needed.

## UE/Slate model copied architecturally

The UE source path used as reference is the viewport-as-draw-element model around `SViewport`, `ISlateViewport`, and `FSlateDrawElement::MakeViewport`. The important rule is architectural, not textual:

```text
scene viewport render target -> viewport draw/layer -> HUD/widgets/overlays above it -> final composed window
```

ACE-PERF2 applies the same rule in the current engine constraints:

```text
DX12 SceneColor + SceneDepth
-> GPU overlay pass for viewport-local HUD/log console
-> one DirectComposition GPU present command list
-> D2D parent chrome remains outside the viewport layer
```

The key difference from PERF1R1 is that the normal path does **not** copy the frame back to CPU just so D2D can put it back on the GPU again, because that was not rendering, that was a scenic tour through latency hell.

## New normal path

`DX12_GPU_COMPOSITED` is the normal Environment 3D mode when only viewport-local UI overlays are active.

It bakes the viewport HUD/log console into the GPU SceneColor texture using a second lightweight overlay pass, then presents that texture through DirectComposition.

Expected indicators:

```text
stat_rhi: path=DX12_GPU_COMPOSITED
stat_rhi: gpuCompositedFrames > 0
stat_rhi: combinedGpuCompositionFrames > 0
stat_rhi: gpuOverlayBakedFrames > 0 when log/telemetry exists
stat_fps: readback=false
```

The top-left viewport label should mention:

```text
GPU-composited no-readback overlay
```

## Fallback path

`DX12_COMBINED_READBACK` remains as a fallback when the viewport cannot be presented through DirectComposition or when a global parent overlay that is not baked into the viewport texture must be above the scene.

Fallback indicators:

```text
stat_rhi: path=DX12_COMBINED_READBACK
stat_fps: readback=true
```

Fallback is allowed. It should not be the normal active path after ACE-PERF2 unless something external blocks the GPU-composited path.

## GPU overlay pass

The overlay pass is intentionally simple:

- translucent panels are quads in viewport-local pixels
- telemetry bars are quads
- engine log text uses a tiny built-in block glyph atlas generated as geometry
- no CPU readback is required for the overlay itself

This is not final typography. It is a staging layer so ACE can keep the performance-critical path on the GPU while the real UI text/brush system evolves later. Human civilization has survived worse fonts.

## One-command GPU composition

PERF1R1 still rendered the scene in one command list and copied SceneColor to the DirectComposition swapchain in a second command list. PERF2 adds a combined submit/present API:

```text
submitAndPresentBgra8ToComposition(...)
```

This records scene render, overlay render, and composition swapchain copy into one GPU command list before Present/Commit. It avoids a redundant CPU/GPU fence wait between scene render and viewport present.

## Non-goals

- no full UE/Slate renderer port
- no material system
- no terrain/lighting/particles/asset browser
- no GPU timestamp query system yet
- no global UI GPU compositor yet
- no promise of 1k FPS on every machine

## Validation

Run:

```powershell
.\Tools\validate_ace_perf2.ps1
.\Tools\validate_ace_perf1.ps1
.\Tools\validate_ace_ui12.ps1
.\Tools\validate_ace_perf0.ps1
.\Scripts\build_release.ps1
```

Then in the app:

```text
stat_rhi
stat_fps
```

Healthy normal output should show:

```text
path=DX12_GPU_COMPOSITED
readback=false
gpuCompositedFrames increasing
combinedGpuCompositionFrames increasing
```

If `path=DX12_COMBINED_READBACK` still dominates, the remaining problem is not vertex upload or mesh complexity. It means DirectComposition presentation is unavailable, failing, or a global parent overlay is forcing fallback.
