# ACE-UI1R3

## Scope

`ACE-UI1R3 = Proper Centered Dashboard Layout + Real Scroll Hit Zones`

This patch fixes the actual UX issues visible after UI1R2.

## Fixed

### Centered dashboard

Environment panel is centered in the app viewport with stable dashboard dimensions.

It no longer depends on sidebar-relative positioning.

### Clean top controls

Controls are arranged as a clean two-row command strip:

Row 1:

- scenario previous / next
- planner previous / next
- reset
- step
- run/pause
- debug truth

Row 2:

- forward
- turn left
- turn right
- wait
- touch
- consume
- push

The scenario/planner text sits between these rows and the dashboard content, not on top of buttons.

### Dashboard content starts below controls

The card grid begins below both control rows.

No control/button/text overlap is allowed.

### Metrics no longer gets a decorative scrollbar

Metrics is static again. Scrollbars stay on long sections only:

- Logs / Episodes
- Debug Truth
- Counterfactual

### Scroll hit zones

Scroll hit-testing now prioritizes visible thumb/track/viewport and ignores sections without overflow.

## Non-goals

No real blur shader pass.

No 3D viewport.

No DX12 Aquarium renderer.

No Python bridge.

No generated images.

## Validation

Run:

```powershell
.\Tools\validate_ace_ui1r3.ps1
.\Scripts\build_debug.ps1
```
