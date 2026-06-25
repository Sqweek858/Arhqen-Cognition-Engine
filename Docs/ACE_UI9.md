# ACE-UI9 - UI Debug Overlay

ACE-UI9 adds a development overlay, basically a tiny Arhqen-flavored Slate Reflector seed. It is meant to stop screenshot archaeology from being the main debugging tool. Humanity may recover.

Implemented:

- `D2DUiDebugOverlay`.
- F9 toggle.
- Command palette entry: `ui_debug`.
- Overlay shows major rects, hover/focus, text stats, retained layout stats, draw command stats and dirty region stats.

Non-goals:

- No live widget picking tree yet.
- No persistent inspector panel yet.
- No remote automation yet.

Next: add click-to-inspect widget and paint flashing.
