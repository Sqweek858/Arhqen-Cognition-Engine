# ACE-AQ3D3 Full-Screen 3D Environment Mode

`ACE-AQ3D3 = Full-Screen 3D Environment Mode`

This milestone turns the Aquarium environment into a focused product mode instead of a small viewport embedded inside a crowded workspace.

## Product direction

The main flow is now:

```text
Environment -> Environment Control Panel -> Enter 3D Environment -> 3D Environment Mode -> Back / Exit 3D
```

The Environment button opens a control panel. The control panel has a visible `Enter 3D Environment` button. Pressing it switches the same main window into a dominant DX12 Environment Mode. `Back / Exit 3D` returns to the control panel.

## Rejected directions

These are not acceptable as the main path:

- a tiny viewport card inside a crowded workspace;
- a separate window;
- a fake 2D render path dressed up as environment rendering;
- a placeholder that reports success while the renderer is not active.

AQ3D3 keeps the 3D environment in the main window and makes the DX12 viewport the dominant surface.

## Layout

The 3D mode layout is computed by `AceEnvironment3DMode`:

- compact top overlay with `Back / Exit 3D`, scenario/planner cycling, reset, step, run/pause, debug truth, camera reset, and manual actions;
- dominant DX12 viewport below the overlay;
- compact bottom log strip;
- inspector collapsed by default so the viewport keeps the space.

The viewport is required to occupy at least 70% of the mode area. The target layout is roughly 90% width and 80% height on normal desktop windows.

## Architecture

The intended chain is:

```text
AceAquariumRuntimeController
  -> AceAquariumSceneAdapter
      -> AceAquariumEmbeddedDx12Viewport
          -> AceEnvironment3DMode
```

Runtime state remains in `AceAquariumRuntimeController`. Scene primitive construction remains in `AceAquariumSceneAdapter`. The embedded child DX12 viewport renders those primitives. The 3D mode class only computes layout and area guarantees.

## DX12 rendering

`AceAquariumEmbeddedDx12Viewport` remains the DX12 path used by the environment mode. The child surface is shown only while full-screen 3D mode is active. It renders:

- floor/grid primitives;
- object/cell primitives;
- agent marker;
- agent direction arrow;
- front-cell highlight;
- debug warning only when Debug Truth is enabled.

AQ3D3 also scales rendered primitives with the viewport camera so the world fills the dominant surface instead of remaining small in the center.

## Flicker prevention

The viewport keeps persistent resources:

- `init_count` increments only when the renderer is initialized;
- `resize_count` increments only on real size changes;
- `frame_count` increments per rendered frame;
- the child surface is moved only when the rect changes;
- renderer resources are not recreated every frame.

The normal expected behavior is `init_count = 1` after the first successful entry into 3D mode, with resize count changing only on real viewport size changes.

## Debug Truth / privacy

When Debug Truth is off, the mode must not display object-kind labels as agent-facing UI. When Debug Truth is on, debug information is separated and marked with:

```text
DEBUG TRUTH - NOT AGENT INPUT
```

The Debug Truth toggle does not feed planner or world-model decisions.

## Limitations

This milestone does not add:

- model import;
- terrain editing;
- material graph;
- asset browser;
- physics engine;
- AQ-M16+ cognition work.

## Next milestone

A sensible next step is camera polish and optional inspector expansion that does not reduce the viewport below the 70% area requirement.
