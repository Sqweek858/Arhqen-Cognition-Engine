# ACE-PERF2R3 - Engine Log Text Selection

## Scope

ACE-PERF2R3 makes the docked Engine Log Console behave like a small text viewer while preserving the existing D2D/DWrite styling.

## Added behavior

- The cursor becomes an I-beam over the log body and console input.
- Click-drag in the log body creates a text selection.
- Selection can span multiple wrapped log lines.
- `Ctrl+C` copies the selected log text to the Windows clipboard.
- `Ctrl+A` selects the input when the input is focused, otherwise selects the log body when it has focus.
- Printable characters are swallowed when the log body has focus so they do not leak into the main chat input underneath the overlay.

## Non-goals

- No renderer/RHI changes.
- No FPS or viewport path changes.
- No D2D/DWrite font replacement.
- No major log console layout rewrite.

## Visual polish note

PERF2R3.2 keeps the existing D2D/DWrite log console and only improves selection feedback.
Selection now uses a single blue-cyan tint for the exact selected text span. The previous row-band plus text-span layering was removed because it created two competing blue highlights that looked like hover and selection fighting over the same row. The console background, border, font, layout, copy logic, renderer, and RHI path are unchanged.

## Validation

Run:

```powershell
.\Tools\validate_ace_perf2r3.ps1
```

The probe checks for selection state, log text hit testing, multiline copy, `Ctrl+C`, `Ctrl+A`, I-beam cursor routing, D2D selection highlight, and lack of renderer/RHI path edits.
