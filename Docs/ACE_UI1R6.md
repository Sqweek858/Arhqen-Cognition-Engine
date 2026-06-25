# ACE-UI1R6

## Scope

`ACE-UI1R6 = Logs Focus Mode + Hide Dashboard`

This patch adds the function that should have existed earlier: a way to hide the dashboard cards and give Logs / Episodes the full remaining Environment panel area.

## Implemented

### Focus Logs toggle

A new runtime control button is added:

```text
Focus Logs
```

When active:

- Body / Agent is hidden
- Observation is hidden
- Planner Trace is hidden
- Counterfactual is hidden
- Memory / Concepts is hidden
- Self Model is hidden
- Delayed / Dynamic is hidden
- World Events is hidden
- Metrics is hidden
- Logs / Episodes uses the full remaining panel area

### Scroll hit targets

Metrics is static and no longer participates in scroll hit-testing.

Scrollable hit targets are now:

- Logs / Episodes
- Debug Truth
- Counterfactual

### No banner stealing log space

Focus Logs mode does not draw an extra banner inside the log area. The active button state is the indicator.

## Non-goals

No 3D viewport.

No DX12 Aquarium renderer.

No Python bridge.

No generated images.

## Validation

Run:

```powershell
.\Tools\validate_ace_ui1r6.ps1
.\Scripts\build_debug.ps1
```
