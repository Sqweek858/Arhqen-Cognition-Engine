# Mega Update Changelog

## Planning

- Consolidated feature specification.
- Added execution, Git safety, context resilience, testing, UI quality, movement quality and milestone rules.

## 2026-07-02 — M0.1 baseline preparation

- Activated the mega-update on the dedicated feature branch.
- Preserved the complete renderer/input/UI working tree inherited from VTBRIDGE5.
- Added persistent execution, milestone, recovery, test and performance documentation.
- Expanded generated-file ignore coverage.
- Verified Debug/Release builds and the current core/UI/RHI/bridge validation set.
- Recorded stale validators and unsafe partial-present telemetry honestly instead of normalizing them away.

## 2026-07-02 — M1.1 canonical units

- Added canonical SI storage dimensions for editor, rendering and physics values.
- Added metric and imperial length/area/volume/speed/mass/force/torque/pressure units plus time, angle, temperature, data and physical light units.
- Added strict unit-aware parsing, stable validation errors, best-fit display selection and deterministic formatting.
- Added standalone warning-as-error tests and both MSBuild/CMake source registration.
- Aligned CMake Win32 Unicode/configuration defines with the canonical Visual Studio build after the cross-build gate exposed ANSI macro expansion failures.

## 2026-07-02 — M1.2a stable identity and virtual asset paths

- Added persistent RFC 4122 version-4 GUID values with canonical lowercase text round-tripping and stable hashing.
- Added strict `/Game` virtual paths rooted in `Content/`; engine source and arbitrary filesystem paths cannot enter the asset namespace.
- Added traversal, invalid-character, reserved-name, length and malformed UTF-8 rejection.
- Added NFC normalization and Unicode-aware case keys to prevent visually duplicate assets on Windows.
- Added warning-as-error identity/path probes and both build-system registrations.

## 2026-07-02 — M1.2b versioned persistence

- Added a stable little-endian ACE container with schema GUID, container/object versions, payload length and FNV-1a checksum.
- Added bounded readers and writers for numeric primitives, booleans, GUIDs and validated UTF-8 strings.
- Added explicit errors for corruption, truncation, incompatible schemas/versions and unsafe sizes.
- Added durable same-directory atomic replacement and bounded file reads on Windows.
- Added warning-as-error failure-path tests and verified MSBuild Debug/Release plus CMake Debug.

## 2026-07-02 — M1.3 transactions

- Added central grouped transactions with deterministic undo/redo ordering.
- Added cancel/revert, scoped auto-commit, redo-branch invalidation and history metadata.
- Added entry and memory budgets with oldest-history eviction.
- Added warning-as-error behavioral tests and verified all build configurations.

## 2026-07-02 — M1.4a editor commands

- Added a single registry for command metadata, execution, enabled/checked state and contextual shortcuts.
- Added shortcut rebinding, context precedence, searchable command views and deterministic conflict reporting.
- Ambiguous shortcuts are never executed arbitrarily.
- Added warning-as-error behavior tests, including a move/evaluation-order regression test discovered during implementation.

## 2026-07-02 — M1.4b input routing

- Added one z-ordered route for pointer, wheel, keyboard and text events.
- Added explicit focus and pointer-capture ownership with cleanup on release, disable, unregister and window deactivation.
- Added modal barriers and context-aware command fallback after focused widgets decline an event.
- Added warning-as-error lifecycle tests and verified MSBuild Debug/Release plus CMake Debug.

## 2026-07-02 — M1.4c global edge resize

- Removed the visible Aquarium corner resize widgets and reclaimed their content space.
- Added reusable invisible left/right/top/bottom/corner hit testing with contextual cursors.
- Removed fixed 500 px width and 920 px height caps; physical layout bounds are now the maximum.
- Added recoverable minimums and persisted final dimensions without writing during mouse movement.
- Added behavioral, persistence and shell-integration gates across all build configurations.

## 2026-07-02 - M1.4d text/style/layout quality

