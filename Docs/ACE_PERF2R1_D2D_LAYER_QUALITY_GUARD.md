# ACE-PERF2R1 - D2D Layer Quality Guard

## Scope

ACE-PERF2R1 corrects the PERF2 experiment. PERF2 proved that the Environment 3D viewport could be presented through a GPU-only DirectComposition path, but it did so by baking telemetry and the engine log console into the DX12 scene as blocky debug geometry. That removed the CPU readback path in the narrow case, but it downgraded the shipped UI: DWrite text, D2D glass panels, retained layout, scrollbars, input chrome, and the existing visual polish were bypassed.

PERF2R1 restores the production-quality UI rule:

```text
DX12 scene viewport -> parent-composited viewport element -> D2D/DWrite HUD and overlays above it
```

This is closer to the useful part of the UE/Slate model: the game viewport and UI are ordered layers. It does **not** mean reimplementing UI text as scene triangles.

## What changed

- The normal Environment 3D path no longer passes `AceAquariumGpuViewportOverlay` into the DX12 renderer.
- The PERF2 block-glyph telemetry/log overlay is disabled for the normal shell.
- `shouldUseDirectCompositionForAquariumViewport()` now acts as a quality gate and keeps the Environment 3D shell parent-composited until ACE owns a true DComp/D2D overlay visual.
- The D2D telemetry HUD and engine log console are again drawn by the existing D2D/DWrite renderer.
- `stat_rhi` and `stat_fps` now show:

```text
ui_layer=D2D_RETAINED_OVERLAY quality=preserved gpu_text_overlay=false
```

so it is obvious which visual path is active.

## Why not keep PERF2 as default?

PERF2 optimized the wrong boundary. It removed readback by moving UI into the scene renderer, but that destroyed the UI renderer contract. A good engine compositor keeps scene and UI as separate layers; it does not throw away the UI stack to win a synthetic no-readback counter.

## Non-goals

- No block-glyph GPU console as the default UI.
- No child HWND clipping or z-order hacks.
- No fake 1k FPS claims.
- No renderer rewrite into a full Slate clone.
- No new command spam; `stat_fps`, `stat_rhi`, and `stat_ui` remain the diagnostic surface.

## Validation

Run:

```powershell
.\Tools\validate_ace_perf2r1.ps1
.\Tools\validate_ace_perf2.ps1
.\Tools\validate_ace_ui12.ps1
.\Tools\validate_ace_perf1.ps1
.\Tools\validate_ace_perf0.ps1
.\Scripts\build_release.ps1
```

In the app:

1. Open Environment 3D.
2. Press backtick to open the engine log console.
3. Run `stat_rhi` and `stat_fps`.
4. Confirm the log console uses the normal D2D/DWrite visual style, not block glyphs.
5. Confirm the stats include `ui_layer=D2D_RETAINED_OVERLAY` and `gpu_text_overlay=false`.

## Expected diagnostic interpretation

If FPS is still low with this path, that is expected: the quality path still uses parent-composited readback because ACE does not yet have a real DComp/D2D transparent UI visual over the scene visual. The next serious architecture step is not to bake UI into the scene again. It is:

```text
DComp root visual
  scene visual: DX12 scene swapchain/texture
  UI visual: D2D/DWrite transparent swapchain/surface
```

That is the actual AAA-shaped direction. PERF2R1 deliberately restores visual quality before that deeper compositor refactor.
