# ACE-AQ3D12

## Scope

`ACE-AQ3D12 = Single-HWND 3D Composition`

AQ3D12 moves the Environment 3D main path away from the embedded child HWND / separate DX12 swapchain path.

The child DX12 viewport still exists as fallback/debug infrastructure, but it is no longer the default Environment 3D path.

## Why

AQ3D11 proved that the child HWND path is structurally unstable during resize:

```text
D2D main HWND
  + child HWND
  + flip-model DX12 swapchain
  + live resize
  = flicker soup
```

Several mitigation attempts were tested:

```text
- hide child during resize
- D2D resize proxy
- resize quarantine
- freeze child at last stable rect
```

The remaining flicker points to the architecture itself, not a missing if statement. Humanity grieves. Briefly.

## New main path

```text
AceAquariumRuntimeController
  -> AceAquariumSceneAdapter
      -> AceAqRenderPrimitive list
          -> AceAquariumRealCamera
              -> CPU view-projection transform
                  -> Direct2D single-HWND composition
```

This keeps:

```text
- same Aquarium runtime
- same scene adapter
- same privacy/debug truth rules
- same real camera movement math
- same shell/workspace UI
```

But removes:

```text
- child HWND main path
- child swapchain main path
- DWM child-window resize flicker
- resize proxy ping-pong
```

## Controls

```text
W/S = camera forward/back
A/D = camera strafe left/right
Q/E = world down/up
RMB drag = mouse look
Camera Reset = resets the single-HWND camera
```

Important:

```text
W/S/A/D use camera-relative vectors.
Q/E are strict world vertical.
Diagonal movement remains normalized.
```

## Rendering

AQ3D12 draws the 3D scene in the main HWND using a CPU projected D2D compositor:

```text
viewProjection = AceAquariumRealCamera::ViewProjectionMatrix(aspect)
world primitive center -> clip/NDC -> viewport pixels
```

Current visual support:

```text
- projected ground grid
- projected tiles
- projected blocks
- projected agent marker
- projected direction marker
- projected highlight marker
- debug marker only when Debug Truth is enabled
```

This is not yet a true DX12 offscreen texture compositor. It is the first flicker-safe single-HWND 3D composition path.

## Debug Truth / Privacy

When Debug Truth is OFF:

```text
- ObjectKind labels are not exposed through the visual path
- cognition/runtime still uses the same observation path
```

When Debug Truth is ON:

```text
- debug markers may render
- the viewport label explicitly warns:
  DEBUG TRUTH - NOT AGENT INPUT
```

## Non-goals

Not included:

```text
- DX12 offscreen render target interop
- DirectComposition
- full RHI rewrite
- material graph
- terrain/water/particles
- editor gizmos
- asset import
```

## Validation

Run:

```powershell
.\Tools\validate_ace_aq3d11.ps1
.\Tools\validate_ace_aq3d12.ps1
.\Scripts\build_release.ps1
```

## Next milestone

`ACE-AQ3D13 = DX12 Offscreen Texture Composition`

Recommended next step:

```text
- render Scene3DDrawList into an offscreen DX12 texture
- expose/compose that texture in the main HWND
- keep child HWND path disabled
```


## AQ3D12R1 bounded grid

AQ3D12 fixed resize flicker by moving the main path to single-HWND composition, but the first projected grid was too broad and behaved like an unstable infinite floor.

AQ3D12R1 changes the single-HWND compositor:

```text
- derives grid bounds from Aquarium scene primitives
- ignores old isometric GridLine primitives in this path
- clamps grid span to avoid giant horizon lines
- draws only useful in-viewport bounded grid segments
- keeps the grid under objects and tied to the Aquarium board
```

This is still the CPU-projected bridge, not the future DX12 offscreen texture compositor.
