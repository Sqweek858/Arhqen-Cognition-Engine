# ACE-AQ3D10 Hard Resize Flicker Fix

ACE-AQ3D10 addresses the remaining resize-only flicker after AQ3D9.
AQ3D9 hid the embedded DX12 child HWND during live resize, but the top-level
window was still using child clipping and the live resize repaint was not forced
synchronously. That left brief compositor gaps/stale child regions visible while
the user dragged the window border.

## Root cause treated

The likely remaining causes were:

- the top-level shell used `WS_CLIPCHILDREN`, so the parent D2D render target could
  fail to paint the viewport rectangle cleanly while the child viewport was hidden;
- `ShowWindow(SW_HIDE)` could leave the child/swapchain region visually active for
  one compositor beat during live resize;
- live resize relied on a later invalidated paint instead of forcing a no-erase
  repaint immediately after each resize message;
- some resize paths may deliver `WM_SIZING` before the UI has entered its explicit
  `WM_ENTERSIZEMOVE` transaction.

## Fixes

- The top-level native window no longer uses `WS_CLIPCHILDREN`.
- The D2D HWND render target uses `D2D1_PRESENT_OPTIONS_RETAIN_CONTENTS`.
- `WM_SIZING` arms the same live resize transaction used by `WM_ENTERSIZEMOVE`.
- The embedded DX12 child viewport has `HideForLiveResize()`, which hides and moves
  the child off-screen without destroying its renderer.
- During live resize, the parent D2D target still resizes and paints the stable
  viewport proxy.
- After each live resize layout update, the parent forces a synchronous no-erase
  repaint using `RDW_UPDATENOW`.
- At resize exit, the final proxy repaint happens first, then the embedded child
  viewport is synchronized to the final rect and rendered once.

## Non-goals

No new 3D features, camera controls, asset/model import, materials, terrain,
AQ-M16+, Python bridge, Panda3D, DearPyGui, or cognition changes.

## Manual validation

Test in Windows:

1. Launch the app.
2. Enter Environment 3D.
3. Drag-resize slowly.
4. Drag-resize quickly.
5. Maximize/restore.
6. Confirm the D2D proxy remains stable during drag.
7. Confirm the DX12 child returns after resize.
8. Step and Run after resize.

If flicker remains, identify whether it appears in the D2D proxy, on child restore,
or only during maximize/restore.
