# ACE-AQ3D6 - UI Render Stability + Layout Correction Pass

ACE-AQ3D6 stabilizes the 3D Environment UI after AQ3D5. AQ3D5 moved the interface in the right direction, but visible testing still showed hover/click flicker, tight topbar spacing, inconsistent left-panel spacing, and section overlap risk when panel sizes changed.

## Problem found in AQ3D5

The AQ3D5 layout still had several practical issues:

- section heights were too small for the controls they contained;
- Main Controls and Manual Actions could visually push into following sections;
- Debug Truth, Logs and X in the topbar were packed too tightly;
- hover movement over the modal still ran hover logic for the underlying chat shell;
- log scroll state did not preserve manual user scroll intent;
- child DX12 placement could still request unnecessary redraws during stable layout passes.

The visible symptom was a UI that looked better than AQ3D4, but still felt unstable and slightly cursed, as if the pixels had formed a labor union.

## Flicker cause identified

The main flicker and lag risks were:

- hover/click over Environment mode could still update unrelated shell hover state;
- D2D repaint was requested for too broad a set of mouse movement cases;
- the DX12 child window was repositioned with redraw flags that were too eager;
- section rects were deterministic, but some control rows exceeded their section heights;
- logs were recomputed without tracking whether the user had manually scrolled away from the bottom.

## Render/update loop changes

ACE-AQ3D6 adds a stable Aquarium-only hover path while the Environment modal is active. Mouse move in 3D mode now computes a deterministic Aquarium hot-id and invalidates only when that hot-id changes. Underlying chat shell controls no longer participate in hover state while the modal owns the pointer.

The DX12 child viewport remains persistent. Placement now avoids redraw-on-positioning flags, and renderer recreation remains limited to real size changes.

## Layout determinism

`AceEnvironment3DMode` now has centralized constants in `AceEnvironment3DLayoutConstants`:

- `TopbarHeight`
- `PanelInset`
- `SectionGap`
- `ButtonHeight`
- `ButtonGap`
- `RowGap`
- `ResizeHandleSize`
- min/max panel sizes
- section heights

The 3D layout pass computes explicit rects for:

- mode bounds;
- topbar clusters;
- viewport background;
- DX12 surface;
- left details panel;
- right logs panel;
- section rects;
- button rects;
- clip rects;
- resize handles.

No control is supposed to position itself independently from those rects.

## Topbar clusters

The topbar is split into three deterministic clusters:

- left: Details toggle;
- center: scenario, planner and step status;
- right: Debug Truth warning space, Truth toggle, Logs toggle and X.

Status and warning text are clipped to their assigned rects. Long text is clipped instead of pushing buttons around.

## Left panel layout

The left panel uses non-overlapping section rects:

- Runtime;
- Scenario / Planner;
- Main Controls;
- Manual Actions;
- Inspector.

Main Controls now has enough vertical space for Reset, Step, Run/Pause, Camera Reset and Debug Truth. Manual Actions now has enough vertical space for the full button grid, including Consume and Push, without overlapping Inspector.

## Right logs panel + scroll

The logs panel tracks:

- `offset`;
- `maxScroll`;
- `visibleLogStart`;
- `visibleLogEnd`;
- `userScrolled`;
- `autoScrollWhenAtBottom`.

New logs auto-scroll only when the user was already at the bottom. If the user manually scrolls upward, the panel preserves that range instead of snapping back down like a needy scrollbar.

## Clipping

Clipping is applied to:

- left panel content;
- individual static sections;
- right logs content;
- log line viewport;
- topbar status and debug warning text.

Sections may be clipped when the panel is resized too small, but they do not overlap neighboring sections.

## Resize handles

The left and right panels remain resizable from the corner only. Handles are placed outside the content clip area, and resizing clamps dimensions using centralized min/max constants.

Moveable panels are intentionally not implemented in this milestone.

## Resource persistence

The DX12 embedded viewport keeps the same renderer across idle frames. It does not reinitialize on hover, click, simple repaint, or layout recomputation. Resize recreation is limited to real width/height changes.

## Performance and lag fixes

This milestone reduces visible lag by:

- avoiding underlying shell hover processing while Environment mode is active;
- avoiding structural UI rebuilds on hover-only and click-only changes;
- using deterministic hit ids for Aquarium hover state;
- keeping layout rects stable across idle frames;
- preserving log scroll state instead of resetting visible range every paint;
- avoiding unnecessary child-window redraw during stable `SetWindowPos` calls.

## Debug Truth / privacy rules

When Debug Truth is off, agent-facing UI and the DX12 viewport must not expose object-kind labels such as WATER, ACID, FOOD, WALL, STONE, ICE, poison/medicine labels, or internal `ObjectKind` / `debug_truth` markers.

When Debug Truth is on, the UI may show debug overlays, but must clearly display:

```text
DEBUG TRUTH - NOT AGENT INPUT
```

Debug Truth remains a visualization/debug channel and is not input to the planner/world model decision logic.

## Not implemented yet

ACE-AQ3D6 intentionally does not add:

- moveable panels;
- docking sockets;
- advanced camera controls;
- full editor tools;
- model import;
- terrain;
- material graph;
- AQ-M16+ cognition features.

## Limitations

The mode is still a UI/viewport shell around the current Aquarium runtime. It is not a complete 3D editor, asset pipeline or terrain system. The current aim is stable, non-overlapping, non-flickering UI.

## Next milestone

The next reasonable milestone is a small interaction polish pass: keyboard shortcuts for the 3D mode, clearer focus state, and possibly collapsible inspector subsections. Moveable panels and docking should remain separate future work.
