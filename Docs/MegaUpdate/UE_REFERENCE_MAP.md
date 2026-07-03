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

## M2.4 - editor camera speed

- `Editor/UnrealEd/Public/Settings/EditorViewportSettings.h` separates current/minimum/maximum camera speed and relative speed settings.
- `Editor/UnrealEd/Private/EditorViewportClient.cpp` changes editor speed multiplicatively rather than by a world-unit linear increment.
- ACE adaptation: the same multiplicative/logarithmic principle is extended with event-time wheel momentum, strict `0.0001`–`100000` limits and a single property shared by direct entry and viewport navigation.

## M2.5 - command-driven editor chrome

- UE menu/toolbar widgets consume shared `FUICommandInfo`/`FUICommandList` bindings with dynamic checked/enabled state.
- ACE adaptation: the visible `Window`, `View`, `Help` menus, toolbar and shortcuts all consume `CommandRegistry`. Menus with no implemented backend are omitted entirely.

## M3.1 - Content mount and Asset Registry

- `Runtime/AssetRegistry/Public/AssetRegistry/AssetRegistryState.h`: disk-cache state is separate from query views and supports indexed enumeration by path/name/class.
- `Runtime/CoreUObject/Public/AssetRegistry/AssetData.h`: transient asset metadata is distinct from serialized package contents and the cache format is explicitly versioned.
- `Runtime/AssetRegistry/Private/AssetRegistry.cpp`: missing mounted content directories are created before installing directory watches; directory changes include folder events.
- `Developer/DirectoryWatcher/Public/IDirectoryWatcher.h`: watcher output distinguishes add/modify/remove/rescan-required and is consumed through a tick boundary.
- ACE adaptation: the initial registry is intentionally smaller and filesystem-oriented, but keeps mounted virtual identity, transient indexed records, a versioned external cache and safe full-rescan recovery. M3.2 will add one coarse `Content/` watch with a bounded change queue and rescan-required fallback, matching UE's warning against excessive granular watchers.

## M3.2 - live Content watch and registry deltas

- `Developer/DirectoryWatcher/Private/Windows/DirectoryWatchRequestWindows.cpp`: Windows subtree watches are represented by a native request and translate platform notifications before consumer delivery.
- `Runtime/AssetRegistry/Private/AssetRegistry.cpp` registers coarse content roots with `IncludeDirectoryChanges`, and treats watcher overflow as an explicit rescan condition rather than trusting an incomplete event list.
- ACE adaptation: `AssetDirectoryWatcher` owns one overlapped `ReadDirectoryChangesW` request and a bounded cross-thread queue. Application tick owns debounce and registry mutation; consumers receive stable GUID deltas. ACE currently rebuilds the compact snapshot per debounced batch, which is safer and fast for the present empty/small Content root; the public delta contract permits a future path-local scanner without UI changes.

## M3.3a - identity moves and reference safety

- `Runtime/AssetRegistry/Public/AssetRegistry/IAssetRegistry.h` exposes an explicit rename event with old object path rather than representing a rename as unrelated delete/add notifications.
- `Editor/WorldBookmark/Private/WorldBookmark/Browser/FolderTreeItem.cpp` validates target folder paths and routes subtree renames through Asset Tools rather than raw filesystem calls.
- `Editor/UnrealEd/Public/AssetDeleteModel.h` treats reference discovery as a required phase of deletion, not a cosmetic confirmation dialog.
- ACE adaptation: registry remaps preserve GUIDs across file/folder moves and publish old/new paths. `AssetReferenceIndex` keeps forward and reverse edges so the next filesystem-operations slice can refuse unsafe deletion before touching disk.

## M3.3b - filesystem asset operations

- UE Asset Tools routes validated create/rename/duplicate operations through shared services rather than letting Content Browser widgets mutate files directly.
- `Editor/UnrealEd/Public/AssetDeleteModel.h` separates reference discovery, user decision and actual deletion; unsafe deletion is not a raw filesystem action.
- ACE adaptation: `AssetOperationService` is the only filesystem mutation boundary and records each completed action in `TransactionManager`. Duplicate/delete use exact external stashes, registry GUIDs survive undo/redo, and known reverse references block deletion before disk mutation. Material creation remains omitted until the real M6 format/compiler exists.

## M3.4 - Content Browser view model

