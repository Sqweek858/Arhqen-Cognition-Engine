# ACE-AQ3D14 - Slate-style Offscreen Viewport Composite

ACE-AQ3D14 stops treating the Aquarium 3D viewport as a separate child HWND in the main 3D Environment path.

The previous approach used a D2D parent shell plus a child DX12 HWND with its own flip-model swapchain. Hover and button press flicker were reduced by dirty rects, but live resize remained unstable because the parent D2D render target and the child DX12/DWM surface could still race each other.

## Direction borrowed from Slate/SViewport

Unreal's Slate viewport architecture does not require a viewport to be a random child window fighting the UI parent. SViewport can be part of the UI tree and the viewport can render directly to the window backbuffer or to a separate render target that Slate later composites.

ACE-AQ3D14 adopts that architecture idea, not the UE code:

```text
Aquarium runtime + scene adapter
  -> retained viewport primitive model
  -> single-HWND D2D shell composition
  -> no child HWND in the main 3D path
```

## What changed

- The 3D Environment main path enables `aquariumUseSingleHwndCompositeViewport_`.
- `renderAquariumDx12ViewportSurface` routes to `renderAquariumSlateCompositeViewport`.
- The legacy embedded DX12 child HWND is hidden/suppressed if it still exists.
- Native resize freezing is disabled for the single-HWND composite path.
- During live resize, D2D continues to paint the viewport area normally.
- Step/Run/Reset update the same retained Aquarium scene model.

## Why this should fix resize flicker

The flicker source was the separate child HWND / swapchain interacting with DWM during parent resize. With the main path composited inside the parent D2D render target, there is no child HWND to move, hide, resize, or present during live resize.

## Current compromise

This milestone prioritizes stability. The viewport uses the same Aquarium render primitives and viewport projection model, but it is composed by the shell as a single-HWND viewport. The old child DX12 renderer remains available as legacy/debug code, but it is not the main 3D Environment path.

## Non-goals

- No new 3D visuals.
- No camera/orbit system.
- No AQ-M16+.
- No asset pipeline.
- No imported Slate code.
- No UE module dependency.

## Next milestone

If higher-fidelity GPU rendering is needed, build a true single-window render ownership path: either D2D/DirectComposition shared surface composition, or one renderer owning both viewport and UI. Do not return to child HWND resize hacks as the default path.
