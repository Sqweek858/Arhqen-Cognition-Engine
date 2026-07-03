# UE 5.7 Reference Map

UE root: `C:\Users\Sqweek\Documents\UE_5.7\Engine\Source`

Populate during implementation with:

- ACE subsystem/decision.
- UE files/modules inspected.
- Reusable architectural idea.
- What is intentionally omitted.
- ThirdParty dependency/licensing notes.
- ACE implementation files and tests.

## M1.1 — units and numeric editor fields

- `Runtime/Core/Public/Math/UnitConversion.h`: separate unit identity, physical dimension, compatibility and display settings.
- `Runtime/Core/Public/Math/UnitConversion.inl`: convert through a shared unit per dimension and quantize for readable presentation.
- `Runtime/Core/Private/Math/UnitConversion.cpp`: centralized aliases, symbols, scale factors, affine temperature conversion and display-unit ranges.
- `Runtime/Core/Tests/Math/UnitConversionTests.cpp`: table-driven conversion and parsing coverage.
- `Editor/DetailCustomizations/Public/Customizations/MathStructProxyCustomizations.h`: details fields attach a unit-aware numeric interface instead of embedding conversion rules in widgets.

ACE adaptation: canonical storage is strict SI (meters, radians, seconds, kilograms, kelvin, newtons and derived SI), unlike UE's historical centimeter world scale. Editor parsing/formatting is centralized in `Core/Units`; UI widgets will consume it without owning conversion policy.

## M1.2a — stable identity and mounted asset paths

- `Runtime/Core/Public/Misc/Guid.h` and `Runtime/Core/Tests/Misc/GuidTest.cpp`: GUID is a small value type with explicit validity, canonical parse/format and archive-friendly bytes.
- `Runtime/CoreUObject/Public/Misc/PackagePath.h`: package identity is a mounted virtual name distinct from a local filesystem path; validation and conversion happen at the mount boundary.

ACE adaptation: `Guid` persists independently from asset names, while `AssetPath` exposes only `/Game/...` and maps it to the confirmed `Content/` root. ACE additionally normalizes UTF-8 to NFC and creates a Unicode case-insensitive key because the current target filesystem is Windows.

## M1.2b — versioned serialization and atomic files

- `Runtime/Core/Public/Serialization/Archive.h`: serialization owns explicit loading/saving state, versions and bounded typed operations rather than raw struct dumps.
- `Runtime/Core/Tests/Serialization/CompactBinaryWriterTest.cpp`: round-trip and malformed-input tests are part of the format contract.
- `Runtime/Core/Tests/Misc/FileTest.cpp`: file behavior is tested independently from object serialization.

ACE adaptation: the initial archive is deliberately smaller than UE's archive stack but preserves the essential contract: fixed endianness, independent container/schema/object versions, bounded payloads, corruption detection and typed reads. Persistence uses a flushed temporary file followed by same-directory atomic replacement; higher asset layers must first resolve paths through the `/Game` sandbox.

## M1.3 — transactions

- `Editor/UnrealEd/Public/ScopedTransaction.h`: RAII defines a deliberate transaction boundary and supports cancellation.
- `Editor/UnrealEd/Private/EditorTransaction.cpp`: grouped records undo in reverse order, redo forward, and custom changes avoid unnecessary whole-object snapshots.

ACE adaptation: a compact operation-based transaction manager provides grouping, scoped commit/cancel, redo-branch invalidation and explicit memory/entry budgets. Property, transform, graph, asset and landscape layers will supply typed operations to this single manager.

## M1.4a — editor commands

- UE editor modules consistently expose `FUICommandInfo` metadata and bind it through shared `FUICommandList` instances; command definitions remain separate from widgets that display or invoke them.
- Context-specific command lists take precedence over global bindings, while shortcut conflicts remain inspectable.

ACE adaptation: `CommandRegistry` owns stable dotted IDs, localized-ready labels/descriptions, dynamic enabled/checked state, contextual chords, rebinding, search and conflict diagnostics. Menus, toolbar, command palette and shortcut editor will consume this registry rather than hardcoded callbacks.

## M1.4b — input routing

- Slate's useful architectural boundary is a single routed event path with explicit handled replies, focus ownership, mouse capture and higher-layer popup/modal precedence.
- ACE adaptation: `InputRouter` owns those lifetimes and invokes `CommandRegistry` only after the focused target declines a key event. Capture and focus are repaired on release, disable, unregister, modal activation and window deactivation.

## M1.4c — panel edge resize

- Slate splitters use a narrow visual separator with a larger interaction area, explicit mouse capture and layout-owned sizing.
- ACE adaptation: `PanelResizePolicy` separates invisible hit slop, edge/corner classification, cursor choice and physical-bound clamping from individual panels. Aquarium Details/Logs now use side and bottom edges, persist only the completed drag, and no longer render corner buttons.

## M2.1 - editor workspace layout

- UE editor modes build layouts from nested `FTabManager::NewPrimaryArea`, `NewSplitter` and `NewStack` declarations, with stable tab IDs and normalized size coefficients.
- Representative layouts inspected in `Editor/AnimationEditor/Private/AnimationEditorMode.cpp`, `Editor/BehaviorTreeEditor/Private/BehaviorTreeEditorModes.cpp` and `Editor/AudioEditor/Private/SoundCueEditor.cpp`.
- ACE adaptation: `EditorWorkspaceLayout` keeps the useful declarative split/stack/tab topology, stable IDs and independent persistence while intentionally omitting floating windows and the much larger global tab-spawner framework until ACE has real consumers for them.

## M2.2 - workspace geometry

- UE's declarative editor layouts separate persistent size coefficients from the actual arranged widget geometry and use splitters as layout-owned boundaries.
- ACE adaptation: `EditorWorkspaceGeometrySolver` arranges only visible descendants, separates visual and hit rectangles, preserves unrelated coefficients during boundary edits and remains deterministic under impossible/tiny bounds. Rendering and pointer capture remain consumers of this model rather than owners of its math.

## M2.3a - workspace interaction

- Slate's splitter interaction keeps capture ownership explicit, updates layout while dragging and treats release/cancel as distinct lifecycle boundaries.
- ACE adaptation: `EditorWorkspaceController` owns that lifecycle over the pure model/geometry layers. It never writes during pointer movement, emits a one-shot commit request after release, and restores a full pre-drag snapshot on cancellation or window rearrangement.

## M2.3b - visible editor host

- UE keeps the level viewport as one editor content region while Slate owns surrounding tabs, panels and input; switching editor modes does not recreate the rendering device.
- ACE adaptation: Engine Mode remains inside the existing top-level HWND and reuses the proven single-HWND DX12/D2D viewport composition. Only panels with real data are exposed, and the AI-specific telemetry overlay is deliberately excluded from the editor viewport.
