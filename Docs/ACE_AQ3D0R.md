# ACE-AQ3D0R

## Scope

`ACE-AQ3D0R = Real DX12 3D Environment Entry + Separate Viewport Surface`

This patch corrects the previous AQ3D0 direction.

The previous AQ3D0 patch introduced a D2D/isometric fallback embedded in the Environment Control Panel. That was useful as a first visual proof, but it is not acceptable as the main 3D direction.

AQ3D0R adds an explicit DX12 entry point:

```text
Open DX12 Viewport
```

The button opens a separate DX12-backed viewport window synchronized with the existing Aquarium C++ runtime.

## What changed from AQ3D0

### Removed D2D fallback as default 3D path

The Environment panel no longer presents the embedded D2D/isometric card as the active 3D viewport.

It now displays a status/entry card:

```text
3D Environment is available. Press Open DX12 Viewport to launch the separate DX12 surface.
```

### Added separate DX12 viewport surface

Added:

```text
AceAquariumDx12Viewport
```

This class owns:

- a separate Win32 window
- its own `Dx12Renderer`
- its own DX12 swapchain through the existing renderer
- a runtime-synchronized render loop triggered from `AceShellUi::tick`

Rendering path:

```text
AceAquariumRuntimeController
  -> AceAquariumSceneAdapter
      -> AceAquariumDx12Viewport
          -> Dx12Renderer
```

## Current rendering path

```text
DX12 separate window
```

The first DX12 viewport uses the existing `Dx12Renderer` / `UiDrawList` primitive path.

That means the first visuals are simple DX12-rendered projected primitives, not a full mesh/material/depth pipeline yet.

Visible elements:

- floor/grid
- tile/object blocks
- agent marker
- direction arrow
- front cell highlight
- debug warning bar/title when Debug Truth is enabled

## How to open it

In the Environment Control Panel:

```text
Manual Actions -> Open DX12 Viewport
```

If the viewport is already open, the button brings it to front.

## Sync behavior

The viewport renders from the same `AceAquariumRuntimeController` used by the Control Panel.

It updates when:

- Step is pressed
- Run/Pause advances the runtime
- Reset is pressed
- manual actions are pressed
- Debug Truth is toggled

No separate runtime is created.

## Debug Truth / Privacy

When Debug Truth is OFF:

- the scene adapter does not emit ObjectKind labels
- no WATER / ACID / FOOD / WALL / STONE / ICE labels are exposed
- the viewport uses generic primitives derived from sensory/effect properties

When Debug Truth is ON:

- debug label primitives may be emitted
- the DX12 viewport title includes:

```text
DEBUG TRUTH - NOT AGENT INPUT
```

- the DX12 surface draws an orange debug strip

Debug Truth is not used by planning/world-model decision logic.

## Limitations

Not implemented yet:

- embedded child viewport inside the Environment panel
- real 3D mesh pipeline
- depth buffer
- camera input/orbit/pan/zoom
- shader/material system
- model import
- terrain
- selection/edit tools
- particles/post-processing

The DX12 path is real, but the geometry is intentionally minimal.

## Validation

Run:

```powershell
.\Tools\validate_ace_aqcpp1.ps1
.\Tools\validate_ace_aqcpp2.ps1
.\Tools\validate_ace_aqcpp3.ps1
.\Tools\validate_ace_aqcpp4.ps1
.\Tools\validate_ace_aqui0.ps1
.\Tools\validate_ace_ui1r9.ps1
.\Tools\validate_ace_aq3d0r.ps1
.\Scripts\build_debug.ps1
```

## Next recommended milestone

```text
ACE-AQ3D1 = Camera/Input/Viewport Polish + Object Visuals
```

Recommended content:

- camera zoom/pan/orbit
- stronger object shapes
- cell selection / inspector bridge
- optional depth buffer / simple mesh pipeline
- better debug truth overlay text inside DX12 viewport
