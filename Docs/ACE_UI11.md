# ACE-UI11 - Style / Brush System

ACE-UI11 introduces the first Arhqen UI style-set layer. It borrows the Slate concept of named styles and tokens, without dragging in UE's entire dependency planet.

Implemented:

- `AceUiStyleSet`.
- Named color tokens.
- Named panel styles.
- Named text styles.
- Default Arhqen style set.
- Style stats in diagnostics/UI stats.

Non-goals:

- Existing D2D brush creation is not fully replaced yet.
- No external theme files yet.
- No live style editor yet.

Next: route panel/button/text drawing through style IDs and add theme serialization.
