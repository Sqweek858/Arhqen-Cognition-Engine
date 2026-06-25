# ACE-AQ3D12 - Owned Popup Resize Shield

AQ3D12 is a resize-only stabilization patch for the 3D Environment viewport.
It does not add 3D features, camera controls, asset import, terrain, material graphs, or AQ-M16+ cognition work.

## Root problem

AQ3D7 fixed hover/button flicker and AQ3D8-AQ3D10 tried to freeze, hide, and proxy the embedded DX12 child HWND during live resize. The remaining symptom points at the Windows compositor path: a D2D parent HWND, an embedded flip-model DX12 child HWND, and a live resize modal loop can still expose stale viewport pixels while the parent and child are fighting over the same screen region.

## UE source lesson

The Unreal source points away from a child-HWND viewport architecture. `SViewport` can render a viewport as part of the Slate draw tree through a viewport texture, and only skips drawing the quad when rendering directly to the backbuffer. `FSceneViewport` also tracks whether it uses a separate render target and responds to Slate renderer pre/post backbuffer resize callbacks. `FSlateRHIRenderer` owns a per-window viewport RHI and resizes it through `ResizeViewportIfNeeded`, with pre/post resize delegates around the RHI resize.

The clean long-term direction is therefore a single-window/integrated render path, not a child DX12 HWND under a D2D parent. AQ3D12 is still a smaller patch: it covers the resize flicker without rewriting the whole renderer.

## Strategy

During live resize:

- the embedded DX12 child HWND is still hidden/quarantined;
- a no-activate owned popup shield is positioned over the viewport in screen coordinates;
- the shield paints its own dark proxy/grid through GDI;
- the shield is armed as early as `WM_NCLBUTTONDOWN` on sizing borders, before the modal resize loop can expose the child area;
- D2D can continue layout/paint work underneath, but visible viewport flicker is covered by the shield;
- after `WM_EXITSIZEMOVE`, the DX12 child is restored and rendered once, then the shield is hidden.

## What this deliberately avoids

- no new 3D visuals;
- no renderer rewrite;
- no cognition changes;
- no Python/Panda/DearPyGui;
- no moveable panels/docking;
- no AQ-M16+.

## Limitations

The shield is a pragmatic Win32 workaround. If this still flickers, the next real milestone should be an integrated single-HWND render path where the 3D viewport is not a separate child HWND.
