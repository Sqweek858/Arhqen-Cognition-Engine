# ACE-UI1R5

## Scope

`ACE-UI1R5 = Logs Contained + Working Log Scrollbar`

This patch fixes the Logs / Episodes panel escaping below the Environment panel.

## Fixed

### Logs are clamped inside the Environment panel

The previous layout used fixed/minimum row heights that could push Logs below the modal boundary.

Now the layout reserves log space first, then shrinks the upper dashboard cards when vertical space is tight.

### Scrollbar remains on Logs

Logs / Episodes remains scrollable and its visible rect is now inside the modal.

Debug Truth shares the log area when enabled and is also clamped inside the modal.

### Removed old escaping expression

The old layout expression:

```text
std::max(logTop + 76.0f, logBottom)
```

could create a log rectangle extending past the panel.

It has been replaced with a bounded `logsRect`.

## Non-goals

No 3D viewport.

No DX12 Aquarium renderer.

No Python bridge.

No generated images.

## Validation

Run:

```powershell
.\Tools\validate_ace_ui1r5.ps1
.\Scripts\build_debug.ps1
```
