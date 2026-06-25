# ACE-UI1R1

## Scope

`ACE-UI1R1 = Stronger Frost + Real Scroll Only Where Needed`

This patch responds to the first ACE-UI1 visual pass being too subtle.

## Fixed

### Static panels no longer get decorative scrollbars

`renderAquariumLines()` is now a real static card renderer again.

Scrollbars are only used by explicit long sections:

- Logs / Episodes
- Debug Truth
- Counterfactual
- Metrics

### Stronger frosted-glass look

The Environment / Aquarium modal now has:

- stronger overlay opacity
- stronger edge vignette
- higher modal fill alpha
- stronger border glow
- stronger blur fallback
- an internal frosted veil to suppress background hero text bleeding through the modal

## Non-goals

No 3D viewport.

No Python bridge.

No DX12 Aquarium renderer.

No generated images.

## Validation

Run:

```powershell
.\Tools\validate_ace_ui1r1.ps1
.\Scripts\build_debug.ps1
```
