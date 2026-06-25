# ACE-AQ3D5 Layout Determinism + Flicker Fix + Panel Clipping

ACE-AQ3D5 stabilizes the 3D Environment UI layout, clipping, panel resizing, logs scrolling, and viewport flicker behavior.

## Problem fixed from AQ3D4

AQ3D4 moved the product in the right direction, but its 3D mode layout could still look unstable:

- topbar clusters were too close together;
- left panel controls could overlap when the panel was narrow;
- Manual Actions could collide with Inspector;
- resize handles could sit on top of content;
- logs had scrolling logic, but the visible range and clip discipline needed hardening;
- the shell invalidated every tick, which could make the D2D overlay and hosted DX12 viewport fight each other in idle.

## Deterministic layout pass

`AceEnvironment3DMode::Compute` is now the single layout pass for 3D mode. It computes stable rects for:

- mode bounds;
- thin topbar;
- left, center, and right topbar clusters;
- status and Debug Truth warning text;
- left details panel;
- right logs panel;
- panel title rects;
- content clip rects;
- section rects;
- all control button rects;
- resize handle rects;
- dominant viewport rect;
- hosted DX12 child surface rect.

The shell now consumes those rects instead of scattering magic numbers through rendering code. Tiny miracle, really: rectangles now know where they live.

## Panel clipping

The left panel uses `leftContentClip`, and the logs panel uses `rightLogsContent` plus the scroll viewport. The shell pushes D2D clips before rendering panel content, so text and controls do not draw into resize handles or outside panel content.

## Topbar clusters

The topbar is split into three non-overlapping clusters:

- left: Details toggle;
- center: scenario | planner | step;
- right: Debug Truth warning, Truth toggle, Logs toggle, X.

Long status/warning text is clipped to its own rect instead of pushing neighboring controls.

## Left panel layout

The left Details / Controls panel is laid out in deterministic sections:

1. Runtime
2. Scenario / Planner
3. Main Controls
4. Manual Actions
5. Inspector

Manual Actions use a stable button grid:

```text
[Forward]
[Turn L]   [Turn R]
[Wait]     [Touch]
[Consume]  [Push]
```

The Inspector starts below Manual Actions. The resize handle is outside the content clip.

## Right logs panel

The right Logs / Episodes panel keeps:

- a real scroll offset;
- max scroll clamping;
- a visible thumb only when content exceeds the viewport;
- clipped log text.

The probe validates that changing scroll offset changes the visible range and that large offsets clamp.

## Resize handles

Panel resize handles remain corner-based:

- left panel: bottom-right corner;
- right logs panel: bottom-left corner.

They change panel width and height through `ResizeLeftPanel` / `ResizeRightPanel`, with min/max clamps. Panels are intentionally not moveable yet.

## Flicker fix

The shell no longer invalidates every idle tick while 3D mode is active and paused. The hosted DX12 child surface renders independently, and the D2D overlay only invalidates for real UI/runtime changes.

The embedded DX12 child window also avoids class-level horizontal/vertical redraw invalidations and uses stable child-window placement instead of forcing redraw through `MoveWindow` every layout pass.

Counters and state remain testable:

- `init_count` should not increase in idle;
- `resize_count` should not increase without real size changes;
- `frame_count` may increase as the viewport renders;
- `layout_pass_count` is exposed by `AceEnvironment3DMode`;
- `last_viewport_rect` is exposed for stable rect checks.

## Viewport/resource persistence

The viewport remains the dominant background. The hosted DX12 child surface is kept away from child-window-hosted panels so Win32 z-order does not cover the D2D controls. Resources are not recreated per frame; resize happens only on real size changes.

## Privacy / Debug Truth

When Debug Truth is off, agent-facing viewport/UI overlays must not expose object-kind labels. When Debug Truth is on, the topbar displays:

```text
DEBUG TRUTH - NOT AGENT INPUT
```

Debug Truth remains a visualization/debug overlay and is not fed into the planner/world model as agent input.

## Not implemented in AQ3D5

- moveable panels;
- docking sockets;
- advanced camera controls;
- full editor mode;
- model import;
- terrain;
- material graph;
- AQ-M16+ cognition features.

## Current limitations

The renderer is still a simple Aquarium visualization, not a full 3D editor. AQ3D5 is intentionally a stability/layout patch, not a feature expansion.

## Next milestone

Recommended next milestone: camera interaction and viewport input polish, after layout and flicker are confirmed stable in the real Windows shell.
