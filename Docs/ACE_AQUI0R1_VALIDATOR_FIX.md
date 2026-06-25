# ACE-AQUI0R1 Validator Fix

## Problem

The original AQUI0 package kept historical validator scripts for AQCPP1/AQCPP2/AQCPP3.

Those validators compiled only the source files that existed at the time of each milestone. After AQCPP4/AQUI0, the shared `AceAqEnvironment.cpp` depends on later core files:

- `AceAqDelayedEffects.cpp`
- `AceAqDynamicWorld.cpp`
- `AceAqWorldEvents.cpp`

So the old `validate_ace_aqcpp1.ps1` script could compile but fail at link time with unresolved externals.

## Fix

All Aquarium validators now compile the current complete Aquarium core:

```powershell
Get-ChildItem Source\Private\Aquarium -Filter "AceAq*.cpp"
```

The probes still test their original milestone behavior, but the linker now receives the full current implementation.

## Changed scripts

- `Tools/validate_ace_aqcpp1.ps1`
- `Tools/validate_ace_aqcpp2.ps1`
- `Tools/validate_ace_aqcpp3.ps1`
- `Tools/validate_ace_aqcpp4.ps1`
- `Tools/validate_ace_aqui0.ps1`

## Non-goals

No runtime behavior changed.

No Aquarium logic changed.

No UI behavior changed.

This patch only fixes stale validator build inputs.
