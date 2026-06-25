# ACE-UI1R4

## Scope

`ACE-UI1R4 = Header Spacing + Grouped Control Tabs`

This patch fixes the visible top-area spacing problems in the Environment / Aquarium control panel.

## Fixed

### Header subtitle spacing

The subtitle is moved below the header divider with real breathing room.

It no longer sits on the border line.

### Grouped controls

The control strip is no longer a cramped row of loose buttons.

Controls are grouped into glass cards:

- Scenario
- Planner
- Runtime
- Manual Actions

Each group has its own title and internal spacing.

### Dashboard start position

Dashboard cards now start below the grouped control section.

The old overlapping scenario/planner text row was removed.

## Non-goals

No 3D viewport.

No DX12 Aquarium renderer.

No Python bridge.

No generated images.

## Validation

Run:

```powershell
.\Tools\validate_ace_ui1r4.ps1
.\Scripts\build_debug.ps1
```
