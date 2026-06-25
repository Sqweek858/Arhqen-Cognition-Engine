# ACE-AQ3D4 - 3D Environment UI Layout Rework

ACE-AQ3D4 repairs the AQ3D3 layout. AQ3D3 moved the renderer into a near full-screen mode, but the UI still behaved like a debug toolbar dumped over a viewport. This milestone turns the Environment workspace into a cleaner tool surface.

## Environment Control Panel

The Control Panel is now a real control surface instead of a tutorial screen.

Implemented structure:

- title row with a compact `X` close button in the top-right;
- `Runtime` summary card;
- `Scenario / Planner` card with compact previous/next controls;
- `Main Actions` with `Enter 3D Environment >`, `Reset`, `Step`, and `Run/Pause`;
- `Manual Actions` in an ordered row;
- `Logs / Episodes` at the bottom with the existing clipped scroll path.

Removed from the Control Panel:

- `Flow` section;
- tutorial copy explaining the launch path;
- the large Back button.

## 3D Environment Mode

The 3D mode now uses a thinner operational layout:

- a 24 px topbar;
- left `Details / Controls` panel;
- right `Logs / Episodes` panel;
- dominant DX12 viewport area between the panels;
- no manual actions in the topbar;
- no giant two-row command strip.

Topbar contents:

- `Details` show/hide toggle;
- centered `scenario | planner | step` status;
- compact `Debug Truth` toggle/indicator;
- `Logs` show/hide toggle;
- `X` to exit 3D mode and return to the Control Panel.

## Left details/control panel

The left panel contains:

- runtime state;
- scenario/planner controls;
- reset/step/run/camera/debug controls;
- manual actions;
- inspector headings for body, observation, planner trace, counterfactual, self model, delayed/dynamic, and metrics.

The panel is intentionally not moveable yet. It is a fixed side panel with runtime-resizable dimensions.

## Right logs panel

The right panel contains `Logs / Episodes` and uses the real `renderAquariumScrollableLines` scroll state. The scrollbar changes the visible range by modifying the line offset and clipping the content viewport.

## Corner resize

AQ3D4 adds corner resize handles:

- left panel: bottom-right handle;
- right logs panel: bottom-left handle.

Resize behavior:

- mouse drag changes width and height;
- dimensions are stored in runtime UI state while the app runs;
- min/max clamps are enforced by `AceEnvironment3DMode::ClampPanelState`;
- moveable panels are intentionally not implemented.

## Viewport behavior

The DX12 viewport remains the main working surface. It is not a small card and does not use a separate window path. The side panels are compact controls/logs, while the viewport remains the central environment surface.

## Flicker/stability

AQ3D4 keeps the AQ3D3 embedded DX12 stability rules:

- persistent child DX12 surface;
- renderer/resources are not recreated every frame;
- resize happens only on real size changes;
- init/resize/frame counters remain available.

## Debug Truth and privacy

When Debug Truth is off, agent-facing overlays do not expose hidden object-kind labels. When Debug Truth is on, the UI displays:

```text
DEBUG TRUTH - NOT AGENT INPUT
```

Debug Truth remains a visualization/debug layer and is not used as planner/world-model input.

## Not implemented yet

Intentionally not included in AQ3D4:

- moveable panels;
- docking sockets;
- model import;
- terrain;
- material graph;
- asset browser;
- full 3D editor;
- AQ-M16+ cognition work.

## Next milestone

The next UI milestone should probably add optional panel collapse persistence and cleaner inspector content binding. Docking/moveable panels should wait until the resize state is stable under real usage.
