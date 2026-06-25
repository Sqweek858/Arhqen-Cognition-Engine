# ACE-UI8 - Invalidation Root / Dirty Regions

ACE-UI8 adds an invalidation root for dirty flags and merged dirty rectangles. It is deliberately small but gives Arhqen UI a sane place to track why repaint/layout happens.

Implemented:

- `AceUiInvalidationRoot`.
- Dirty reasons: Paint, Layout, Text, Effect, Viewport, Input, All.
- Dirty rect marking and simple merge logic.
- Snapshot/consume counters.
- Integration in `invalidate()` and `invalidateRect()`.

Non-goals:

- No full partial-render scheduler yet.
- No widget-level invalidation tree yet.

Next: connect retained layout nodes to invalidation reasons so hover changes repaint only the affected controls.
