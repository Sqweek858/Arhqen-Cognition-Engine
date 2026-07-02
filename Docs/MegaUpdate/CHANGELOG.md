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
