# ACE-UI1R8

## Scope

`ACE-UI1R8 = Log End Padding + Scrolled Content Bottom Fix`

This patch fixes the missing visible end of the Logs / Episodes panel after ACE-UI1R7.

## Problem

ACE-UI1R7 correctly introduced a clipped content viewport, but the content bottom was still calculated from the fixed viewport top:

```cpp
contentBottom = viewportTop + contentHeight;
```

The content top moved with scroll, but the content bottom did not.

That meant the Logs card could visually lose its ending / bottom edge while scrolling.

## Fix

The content bottom now moves with the content top:

```cpp
contentBottom = contentTop + contentHeight;
```

Also added a small `logEndPadding` and an 8px bottom inset for the Logs rect so the final edge/border can be seen before the viewport clips it.

## Non-goals

No 3D viewport.

No DX12 Aquarium renderer.

No Python bridge.

No generated images.

## Validation

Run:

```powershell
.\Tools\validate_ace_ui1r8.ps1
.\Scripts\build_debug.ps1
```
