# ACE-UI1

## Scope

`ACE-UI1 = Frosted Glass + Border Glow + Vignette + Scrollable Panels`

This patch polishes the existing Arhqen Cognition Engine shell UI and the Aquarium control panel.

It does not generate images, does not add a 3D viewport, does not add a DX12 Aquarium renderer, and does not add a Python bridge.

## Implemented

### Frosted-glass feel

The Cognitive Environment Control Panel now uses a stronger layered glass composition:

- higher modal glass opacity
- stronger border glow
- subtle blur fallback layers
- stronger internal highlight
- button glass materials
- section glass materials

This is still a pragmatic Direct2D/HwndRenderTarget-friendly frosted look, not a new shader pass.

### Vignette

When the Environment modal is open, a subtle global edge vignette is drawn using existing panel brushes. This improves focus and makes the overlay feel more like a real engine modal.

### Scrollable panels

Added scroll state and scrollbars for long Aquarium panel sections:

- Logs / Episodes
- Debug Truth
- Counterfactual
- Metrics

Supported interactions:

- mouse wheel over scrollable sections
- draggable scrollbar thumb
- clipped scroll content using `PushAxisAlignedClip`

### UI safety

No 3D was implemented.

No Python runtime was added.

No image generation or image assets were added.

No Aquarium DX12 renderer was added.

## Validation

Run:

```powershell
.\Tools\validate_ace_ui1.ps1
```

Then normal build:

```powershell
.\Scripts\build_debug.ps1
```

## Next recommended patch

`ACE-UI2 = Episode Timeline + Better Inspector Cards`

Possible content:

- timeline list for recent episodes
- selected episode inspector
- pinned section layout
- smarter scroll persistence per scenario
