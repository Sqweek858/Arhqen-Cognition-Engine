# ACE-AQ3D7 Flicker Fix / Dirty Rects / Stable Child HWND

ACE-AQ3D7 is a focused paint/render-pipeline patch for the 3D Environment UI. It does not add new 3D features, camera systems, renderer paths, terrain, model import, editor tools, Python runtime, Panda3D, DearPyGui, or AQ-M16+ cognition work. The point is narrower and less glamorous, naturally: stop the UI from flickering when a human waves a mouse over a button.

## Problem addressed

The remaining AQ3D6 issue was flicker and lag caused by aggressive parent-window repaint behavior and child DX12 viewport management being too close to the D2D paint path. The suspect pattern was:

- full-window invalidation on hover and button state changes;
- classic Windows background erase behavior;
- parent D2D repaint touching child HWND regions;
- child viewport placement/visibility sync being driven by render-time code;
- direct renderer recreation during size messages.

## Root causes addressed

### Parent window class

The native parent window class no longer uses class-level redraw flags and no longer installs a classic background brush. `WM_ERASEBKGND` returns `1`, so Windows does not erase the background behind the custom D2D/DX12 UI before the engine paints.

### Dirty rect invalidation

`AceShellUi` now has a dirty-rect helper for hover/press repaint paths. Hover transitions invalidate only the old and new Aquarium hot control rects, inflated for glow. The dirty rect path uses `RedrawWindow` with `RDW_INVALIDATE | RDW_NOERASE | RDW_NOCHILDREN`, so parent UI hover does not invalidate the DX12 child HWND.

Full invalidation remains available for major mode/layout changes, but it also uses `RDW_NOERASE | RDW_NOCHILDREN`.

### Stable child HWND sync

The render path now publishes the desired DX12 viewport rectangle only. Actual child HWND visibility and placement are synchronized by `syncAquariumEmbeddedViewportWindow()` outside D2D paint. This prevents `WM_PAINT` from repeatedly managing the child viewport.

### Resize behavior

The embedded DX12 viewport queues pending resizes. `WM_SIZE` does not directly recreate renderer resources. The pending resize is applied once in the controlled render-frame path, and no renderer recreation happens during idle hover/click.

## Diagnostic counters

The patch adds or preserves counters for:

- parent paint count;
- full invalidation count;
- partial invalidation count;
- hover invalidation count;
- child sync count;
- child show/hide/move/resize count;
- renderer recreate count;
- viewport init/resize/frame count.

During hover-only interaction, partial/hover invalidation can increase, but full invalidation, child resize, and renderer recreate must not increase.

## What is not implemented

- no new 3D features;
- no camera/orbit/movement controls;
- no renderer rewrite;
- no shader/material/asset pipeline;
- no terrain;
- no model import;
- no AQ-M16+ cognition changes;
- no Python/Panda/DearPyGui dependency.

## Validation

`Tools/validate_ace_aq3d7.ps1` performs static checks for native window class settings, erase handling, dirty rect usage, child HWND sync separation, pending resize behavior, and legacy validator compatibility. `Tools/AceAq3D7FlickerProbe.cpp` validates the headless invalidation/counter model.

## Manual test

After building on Windows, test:

1. open Environment and enter the 3D viewport;
2. move mouse over controls without clicking;
3. click buttons and toggles;
4. toggle logs/details/truth;
5. resize the window;
6. step repeatedly;
7. run for a few seconds, then pause;
8. check if viewport flicker remains.

Expected result: hover and button press should no longer flicker the DX12 viewport. Resize may still produce normal swapchain-size transition artifacts, but renderer recreation should be queued and limited rather than spammed.
