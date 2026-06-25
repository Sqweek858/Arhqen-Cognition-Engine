# ACE-UI3G - Logical UI Scale + Viewport Fit Polish

ACE-UI3G keeps the ACE-UI3F pixel-space D2D correction intact and adds a controlled logical scale pass for the 3D Environment UI.

## Problem

ACE-UI3F fixed the DPI blow-up by forcing the D2D HWND render target back to 96 DPI. That made the layout stable again, but the 3D Environment panels and text became visually too small on high-resolution displays.

The fix is **not** to re-enable automatic D2D DPI scaling. That path caused the previous oversized layout bug. Instead, ACE-UI3G makes the 3D Environment UI intentionally larger through its own layout constants, font sizes, panel defaults, and viewport fit rules.

## Changes

- Keeps D2D render target DPI at 96.0 so the UI remains pixel-space.
- Adds `LogicalScale` documentation to the 3D layout constants.
- Increases 3D mode topbar height, panel padding, section spacing, button height, and default panel widths.
- Increases key text formats by about one pixel so the UI reads better without becoming the earlier zoomed-in disaster.
- Enlarges the 3D viewport scene fit and lowers the projected scene slightly so it feels more centered and intentional.
- Keeps AQ3D14 single-HWND composite as the main viewport path.

## Non-goals

- No D2D automatic DPI scaling.
- No child DX12 HWND resurrection.
- No new Aquarium cognition behavior.
- No new 3D renderer features.
- No moveable/docking panel work.

## Validation

`Tools/validate_ace_ui3g.ps1` checks that:

- pixel-space D2D DPI correction remains active;
- logical 3D scale constants exist;
- 3D panels/buttons/topbar are larger than the tiny UI3F defaults;
- text formats are nudged upward;
- AQ3D14 single-HWND composite remains active;
- the viewport fit was polished instead of changing the render architecture.

## Next

The next sensible UI pass is ACE-UI2-style text layout and clipping: proper ellipsis, text measurement, and no-overlap guarantees for every widget row.
