# ACE-AQ3D13 - Frozen Native Resize Commit

## Problem

AQ3D7-AQ3D12 reduced hover/button flicker and tried increasingly hard live-resize shields, but the remaining flicker persisted specifically while the top-level window border was being dragged in 3D Environment mode.

The stubborn part is architectural: the app uses a D2D parent window and an embedded DX12 child HWND/swapchain. During Win32 live resize, USER32/DWM can repeatedly expose, resize, clip, and composite these surfaces while the app is inside the modal sizing loop.

## UE-informed direction

Unreal's Slate viewport path is not equivalent to a D2D parent plus a separate child DX12 HWND. SViewport participates in the Slate widget tree and can either render to the window backbuffer or use a separate render target that Slate then paints. SceneViewport also hooks pre/post backbuffer resize notifications so viewport resources are released/recreated under renderer control.

ACE does not yet have that integrated single-HWND render architecture. AQ3D13 is therefore a hard stabilization patch, not the final architecture.

## Strategy

In 3D Environment mode only:

1. Start a live resize transaction on border sizing.
2. Store the current top-level window RECT as the frozen RECT.
3. During WM_SIZING, save the proposed final RECT but return the frozen RECT to USER32.
4. The actual client area does not continuously change while the user drags the border.
5. On WM_EXITSIZEMOVE, commit the final RECT once with SetWindowPos.
6. D2D/DX12 resize and final viewport restore then happen once.

This intentionally trades continuous live-resize feedback for flicker stability in the 3D viewport. Normal shell/control-panel resize behavior is not frozen.

## Non-goals

- No new 3D features.
- No cognition/core changes.
- No shader/material/asset pipeline.
- No camera/orbit work.
- No AQ-M16+.
- No Python/Panda/DearPyGui.
- No copied Unreal source code.

## Next real architecture

The proper long-term fix is ACE-AQ3D14 or similar:

- remove the embedded DX12 child HWND for the viewport;
- use a single top-level render ownership path;
- either render UI and 3D into one swapchain or render the 3D viewport into a texture consumed by the UI compositor.

AQ3D13 is the last pragmatic Win32 live-resize workaround before that architecture change.
