# ACE-UI3F: D2D Pixel-Space DPI Correction

## Problem

ACE-UI3 added per-monitor DPI awareness and monitor geometry helpers. That part was useful, but the D2D `HwndRenderTarget` was still created with automatic DPI (`0, 0`). On a high-DPI monitor, Direct2D interpreted Arhqen's existing layout coordinates as DIPs instead of the pixel-space coordinates used by the shell.

The visible result was a comically broken layout: panels shifted and scaled as if the UI had been multiplied by the monitor DPI behind our back. Very helpful, Direct2D, nobody asked.

## Fix

The shell now keeps the main D2D render target explicitly pinned to 96 DPI:

```cpp
D2D1::RenderTargetProperties(..., 96.0f, 96.0f)
renderTarget_->SetDpi(96.0f, 96.0f)
```

This preserves the current pixel-space Arhqen layout while keeping ACE-UI3's monitor/DPI data available for diagnostics, popup placement, future scaling, and multi-monitor geometry.

## Rules

- Do not let `ID2D1HwndRenderTarget` auto-scale the existing shell coordinates.
- Keep monitor DPI capture for placement and future scale-aware widgets.
- Apply the 96-DPI target pin after creation and after `Resize()`.
- Do not remove ACE-UI3/ACE-UI4 systems.

## Future work

A later UI milestone can introduce a real logical-unit layout system. Until then, the current shell is pixel-space and D2D must be forced to behave like it.