- Corrected the DirectWrite cache key to include trimming behavior and font-resource generation.
- Added real DirectWrite ellipsis trimming instead of slicing the displayed UTF-16 string in the draw path.
- Prevented invalid layouts from polluting the LRU and normalized non-finite or invalid layout dimensions safely.
- Added pixel-snapped text origins and surrogate-safe behavior for the legacy string-returning ellipsis helper.
- Added monotonic font/style generations for future live theme and resource invalidation.
- Added a warning-as-error probe for real DirectWrite layouts, Romanian/Unicode text, native trimming, caret hit testing, cache invalidation and bounded eviction.

## 2026-07-02 - M2.1 editor workspace layout model

- Added a compact Slate-inspired split/stack/tab tree with stable node and tab identifiers.
- Added the default Engine workspace topology: central viewport, right Outliner/Details split and hidden lower Content Browser drawer.
- Added deterministic show/hide/activation, normalized direct resize ratios, reset and recursive visibility queries.
- Added strict depth/count/identifier/UTF-8 validation and repair of recoverable active-tab or ratio state.
- Persisted the independent editor layout through the versioned ACE archive and atomic file replacement systems.
- Added warning-as-error tests for behavior, corruption, missing files, invalid text, round trips and temporary-file cleanup.

## 2026-07-02 - M2.2 workspace geometry and splitter ratios

- Added a deterministic geometry solver for the persistent split/stack/tab workspace tree.
- Hidden panels collapse fully; a lone visible child fills its parent without a dead separator.
- Splitters now expose separate one-pixel visual bounds and larger clipped interaction bounds.
- Added direct splitter/tab hit testing and pair-local ratio edits suitable for pointer-captured dragging.
- Added minimum-pane distribution and adaptive behavior for physically tiny and even subpixel bounds.
- Added warning-as-error tests for exact topology, panel visibility, drawer geometry, hit targets, ratios and malformed inputs.

## 2026-07-02 - M2.3a workspace interaction controller

- Added splitter pointer capture with orientation-correct hover and captured cursors.
- Rebuilds geometry during drag while deferring persistence until one mouse-up commit request.
- No-op clicks remain clean and never request a disk write.
- Cancel and window rearrangement restore the exact pre-drag layout plus previous dirty/commit state.
- Tab activation, panel visibility and reset now use the same controller-owned mutation boundary.
- Added warning-as-error tests for pointer lifecycle, invalid input, cursor behavior and one-shot commits.

## 2026-07-03 - M2.3b visible Engine-mode shell

- Added the `Engine` entry point and `AI Details` return path in the same native window.
- Rendered the persisted editor workspace around the real DX12 viewport through the existing D2D composition path.
- Added functional Outliner/Details visibility controls, Reset Layout and pointer-captured splitters.
- Added independent atomic editor-layout load/save with canonical-topology recovery.
- Kept Content Browser and unavailable editor commands hidden rather than presenting inert UE-shaped controls.
- Removed Aquarium telemetry and AI scenario/planner labels from Engine Mode after visual smoke feedback.
- Added strict shell-integration checks and a real hidden Debug executable startup smoke test.

## 2026-07-03 - M2.4 premium camera speed

- Added a frame-independent logarithmic camera-speed model covering `0.0001` through `100000`.
- Wheel cadence now shapes momentum: isolated notches stay precise while rapid same-direction impulses accelerate smoothly.
- Preserved scroll ownership for logs, panels and popups before routing unhandled wheel input to the real DX12 viewport.
- Added the same code-native camera control to AI Details and Engine Mode, with immediate numeric and logarithmic-bar feedback.
- Added an anchored direct-entry popup with strict parsing, clipboard editing, Enter commit, Escape cancel and outside-click dismissal.
- Kept D2D popup layers above DX12 by integrating them with the existing parent-composition overlay policy and retained dirty layer.
- Scaled camera acceleration above the tuned default so high requested speed values affect actual travel rather than only the UI.
- Added 32 warning-as-error checks plus Debug, Release, CMake and hidden-startup gates.

