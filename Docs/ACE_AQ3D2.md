# ACE-AQ3D2

## Scope

`ACE-AQ3D2 = Full Environment Workspace + Stable Embedded DX12 Viewport`

AQ3D2 replaces the cramped AQ3D1 modal/card approach with a dedicated Environment workspace. The viewport is now the main surface, not a small card inside a scrolling control panel.

## Problems fixed from AQ3D1

AQ3D1 had several unacceptable issues:

- the DX12 viewport could become tiny
- the Environment UI still behaved like a modal/control panel
- incorrect text such as `Embedded Embedded DX12 viewport failed to initialize`
- incorrect wording around a separate DX12 surface
- flicker risk from moving/resizing/recreating the child surface too often

## New solution

Environment now opens a full workspace inside the main shell:

```text
Main Window
  -> Environment Workspace
      -> top toolbar
      -> large embedded DX12 viewport
      -> right inspector
      -> bottom logs/timeline
```

The `Environment` button enters the workspace and enables the viewport automatically.

The toolbar button:

```text
Open 3D Environment
```

shows/focuses the embedded viewport region if it was hidden.

## Architecture

```text
AceAquariumRuntimeController
  -> AceAquariumSceneAdapter
      -> AceAquariumEmbeddedDx12Viewport
          -> Environment Workspace
          -> child HWND
          -> Dx12Renderer
```

Layout is computed by:

```text
AceEnvironmentWorkspace
```

## Embedded DX12 viewport

The embedded viewport uses a child `HWND` inside the main app window and renders through `Dx12Renderer`.

Rendering remains minimal but real DX12:

- grid/floor
- tiles / object blocks
- agent marker
- agent direction
- front-cell highlight
- debug strip when Debug Truth is enabled

The current geometry path still uses `UiDrawList` rectangle primitives. A future milestone should replace this with proper mesh/line/triangle batches.

## Flicker prevention

AQ3D2 adds persistence markers and behavior:

- viewport child surface persists
- `MoveWindow` is called only when the viewport rect actually changes
- renderer resources are recreated only when the embedded viewport size changes
- `InitCount`, `ResizeCount`, and `FrameCount` are exposed for diagnostics/validation
- render frame does not call `RecreateRenderer`

This avoids the obvious flicker path where the viewport destroys/recreates DX12 resources every frame. Humanity survives another frame, somehow.

## Viewport size

The workspace layout targets a large viewport:

```text
desktop: width >= 800 px, height >= 450 px
smaller windows: width >= 60% client, height >= 55% client
```

The AQ3D2 probe validates the computed 1440x860 layout and a resized 1280x760 layout.

## Debug Truth / Privacy

When Debug Truth is OFF:

- no ObjectKind labels are emitted
- labels such as WATER / ACID / FOOD / WALL / STONE / ICE are not exposed
- visual primitives remain generic/sensory/effect-based

When Debug Truth is ON:

- debug label primitives may be emitted
- the UI shows:

```text
DEBUG TRUTH - NOT AGENT INPUT
```

- the DX12 viewport uses a visible debug strip/color state

Debug Truth is not used by planner/world-model/core logic.

## Build artifacts

AQ3D2 keeps probe/build outputs under `Build/`.

The validator checks that the repository root contains no:

```text
.obj
.exe
.pdb
```

## Validation

Run:

```powershell
.\Tools\validate_ace_aqcpp1.ps1
.\Tools\validate_ace_aqcpp2.ps1
.\Tools\validate_ace_aqcpp3.ps1
.\Tools\validate_ace_aqcpp4.ps1
.\Tools\validate_ace_aqui0.ps1
.\Tools\validate_ace_ui1r9.ps1
.\Tools\validate_ace_aq3d2.ps1
.\Scripts\build_debug.ps1
```

## Current limitations

Not implemented yet:

- full mesh pipeline
- depth buffer
- camera input/orbit/pan/zoom
- material graph
- terrain
- model import
- asset browser
- editor tools
- AQ-M16+

## Next milestone

```text
ACE-AQ3D3 = Mesh Primitive Pipeline + Camera Input
```

Recommended focus:

- real line/triangle mesh batches
- camera zoom/pan/orbit
- debug overlay text inside DX12 viewport
- selected-cell inspector bridge
