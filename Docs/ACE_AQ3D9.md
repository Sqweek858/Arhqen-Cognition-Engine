# ACE-AQ3D9 Resize-Safe Viewport Proxy / Hide Child HWND During Live Resize

ACE-AQ3D9 fixes the remaining live-resize flicker in the 3D Environment mode.
AQ3D7 made hover and button press stable, and AQ3D8 introduced a resize
transaction, but the child DX12 HWND could still remain visible while the parent
D2D window was actively resizing. That left the flip swapchain fighting the
parent paint path during border drag, because apparently one window was not
sufficient suffering for Windows.

## Root cause addressed

During live resize, the parent window receives rapid `WM_SIZE` messages. AQ3D8
already prevented aggressive child resize/recreate work, but the child DX12 HWND
could still be visible while the parent D2D render target changed size. The fix
is to hide the child viewport during the live transaction and let the D2D parent
paint a stable proxy.

## Resize transaction strategy

`WM_ENTERSIZEMOVE` now starts a live resize transaction:

- `windowLiveResizeActive_` becomes true;
- embedded viewport resize application is suspended;
- if the DX12 child viewport was visible, it is hidden once;
- `aquariumViewportHiddenForLiveResize_` tracks that the proxy should be painted;
- the renderer/controller/core are not destroyed or reset.

During `WM_SIZE` while live resize is active:

- the pending width/height are updated;
- the D2D render target and layout still update to the new parent size;
- expensive gradients are deferred by `gradientsDirty_`;
- child HWND sync is skipped;
- DX12 `RenderFrame` is skipped;
- renderer recreate is not performed.

`WM_EXITSIZEMOVE` ends the transaction:

- resize suspension is cleared;
- final pending size is applied once;
- child viewport sync is requested;
- the D2D proxy remains visible until the child HWND is shown at the final rect;
- normal DX12 rendering resumes after the final sync.

## D2D viewport proxy

During live resize, `renderAquariumResizeProxyViewport` draws a stable D2D
replacement for the viewport rect:

- dark background;
- simple grid;
- small agent marker/direction line;
- `Resizing viewport...` text.

This proxy is intentionally simple. It is not a new renderer and not a fake 3D
path for normal operation. It exists only while live resizing or until the child
DX12 HWND has been restored after resize.

## Counters

AQ3D9 adds diagnostic counters:

- `viewportHideForLiveResizeCount_`;
- `viewportShowAfterLiveResizeCount_`;
- `liveResizeProxyPaintCount_`;
- `d2dResizeDuringLiveResizeCount_`.

Expected behavior during one drag-resize:

- child hide count grows once;
- proxy paint count grows while dragging;
- D2D resize count grows while dragging;
- renderer recreate during live resize remains zero;
- child move during live resize remains zero;
- child show-after-resize grows once after the final sync.

## Non-goals

AQ3D9 does not add:

- new 3D visuals;
- camera controls;
- model import;
- terrain;
- material/shader pipeline;
- editor features;
- AQ-M16+ cognition work;
- Python/Panda/DearPyGui dependencies.

## Manual test focus

After build, test:

- slow border resize;
- fast border resize;
- maximize/restore;
- hover and button press after resize;
- Step and Run after resize.

The intended visible behavior is that the DX12 child viewport disappears during
live resize, the parent draws the stable D2D proxy, and the DX12 viewport returns
after the resize completes.

## Next milestone

Next work can add better resize diagnostics or camera controls, but AQ3D9 itself
is strictly a resize flicker stabilization patch.
