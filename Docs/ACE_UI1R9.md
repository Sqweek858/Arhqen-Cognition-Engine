# ACE-UI1R9

## Scope

`ACE-UI1R9 = Subtle Modal Blur + Hover Buttons + Softer Vignette`

## Implemented

- softened and enlarged outer vignette so it looks less like hard dark bars
- added hover response for Aquarium control buttons inside the Environment panel
- increased visible blur/frosted backdrop when Settings or Environment tabs are open

## Notes

This still uses the existing blur fallback path for the current `ID2D1HwndRenderTarget` renderer.
No real Gaussian effect-chain blur was added in this patch.

## Validation

Run:

```powershell
.\Tools\validate_ace_ui1r9.ps1
.\Scripts\build_debug.ps1
```
