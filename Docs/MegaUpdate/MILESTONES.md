# Mega Update Milestones

This is the initial execution map. Refine mini-milestones after repository/UE audits, but preserve dependency order and safe commit boundaries.

## M0 — Audit, baseline, branch, documentation

- Full repo/worktree/build/test/dependency audit.
- Performance and behavior baseline.
- Ignore/secret/generated-file audit.
- Create `feature/ace-editor-mega-update`.
- Baseline commit and push.
- Finalize architecture map and test gates.

## M1 — Core foundations

- Canonical SI unit library and editor parsing/formatting.
- Stable IDs, asset paths, serialization primitives and versioning.
- Transaction/Undo-Redo foundation.
- Command registry, input routing, style/text/layout foundations.
- Global panel resize and persistent layout primitives.

## M2 — Main Engine editor shell

- Engine mode/window transition.
- Menus/toolbar/viewport/panel shell with only working commands.
- Dock/split/tab/layout persistence foundation.
- Premium camera/navigation and camera-speed controls.
- DPI/text/focus/scroll correctness gates.

## M3 — Content and asset foundation

- Empty sandboxed `Content/` root.
- Asset Registry, metadata, references and file watching.
- Content Browser tree/grid/search/filter/rename/move/delete/create.
- F2, Ctrl+Space, drag/drop and reference-safe operations.
- Asset Editor Host native window and tab lifecycle.

## M4 — Scene editor

- Versioned scene storage.
- Hierarchy/Outliner and Details reflection/property model.
- Selection, multi-select, picking and outline.
- Move/Rotate/Scale gizmos, snapping and world/local space.
- Object/folder/component lifecycle and transaction coverage.

## M5 — Import and mesh assets

- Dependency decision/integration for FBX.
- FBX popup, units/axes/transforms/normals/tangents/material slots.
- Static Mesh asset and renderer integration.
- Mesh Editor, UVs, LODs, sockets and collision tooling.
- Reimport/dependency/reference safety.

## M6 — Shader compiler and material system

- DXC service, include sandbox, reflection, cache, dependencies and hot reload.
- Typed material IR/compiler and Material/Material Function assets.
- Material Graph D2D canvas and initial general node library.
- Custom HLSL, Material Attributes, instances, preview and diagnostics.
- Shader/PSO dashboard and failure recovery.

## M7 — Render Graph and modern rendering

- Render Graph/resource lifecycle/barriers/transient allocation/profiling.
- HDR/GBuffer/depth/velocity/lighting/shadows foundation.
- Required geometry, lighting, editor and debug passes.
- Post-process stack, profiles and volumes.
- View modes and Render Graph Inspector.

## M8 — ACE GI and reflections

- DXR capability/acceleration-structure foundation.
- Screen traces, Surface Cache, Screen Probes and Radiance Cache.
- Final gather, temporal/spatial filtering, reflections and compositing.
- Fallback/scalability/debug modes.
- Performance and artifact acceptance gates.

## M9 — Physics

- UE/Chaos/backend audit and ThirdParty decision.
- ACE physics API/world/fixed step/layers/materials.
- Bodies/shapes/queries/events.
- Constraints, editor details/debug and simulation controls.
- Mesh/landscape collision integration.

## M10 — Landscape

- Landscape asset/chunks/heightfield/renderer/collision.
- Creation UI and basic partition/streaming.
- Minimum 30 real sculpt modes and common brush engine.
- Paint/material layers and Material Graph integration.
- LOD, persistence, undo, streaming and debug gates.

## M11 — Console, diagnostics and profiling

- Clean console lifecycle and retained crash/session logs.
- Concise `stat_fps`/`stat_rhi`; detailed commands separated.
- Extended command registry and on-screen overlays.
- CPU/GPU/RAM/VRAM telemetry.
- Triangle/geometry debugging.
- CPU/GPU timelines, hitch capture, reports and exports.

## M12 — Integration, hardening and handoff

- Cross-system integration and migration cleanup.
- Full test matrix, leak/device-loss/resize/DPI/performance passes.
- Remove dead code and unexposed abandoned paths.
- Final docs, known limitations, usage/testing guide and commit map.
- Final Release build and pushed known-good branch.

