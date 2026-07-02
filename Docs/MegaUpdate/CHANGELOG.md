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
