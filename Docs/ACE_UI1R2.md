# ACE-UI1R2

## Scope

`ACE-UI1R2 = Actual Layout Repair + Working Scrollbars`

This patch fixes the real problems left by ACE-UI1/ACE-UI1R1.

## Fixed

### Environment is no longer a small modal

The Environment panel now uses a real workspace-sized overlay area instead of copying the Settings modal rectangle.

It is placed relative to the sidebar and topbar:

- left of panel starts after the sidebar
- top starts below the app topbar
- bottom stays inside the window
- logs are kept inside the Environment panel

### Chat layer hidden while Environment is open

While Environment is open, the normal chat/home layer is not rendered:

- no empty-state hero text behind the panel
- no command input bar overlapping Logs / Episodes
- no send circle bleeding over the modal
- no autocomplete over the modal

### Controls are visible and clickable

Aquarium control button rectangles are computed from the actual Environment panel during render:

- scenario previous/next
- planner previous/next
- reset
- step
- run/pause
- debug
- manual actions

This fixes the previous invisible/zero-size hit rect problem.

### Fake vignette bars removed

The previous edge vignette rectangles looked like broken black bars. They were removed.

The overlay is now a clean focus layer, and panel depth comes from the modal glass/glow/veil.

### Scrollbars

Scroll infrastructure remains on long sections:

- Logs / Episodes
- Debug Truth
- Counterfactual
- Metrics

Mouse wheel and thumb drag hit testing remain tied to the visible panel bounds.

## Non-goals

No 3D viewport.

No DX12 Aquarium renderer.

No Python bridge.

No generated images.

## Validation

Run:

```powershell
.\Tools\validate_ace_ui1r2.ps1
.\Scripts\build_debug.ps1
```
