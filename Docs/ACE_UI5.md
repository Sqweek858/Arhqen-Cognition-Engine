# ACE-UI5 - Text Layout / Clipping / Ellipsis

ACE-UI5 introduces a Slate-inspired text layout foundation for Arhqen UI. Text is treated as measured layout content instead of raw coordinates thrown at DirectWrite and then politely ignored by reality.

Implemented:

- `D2DTextLayoutFoundation`.
- Explicit `Clip`, `Ellipsis`, and `Wrap` overflow modes.
- Binary-search ellipsis fitting using the existing DirectWrite/text cache path.
- Push/pop clip helpers for widget text regions.
- Stats for draw, measure, ellipsis, clip and cache-friendly layout counts.

Non-goals:

- No Slate import.
- No typography engine rewrite.
- No new rendering backend.

Next: migrate more existing `drawTextEx` call sites to explicit text layout specs instead of raw one-off rectangles.
