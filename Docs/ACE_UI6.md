# ACE-UI6 - Retained Layout Tree

ACE-UI6 adds a small retained layout tree inspired by Slate's `Measure -> Arrange -> Paint -> HitTest` discipline, but rewritten for Arhqen UI.

Implemented:

- `AceUiRetainedLayoutTree`.
- Nodes with `desired`, `allocated`, `content`, `clip`, and `hit` rects.
- Horizontal/vertical child arrangement with gap, padding, min size and weight.
- Sibling overlap detection.
- Hit testing and layout stats.
- Shell integration for core regions so the debug overlay can inspect major rects.

Non-goals:

- No full widget hierarchy replacement yet.
- No docking/moveable panels.
- No Slate dependency.

Next: migrate environment controls into retained nodes so the layout tree becomes authoritative rather than diagnostic/foundation-only.
