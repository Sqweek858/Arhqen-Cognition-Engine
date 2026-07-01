# ACE-AQ3D11 / M1

## Scope

`ACE-AQ3D11 / M1 = Real 3D Viewport + Simple Camera`

This milestone starts replacing the old projected/2D-style Environment viewport with a minimal real 3D path.

The embedded DX12 child viewport remains the shell integration point. AQ3D11 changes what is rendered inside it:

```text
AceAquariumRuntimeController
  -> AceAquariumSceneAdapter
      -> Scene3DDrawList
          -> Dx12Renderer::renderFrame3D
              -> real 3D vertex shader
              -> depth buffer
              -> embedded DX12 child viewport
```

## Camera

Added:

```text
AceAquariumRealCamera
```

Camera state:

- 3D position
- yaw
- pitch
- forward/right/up vectors
- view matrix
- projection matrix
- view-projection matrix
- dt-based movement

## Controls

Movement:

```text
W = move forward along camera forward
S = move backward along camera forward
A = strafe left relative to camera right
D = strafe right relative to camera right
Q = world down
E = world up
```

Important rule:

```text
W/S/A/D are camera-relative.
Q/E are the only hardcoded world vertical movement controls.
```

So if the camera looks up, W moves upward along the view direction. If the camera looks down, W moves downward along the view direction. The world-vertical controls remain Q/E only, because apparently even chaos needs a vertical axis.

Mouse look:

```text
right mouse drag = yaw/pitch look
pitch is clamped
yaw is free
```

No Shift/Ctrl speed controls are added in M1.

## Rendering

AQ3D11 adds:

```text
Scene3DDrawList
Dx12Renderer::renderFrame3D
3D vertex shader
3D pixel shader
constant buffer MVP
depth buffer / DSV
```

The current 3D scene renders:

- ground grid / floor
- Aquarium tiles
- simple object boxes
- agent marker
- direction marker
- front cell highlight
- debug marker primitives only when Debug Truth is enabled

Geometry is still minimal, but it is now sent through a real 3D vertex path with a view-projection matrix and depth testing.

## Privacy / Debug Truth

When Debug Truth is OFF:

- the viewport does not expose ObjectKind labels to cognition/runtime
- scene colors/forms are visual-only
- the runtime still receives observations through the existing privacy-preserving path

When Debug Truth is ON:

- debug primitives may appear
- UI/debug warning remains separate

Debug Truth is not fed into planner/world-model/core logic.

## Non-goals

Not implemented in AQ3D11:

- editor mode
- gizmo
- material graph
- shader compiler refactor
- FBX/glTF import
- terrain/landscape
- water shader
- particles
- ray tracing
- orbit camera
- focus selected
- Blender/UE-style full control set
- AQ-M16+

## Validation

Run:

```powershell
.\Tools\validate_ace_aq3d10.ps1
.\Tools\validate_ace_aq3d11.ps1
.\Scripts\build_release.ps1
```

The AQ3D11 probe validates:

- forward vector changes with yaw/pitch
- W/S use camera forward
- A/D use camera right
- Q/E use world up/down
- diagonal movement is normalized
- pitch is clamped
- movement is dt-based
- view/projection matrices exist
- AQ3D10 resize hardening marker still exists

## Known limitations

- geometry uses simple colored triangles/boxes
- no real mesh/material pipeline yet
- no depth texture resizing API beyond existing renderer reinit path
- no camera speed tuning UI yet
- no scroll-wheel speed or FOV controls yet

## Next milestone

```text
ACE-AQ3D12 / M2 = Camera Polish + Primitive Mesh Quality
```

Recommended next work:

- proper mesh helpers for floor, cubes, arrows
- optional scroll speed/FOV
- camera status overlay
- selected-cell visual marker
- stronger debug overlay text inside the viewport


## AQ3D11R1 correction

AQ3D11R1 disables the old `aquariumUseSingleHwndCompositeViewport_` path by default.

Reason:

```text
Single-HWND composite viewport = old projected fallback
Embedded DX12 child viewport + renderFrame3D = AQ3D11 real 3D path
```

The visible Environment status now says:

```text
Real DX12 3D active | RMB look | WASD move | Q/E vertical
```

This correction is required so WASD/mouse-look affects the real camera instead of leaving the UI stuck on the old composite view.


## AQ3D11R2 correction

AQ3D11R2 fixes two user-visible problems from AQ3D11R1:

- mouse X was inverted for yaw, so dragging right turned the view left;
- resizing the top-level window could flicker because the live child DX12 swapchain path was still involved.

Mouse correction:

```text
yaw -= mouseDeltaX * sensitivity
```

Resize correction:

```text
live resize begins
  -> hide embedded DX12 child viewport
  -> do not freeze the native top-level window
  -> do not use the popup resize shield
  -> paint the old stable D2D composite as resize-only proxy
live resize ends
  -> restore embedded child viewport
  -> renderFrame3D resumes
```

The resize proxy is not the main 3D path. It is only a temporary resize cover to avoid flip-model child swapchain flicker while the user drags the window border.


## AQ3D11R3 resize quarantine

AQ3D11R3 adds a stronger resize quarantine around the embedded DX12 child viewport.

Fixed resize cases:

```text
- native top-level WM_ENTERSIZEMOVE / WM_EXITSIZEMOVE
- direct WM_SIZE / maximize / restore
- Environment side-panel resize handles
- post-resize restore timing
```

During quarantine:

```text
- embedded child HWND is hidden/moved off-screen
- renderer resize apply is suspended
- parent paints the stable D2D proxy
- child HWND sync is blocked
- renderFrame3D resumes only after a short stable delay
```

This keeps the real 3D path as the default, but avoids showing the flip-model child swapchain while layout is actively changing.


## AQ3D11R5 no-proxy resize freeze

AQ3D11R5 removes the resize proxy swap from the active path.

Problem:

```text
R3 could switch between the real DX12 child viewport and the old D2D resize proxy.
This caused visible 2D/3D ping-pong during resize.
```

New behavior:

```text
- no D2D proxy swap during resize
- no HideForLiveResize loop in the main UI path
- child DX12 viewport stays visible at the last stable rect
- child MoveWindow / swapchain resize remains suspended while layout changes
- final rect is applied after a short stable delay
```

Result: during resize the 3D viewport may look temporarily frozen, but it should no longer alternate between old 2D and real 3D.
