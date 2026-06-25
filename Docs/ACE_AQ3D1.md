# ACE-AQ3D1

## Scope

`ACE-AQ3D1 = Embedded DX12 Environment Viewport in Main Shell`

This milestone replaces the AQ3D0R separate viewport window direction with an embedded DX12 child surface inside the main Arhqen Cognition Engine shell.

## Problem fixed

AQ3D0R opened a separate top-level window:

```text
Arhqen Cognition Engine - DX12 3D Environment
```

That was not the intended final direction. It also split the user's workflow between the Environment Control Panel and a second window.

AQ3D1 removes that main path and embeds the DX12 viewport directly in the Environment panel.

## New solution

The Environment panel now contains a large `DX12 3D Environment` region.

The control button:

```text
Open 3D Environment
```

shows/focuses the embedded viewport region. It does not open a top-level window.

Technical path:

```text
AceAquariumRuntimeController
  -> AceAquariumSceneAdapter
      -> AceAquariumEmbeddedDx12Viewport
          -> child HWND inside main shell
          -> Dx12Renderer
```

## Embedded rendering path

AQ3D1 uses a DX12 child surface integrated into the main window.

Accepted technical model:

```text
Main Arhqen Cognition Engine Window
  -> Environment Panel
      -> Embedded child HWND viewport region
          -> own DX12 swap chain through Dx12Renderer
```

This is not a D2D/isometric fallback and not a placeholder.

## Visible elements

The embedded DX12 viewport renders:

- grid/floor
- tile/object primitives
- agent marker
- agent direction
- front-cell highlight
- debug strip/color state when Debug Truth is enabled

The geometry is intentionally minimal. It still uses the existing `UiDrawList` rectangle primitive path inside `Dx12Renderer`, so this milestone is not a full mesh/depth/material renderer yet.

## Sync behavior

The embedded viewport uses the same `AceAquariumRuntimeController` as the Control Panel.

It updates when:

- Step is pressed
- Run/Pause advances the runtime
- Reset is pressed
- manual actions are pressed
- scenario changes
- Debug Truth is toggled

No separate Aquarium runtime is created.

## Debug Truth / Privacy

When Debug Truth is OFF:

- the scene adapter does not emit ObjectKind labels
- labels such as WATER / ACID / FOOD / WALL / STONE / ICE are not exposed
- the viewport uses generic visual primitives derived from sensory/effect hints

When Debug Truth is ON:

- debug label primitives may be emitted
- the Environment panel shows:

```text
DEBUG TRUTH - NOT AGENT INPUT
```

- the embedded DX12 viewport uses a visible debug strip/color state

Debug Truth is not used by planner/world-model/core decision logic.

## Build artifact cleanup

AQ3D1 updates probe validators so MSVC object files are emitted under:

```text
Build/<validator>/obj/
```

The AQ3D1 validator also checks that the project root contains no build trash:

```text
.obj
.exe
.pdb
.ilk
.log
```

## Limitations

Not implemented in AQ3D1:

- full mesh pipeline
- depth buffer
- camera input/orbit/pan/zoom
- material graph
- terrain
- model import
- asset browser
- 3D editor tools
- AQ-M16+

## Validation

Run:

```powershell
.\Tools\validate_ace_aqcpp1.ps1
.\Tools\validate_ace_aqcpp2.ps1
.\Tools\validate_ace_aqcpp3.ps1
.\Tools\validate_ace_aqcpp4.ps1
.\Tools\validate_ace_aqui0.ps1
.\Tools\validate_ace_ui1r9.ps1
.\Tools\validate_ace_aq3d1.ps1
.\Scripts\build_debug.ps1
```

## Next recommended milestone

```text
ACE-AQ3D2 = Embedded Viewport Mesh Pipeline + Camera Controls
```

Recommended focus:

- real line/triangle mesh batches instead of rectangle-only `UiDrawList`
- camera pan/zoom/orbit
- resize hardening
- selected cell inspector
- stronger debug truth overlay text inside the viewport
