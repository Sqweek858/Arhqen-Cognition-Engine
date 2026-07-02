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
