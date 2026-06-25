# ACE-UI3 DPI / Multi-Monitor Geometry Pass

ACE-UI3 adds a Slate-inspired display geometry layer for the custom D2D shell. It does not import Slate code. The intent is to keep the UI stable when the window moves across monitors, DPI changes, or monitor work areas differ.

## Implemented

- `D2DDisplayMetrics` captures nearest-monitor geometry, work area, client/window rects, and per-window DPI scale.
- `AceShellUi` handles `WM_DPICHANGED` and `WM_DISPLAYCHANGE`.
- `NativeWindow` requests per-monitor DPI awareness when supported by the OS.
- `D2DRenderContext` exposes `dpiScale`, `dpiX`, and `dpiY`.
- Popup/panel helper functions can convert client/screen rects and clamp UI to monitor work area.

## Why this exists

UE/Slate treats geometry as a first-class UI concern. ACE now has the same direction: screen/client transforms and monitor DPI live in a focused helper instead of being scattered through UI code like someone spilled Win32 on the floor.

## Non-goals

- No Slate import.
- No docking system.
- No new 3D features.
- No renderer rewrite.

## Next work

Use `D2DDisplayMetrics::clampToWorkArea` for every future popup/modal and scale selected layout constants through `ctx.dpiScale` where it improves readability.