## 2026-07-03 - M2.5 functional editor command surface

- Added a contextual editor command registry as the single backend for visible menu, toolbar and shortcut actions.
- Added functional `Window`, `View` and `Help` menus while deliberately withholding scene, asset, shader and tool menus with no backend yet.
- Added live checked state for Outliner, Details and Console plus real actions for layout reset, camera reset/speed, help and AI-mode return.
- Added a compact toolbar that executes the same registered commands rather than maintaining a second action path.
- Added Ctrl+Shift+O, Ctrl+Shift+D, Ctrl+0 and F1 routing with repeat suppression and input-focus barriers.
- Painted menu popups after viewport/panels and integrated their lifetime with parent composition, Escape, outside-click and focus-loss behavior.
- Corrected Shortcut Help priority so its pointer and Escape input cannot leak into Engine Mode underneath.
- Expanded the shell gate to 20 focused command/menu/overlay assertions and passed all build/startup configurations.

## 2026-07-03 - M3.1 Content mount and Asset Registry

- Added a runtime-created `Content/` root mounted exclusively as `/Game`, with registry state stored outside user content.
- Added typed asset/folder records, stable GUID and virtual-path indexes, deterministic ordering and exact direct/descendant counts.
- Added supported ACE, mesh-source and texture extension classification while excluding unsupported technical files.
- Skipped dot/internal, Windows hidden/system, symlink and unsafe entries without following them outside the sandbox.
- Added versioned atomic registry identity persistence and recoverable rebuild after corrupt state.
- Wired the registry into application initialization and verified a fresh runtime leaves `Content/` at exactly zero entries.
- Added 24 warning-as-error tests plus GUID/path/archive regressions and all build configurations.

## 2026-07-03 - M3.2 live Content watch and deltas

- Added one coarse asynchronous Windows watch over the complete Content subtree, including folder events.
- Added a bounded thread-safe event queue, repeated-modification coalescing and explicit `RescanRequired` recovery on overflow or malformed native data.
- Preserved native old/new rename pairs and exposed watcher batch, queued, delivered, dropped and rescan counters.
- Consumed filesystem events on the main runtime tick after a short quiet debounce with a hard maximum delay.
- Added deterministic registry deltas containing stable GUID lists for added, modified and removed assets.
- Made rescan publication transactional so duplicate identity/path failure cannot leave partially rebuilt lookup indexes.
- Added a 34-check warning-as-error gate and an end-to-end application add/remove watch smoke with an empty final Content root.

## 2026-07-03 - M3.3a asset identity moves and references

- Added identity-preserving path remapping for one asset or every asset/folder beneath a moved subtree.
- Added explicit moved deltas containing stable GUID, old virtual path and new virtual path.
- Kept remap publication and external state persistence transactional with complete rollback after write failure.
- Rejected collisions, root moves and moves into the source's own subtree before mutating live indexes.
- Added a bounded bidirectional asset reference index with deduplication, deterministic queries and reference-aware delete eligibility.
- Expanded the warning-as-error asset gate to 49 checks and passed all build configurations.

## 2026-07-03 - M3.3b transactional asset operations

- Added one Asset Operation Service for folder create/delete and asset rename, move, duplicate and delete.
- Integrated every mutation with central Undo/Redo and stable registry identity remapping.
- Added external byte-exact stash files for duplicate/delete redo and crash-start cleanup limited to owned extensions.
- Refused referenced deletes before touching disk and restored GUID/outgoing references on undo.
- Rejected unsafe names, collisions, root mutation, nonempty folder deletion and undo storage inside/above Content.
- Fixed stable-ID archive spelling so case-folded `/game` keys serialize with the canonical `/Game` mount and survive restart.
- Kept unimplemented Material creation out of both service and UI.
- Added 35 warning-as-error operation checks plus full registry/transaction/build/runtime gates.

## 2026-07-03 - M3.4 Content Browser model