- `Editor/ContentBrowserData/Public/ContentBrowserDataSubsystem.h` separates item enumeration, path conversion and queued item updates from Slate widgets.
- `Editor/ContentBrowserData/Public/ContentBrowserItem.h` gives browser items stable identity independent of their current visual row or tile.
- `Editor/ContentBrowser/Public/ContentBrowserDelegates.h` keeps selection, activation and rename as explicit interaction boundaries.
- ACE adaptation: `ContentBrowserModel` consumes immutable registry generations and projects navigation, breadcrumbs, folders, filtered direct children and stable selection without D2D ownership. Assets use GUID identity across moves; folder/current-path state repairs conservatively after removal. The next slice can render and route input without duplicating asset logic inside paint code.

## M3.5 - Content Browser interaction and drawer

- UE keeps Content Browser item actions behind data-source/Asset Tools operations and uses temporary creation/rename contexts rather than mutating files from Slate paint code.
- UE's Content Drawer is a docked editor surface with shared command bindings and keyboard focus, not a separate renderer window.
- ACE adaptation: `ContentBrowserController` is the action boundary; the D2D drawer consumes only controller/model state. Ctrl+Space toggles the existing workspace tab, inline edits commit through validated operations, and focus prevents browser shortcuts from leaking into the viewport. Only backed actions are painted.

## M3.6 - scene world and editor hierarchy

- `Runtime/Engine/Classes/Engine/World.h` and `GameFramework/Actor.h` separate world ownership, stable actor identity/labels, transforms and attachment hierarchy.
- `Editor/UnrealEd/Private/EditorActor.cpp` routes attachment and label changes through validated editor operations; actor children are enumerated recursively for hierarchy actions.
- `Editor/UnrealEd/Private/EditorActorFolders.cpp` maintains folder state as world/editor data and broadcasts hierarchy changes rather than deriving ownership from painted rows.
- ACE adaptation: `SceneWorld` owns GUID entities and validated hierarchy, `SceneSelection` owns selection identity, and `SceneHierarchyModel` projects indexed children for the D2D Outliner. `.acescene` persistence uses ACE's bounded archive rather than serializing widget state. Details is a read-only projection in this slice; transform transactions/gizmos follow in M3.7.

## M3.7a - interactive transform transactions

- `Editor/UnrealEd/Public/EditorModeManager.h`, `MouseDeltaTracker.h` and `LevelEditorViewport.cpp` separate StartTracking, repeated InputDelta and TrackingStopped boundaries.
- UE starts one transaction for a widget drag and finalizes it at tracking stop instead of transacting every mouse delta.
- ACE adaptation: `SceneEditController` owns the same lifecycle over `SceneWorld`. It applies live multi-target values, records one before/after operation at commit, restores exact pre-drag values on cancel, and leaves paint/input/gizmo math for M3.7b.

## M3.7b - transform gizmo math

- UE's editor widget applies translate/rotate/scale deltas through mode tools while the mouse delta tracker owns accumulated tracking state and snapping policy.
- ACE adaptation: `TransformGizmo` evaluates every update from immutable drag-start transforms, filters by axis/plane, converts local translation through the entity Euler basis and snaps each domain independently. It delegates state mutation and history to `SceneEditController`; visible handles wait for real picking.

## M3.7c - scene picking foundation

- UE viewport selection separates hit-proxy identity from widget paint and resolves selection only after the viewport has produced an authoritative hit result.
- `Runtime/Engine/Public/HitProxies.h` defines stable hit-proxy identity and priority concepts; editor viewport clients consume the resolved hit instead of embedding scene mutation in rendering code.
- ACE adaptation: `ScenePicker` owns GUID-keyed world bounds, normalized ray intersection and deterministic priority/identity tie-breaking independently of D2D paint. The initial broad phase is linear but hidden behind a replaceable API; screen unprojection, render-proxy synchronization and visible gizmo handles follow as separate integration slices.

## M3.7d - viewport unprojection and rendered-scene selection

- `Runtime/Engine/Private/SceneView.cpp` converts pixels inside the actual view rectangle to projection space, constructs a near-plane origin and transforms a normalized ray into world space.
- `Editor/UnrealEd/Private/EditorViewportClient.cpp` resolves a hit at the click boundary and only then dispatches selection, keeping render identity separate from editor mutation.
- ACE adaptation: `ViewportProjection` performs the equivalent perspective math directly from ACE's validated camera basis, while `AquariumScenePickAdapter` translates the exact submitted primitive stream into stable-GUID proxies. The shell consumes the resolved hit through the shared `SceneSelection`; it does not infer selection from D2D layout or labels.
