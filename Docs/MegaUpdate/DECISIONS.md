# Decisions

- Branch: `feature/ace-editor-mega-update`.
- Push each verified mini-milestone to the existing remote; no automatic PR.
- User content root: `ArhqenCognitionEngine/Content/`, initially empty and sandboxed.
- Canonical units use SI; length is meters.
- Visible console clears on open/close; persistent session/crash diagnostics remain separate.
- UE 5.7 source is the primary architectural reference.
- Prefer UE-compatible ThirdParty foundations when practical; use documented mature alternatives when reproducing massive custom systems would reduce quality.
- Incomplete subsystems remain unexposed.
- Skeletal animation and Niagara/particles are excluded.
- UI text/rendering/input must meet a premium Slate-inspired quality bar.
- Navigation/movement must be frame-rate-independent and premium-feeling.