- Added a UI-independent, generation-synchronized Content Browser model over the real Asset Registry snapshot.
- Added validated `/Game` navigation with back/forward/up history, breadcrumbs and a complete folder-tree projection.
- Added direct-child tile/list items, folder-first deterministic sorting, case-insensitive search and real asset-type filtering.
- Added stable selection by asset GUID, path-based folder selection, replace/add/toggle/range behavior and F2 rename lifecycle state.
- Preserved selection when an asset moves, removed stale targets after deletion and repaired removed current folders to a surviving parent.
- Wired startup and live watcher publications to the model while keeping the visual drawer hidden until its complete interaction layer is connected.
- Added 43 warning-as-error checks including a bounded 100,000-asset synchronization gate, plus all builds and hidden startup.

## 2026-07-03 - M3.5 functional Content Browser drawer

- Added a controller boundary that binds the browser model to transactional create, rename, duplicate, delete and Undo/Redo operations.
- Exposed the retained D2D Content Browser drawer through Ctrl+Space, Window menu and toolbar without creating another HWND.
- Added folder tree, breadcrumbs, navigation history, search, tile/list modes, bounded scroll and item counts over live registry generations.
- Added pointer, Ctrl-toggle, Shift-range and keyboard selection, Enter folder activation, Ctrl+A, F2 inline rename and real New Folder creation.
- Added strict UTF-8/UTF-16 conversion for project names and adaptive two-row toolbar/tiny-panel guards.
- Preserved DX12/D2D layer order by painting the drawer before the final menu overlay and routing its input before viewport interaction.
- Deliberately omitted New Material, asset opening and destructive Delete because their editor/confirmation backends are not complete yet.
- Added 24 controller checks and expanded shell coverage to 30 checks; all build and runtime-empty-content gates pass.

## 2026-07-03 - M3.6 scene/world foundation

- Added stable-GUID typed scene entities with parent hierarchy, asset identity, visibility, lock and double-precision transforms.
- Added invariant-checked create, subtree remove, reparent, rename and property mutation with atomic rejection of cycles, excessive depth and invalid values.
- Added a versioned `.acescene` archive using the existing checksummed container and atomic file replacement.
- Added indexed parent-to-children queries plus selection reconciliation and searchable expand/collapse hierarchy projection.
- Verified a 20,001-row broad hierarchy remains bounded instead of degrading to repeated whole-world scans.
- Replaced hardcoded Outliner content with the live scene model, selection, expansion and wheel scrolling.
- Replaced hardcoded Details content with the selected entity's type, GUID, transform, visibility, lock and asset reference.
- Added a transient six-entity editor preview world and synchronized its camera entity with the real viewport without polluting `Content/`.
- Added 36 warning-as-error scene checks, expanded shell coverage to 33 checks and passed all build/runtime gates.

## 2026-07-03 - M3.7a transactional scene transform lifecycle

- Added a scene edit controller sharing the central editor transaction history used by asset operations.
- Added transactional entity rename and direct transform mutation with locked/root validation.
- Added interactive multi-entity transform begin/update/commit/cancel semantics modeled after UE viewport tracking boundaries.
- Live drag updates create no history entries; mouse-up creates exactly one and Escape restores every pre-drag transform.
- Deduplicated target IDs, rejected invalid targets before tracking and atomically rolled back partial invalid updates.
- Added 26 warning-as-error lifecycle checks and passed scene regression plus all build configurations.

## 2026-07-03 - M3.7b transform gizmo math foundation

- Added Translate, Rotate and Scale gizmo modes with X/Y/Z, plane and uniform axis masks.
- Added accumulated-delta evaluation so live updates always derive from the drag-start transforms without numeric drift.
- Added world/local translation bases and symmetric translation/rotation/scale snapping.
- Routed multi-selection gizmo updates through the single-transaction tracking lifecycle from M3.7a.
- Added a nonzero scale floor to prevent accidental singular transforms.
- Kept visual handles and input hidden pending real hit proxies and screen-to-world math.
- Added 22 warning-as-error gizmo checks and passed all build configurations.
