# ACE-UI1R7

## Scope

`ACE-UI1R7 = Environment Content Clip + Container Scrollbar`

This patch replaces the previous attempts to squeeze all cards into the Environment panel.

## Implemented

### One clipped content viewport

Controls/header remain fixed.

Everything below Manual Actions is rendered inside:

```text
aquariumContentScroll_.viewport
```

The viewport uses:

```cpp
PushAxisAlignedClip(...)
PopAxisAlignedClip()
```

### One container scrollbar

The Environment content area now has one parent scrollbar:

```text
aquariumContentScroll_
```

This scrolls the whole dashboard/log flow.

### Logs stay in the content flow

Logs / Episodes is no longer positioned as a fragile bottom hack.

It is part of the scrollable content flow and is clipped by the parent viewport.

### Child scroll conflict removed

Logs / Debug / Counterfactual no longer fight the parent scroll system.

The parent content scrollbar is authoritative.

## Non-goals

No 3D viewport.

No DX12 Aquarium renderer.

No Python bridge.

No generated images.

## Validation

Run:

```powershell
.\Tools\validate_ace_ui1r7.ps1
.\Scripts\build_debug.ps1
```
