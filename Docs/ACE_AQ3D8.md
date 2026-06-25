# ACE-AQ3D8 Live Resize Flicker Fix / Resize Transaction Stabilization

ACE-AQ3D8 is a focused stability patch for the remaining resize flicker in the 3D Environment. It does not add new 3D features, object visuals, shaders, materials, terrain, model import, AQ-M16+ cognition work, Python runtime, Panda3D, or DearPyGui. Apparently the universe demanded another milestone because dragging a window border should not look like a dying fluorescent tube.

## Problem addressed

AQ3D7 fixed the worst hover/button flicker by moving to dirty rect invalidation and stable child HWND sync. The remaining flicker happened during live window resize. The resize path could receive a rapid stream of `WM_SIZE` events while the user dragged the top-level window. That stream could repeatedly update layout, resize D2D resources, sync the embedded child HWND, queue DX12 resize, and recreate renderer resources.

## Root cause addressed

Live resize is now treated as a transaction:

```text
WM_ENTERSIZEMOVE -> begin live resize transaction
WM_SIZE spam     -> store pending width/height only
WM_EXITSIZEMOVE -> apply final layout and child viewport sync once
RenderFrame      -> apply the final pending DX12 resize once
```

The key change is that live `WM_SIZE` events no longer move/resize the embedded DX12 child HWND or recreate the DX12 renderer repeatedly.

## UI transaction state

`AceShellUi` now tracks:

- `windowLiveResizeActive_`
- `pendingResizeAfterLiveDrag_`
- `pendingLiveResizeWidth_`
- `pendingLiveResizeHeight_`
- `gradientsDirty_`
- live resize diagnostic counters

During live resize, `handleResize()` records the latest size and returns without running the expensive layout/sync path.

## Stable child HWND sync

`syncAquariumEmbeddedViewportWindow()` is guarded during live resize. If a live resize is active, it only marks the embedded viewport sync as needed and returns. It does not call `Show`, `Hide`, `SetWindowPos`, or any child resize path.

## Embedded viewport resize suspension

`AceAquariumEmbeddedDx12Viewport` now exposes:

- `SetResizeApplySuspended(bool)`
- `IsResizeApplySuspended()`
- `HasPendingResize()`

When resize apply is suspended, `ApplyPendingResizeIfNeeded()` returns without recreating renderer resources. When the live resize transaction ends, suspension is lifted and the final pending resize is applied on the next controlled render frame.

## D2D gradients and paint churn

Gradient recreation is deferred during live resize. Existing gradient resources remain in use while the user drags the window border, and the gradients are rebuilt once the final layout is applied. This avoids unnecessary resource churn during resize spam.

## Optional viewport freeze

During live resize, the embedded DX12 viewport is effectively frozen. The parent UI records pending dimensions, but it does not render-frame/apply-resize the child surface until the transaction ends. A stable frozen viewport during a short drag is better than a flickering viewport. Shocking, but true.

## Diagnostics

The patch adds counters for:

- live resize enter/exit;
- deferred live resize size events;
- final live resize apply;
- renderer recreate during live resize;
- child move during live resize.

Expected behavior:

```text
rendererRecreateDuringLiveResizeCount = 0
childMoveDuringLiveResizeCount = 0
```

The final resize may recreate renderer resources once after `WM_EXITSIZEMOVE`.

## What is not implemented

- no new 3D features;
- no new camera/orbit/movement system;
- no object visual changes;
- no model import;
- no asset browser;
- no material editor;
- no terrain;
- no shader pack;
- no particles/post-processing;
- no AQ-M16+;
- no Python/Panda/DearPyGui.

## Validation

`Tools/validate_ace_aq3d8.ps1` performs static checks for resize transaction state, `WM_ENTERSIZEMOVE` / `WM_EXITSIZEMOVE`, child HWND sync guards, embedded viewport resize apply suspension, gradient deferral markers, and compatibility with AQ3D7/AQ3D0/AQUI0/AQCPP validators.

`Tools/AceAq3D8ResizeProbe.cpp` validates the headless live-resize transaction model: repeated `WM_SIZE` events during drag do not sync/move/resize/recreate the child viewport, and the final pending resize is applied once.

## Manual test

After building on Windows:

1. start the app;
2. enter Environment 3D;
3. verify hover and button press remain stable;
4. slow-resize the window;
5. fast-resize the window;
6. maximize and restore;
7. press Step after resize;
8. start Run after resize.

Report resize flicker honestly. The intended result is that live resize flicker is eliminated or clearly reduced, with at most a stable frozen viewport while dragging and one final resize update after release.

## Next milestone

A future milestone can add smoother visual resize feedback or a dedicated resizing overlay if needed. AQ3D8 intentionally stops at transaction stabilization.
