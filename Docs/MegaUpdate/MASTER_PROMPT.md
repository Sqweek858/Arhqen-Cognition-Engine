# ACE Editor Mega Update — Master Execution Prompt

## Activation

This prompt is dormant until the user explicitly says `pornește mega-update-ul` or an unmistakable equivalent. Creating or editing these planning documents is not activation. Once activated, continue autonomously while safe, in scope, and technically productive.

## Mission

Transform Arhqen Cognition Engine into a compact but real UE-inspired editor/runtime foundation. Deliver functional editor UI, asset workflows, scene editing, material/shader compilation, mesh tooling, rendering/post-processing/GI, physics, landscape, console/debugging, persistence, tests, and documentation described in `FEATURE_SPECIFICATION.md`.

The goal is not visual cosplay. Do not expose controls, asset types, menu items, modes, statistics, or commands without a working backend. An incomplete feature stays unexposed and documented.

## Canonical paths

- ACE repository: `C:\Users\Sqweek\Documents\ArhqenCognitionEngine\ArhqenCognitionEngine`
- UE 5.7 source reference: `C:\Users\Sqweek\Documents\UE_5.7\Engine\Source`
- User content root: `C:\Users\Sqweek\Documents\ArhqenCognitionEngine\ArhqenCognitionEngine\Content`
- Mega-update documentation: `C:\Users\Sqweek\Documents\ArhqenCognitionEngine\ArhqenCognitionEngine\Docs\MegaUpdate`

Never lose these paths during context compaction. Re-read this file and `CURRENT_STATUS.md` after any context loss.

## Mandatory reading order on activation

1. `MASTER_PROMPT.md`
2. `FEATURE_SPECIFICATION.md`
3. `CURRENT_STATUS.md`
4. `MILESTONES.md`
5. `DECISIONS.md`
6. `TEST_MATRIX.md`
7. `RECOVERY.md`
8. Current Git status, history, remotes, ignore rules, build configuration, and relevant existing docs/tests.

## Git safety and persistence

1. Audit the entire worktree before changing branches or staging files.
2. Preserve all relevant local source/config/tool/doc changes. Do not commit builds, logs, caches, binaries, temporary files, credentials, tokens, machine-local secrets, or accidental generated data.
3. Create/switch to branch `feature/ace-editor-mega-update` only after the audit.
4. Establish a clean, explicit baseline commit containing the relevant current local state.
5. Push the baseline to the existing Git remote.
6. Work in small, reviewable mini-milestones.
7. At the end of every successful mini-milestone: inspect diff, run proportional tests, compile required configurations, audit edge cases, update docs, commit intentionally, and push the branch.
8. Never use `git reset --hard`, destructive checkout, or unverified recursive deletion.
9. Recover through known-good commits, `git revert`, a safety branch, or a new corrective commit.
10. Never overwrite user work or silently stage unrelated files.
11. Do not create a PR unless the user later asks.

## Context-loss protocol

`CURRENT_STATUS.md` is operational memory and must stay accurate. Update it:

- before starting a mini-milestone;
- after any major architectural decision;
- before a risky migration;
- after tests/builds;
- immediately before commit/push;
- whenever stopping or encountering a blocker.

It must always state:

- active macro/mini milestone;
- current branch and latest known-good commit;
- files/systems changed;
- what works;
- what remains;
- known failures/risks;
- tests run and exact results;
- next concrete actions.

Update `DECISIONS.md`, `UE_REFERENCE_MAP.md`, `TEST_MATRIX.md`, `PERFORMANCE_BASELINE.md`, and `CHANGELOG.md` as the work evolves. Do not rely on conversational memory.

## Research policy

- For UE-like architecture, inspect UE 5.7 source before designing the ACE equivalent.
- Record useful UE source paths and conclusions in `UE_REFERENCE_MAP.md`.
- Adapt architecture and logic; do not blindly copy large implementation bodies or drag UE bloat into ACE.
- Inspect the exact ThirdParty libraries and integration patterns used by UE.
- Prefer compatible UE dependencies when licensing, redistribution, build size, and scope are reasonable.
- For massive custom UE systems such as Chaos, preserve a clean ACE-facing architecture and use a mature ThirdParty backend if reproducing the solver would endanger quality.
- When UE does not provide a clear/appropriate direction, research primary, official, or otherwise authoritative technical sources online. Record links and decisions.
- Never paste uncertain internet code into production without understanding, adaptation, tests, and license review.

## Implementation principles

- Correctness and recoverability before feature count.
- Real vertical slices before broad placeholders.
- Central reusable systems instead of one-off panel/widget hacks.
- Typed, versioned data and stable IDs.
- Explicit ownership and lifecycle for windows, resources, assets, scene objects, commands, and async jobs.
- Event/invalidation-driven UI; no needless full-window redraw or per-frame rebuilding.
- No per-frame allocations, shader compilation, PSO creation, resource recreation, layout rebuild, or disk access in hot paths unless explicitly justified and measured.
- Preserve the current working renderer/UI behavior until a replacement passes its acceptance gates.
- Feature flags may protect incomplete internals, but incomplete features remain absent from user-facing UI.
- Use Release measurements for performance conclusions and Debug/validation layers for correctness.

## UI quality bar

The editor UI is new and must look deliberate and premium.

- Study Slate layout, invalidation, widget hierarchy, focus, input routing, text shaping, clipping, virtualization, docking, command, and style concepts.
- Preserve the ACE visual identity while adopting proven UE editor ergonomics.
- Text must have stable baseline, spacing, wrapping, clipping, ellipsis, DPI behavior, selection, caret, hit testing, and font fallback.
- No random line gaps, blurry half-pixel placement, clipped glyphs, broken selection highlights, stale text layouts, or inconsistent padding.
- Use cached text layouts and virtualized lists where needed.
- Every popup, menu, tab, splitter, scroll area, graph node, tooltip, and details row must have correct focus/capture/close behavior.
- Global panel resize is direct from edges, cursor-correct, persistent, smooth, and reusable across all panel types.
- Verify normal DPI and non-default Windows scaling.
- Avoid copying UE visual controls that have no ACE function.

## Premium interaction and movement bar

- Camera flight, walking/navigation, orbit, pan, zoom, gizmos, scrolling, graph panning, docking, and landscape strokes must feel smooth and intentional.
- Input accumulation/consumption must have one owner and one update path.
- Use high-resolution timing, raw relative mouse where appropriate, frame-rate-independent integration, stable delta handling, acceleration/deceleration curves, and configurable sensitivity.
- Avoid teleporting, chunky movement, duplicate processing, event-repeat coupling, quantization, and layout/render work triggered by camera keys.
- Camera speed uses time-aware scroll momentum and logarithmic scaling over the specified range.
- Add sensible damping/response curves without adding latency or floaty behavior.
- Test at low, variable, and very high frame rates.

## Units

- Canonical SI internals: meters, kilograms, seconds, radians, newtons, and appropriate SI photometric/temperature units.
- Editor accepts and converts common metric and imperial display/input units.
- Unit conversion is centralized and used by transforms, grids, snapping, camera, FBX, physics, landscape, lighting, and details fields.

## Dependency policy

- Audit UE 5.7 ThirdParty choices first.
- Maintain a `ThirdParty` integration with pinned versions, licenses, build instructions, hashes/source provenance, and update notes.
- Do not download opaque binaries without provenance.
- Do not commit secrets or machine-local absolute dependency caches.
- Jolt or another mature backend is acceptable if a practical Chaos reproduction is not justified.
- Prefer the UE FBX foundation when licensing/redistribution permits; otherwise choose a robust dedicated importer and document the deviation.

## Milestone discipline

Follow `MILESTONES.md`. A mini-milestone is complete only when:

- implementation is real and integrated;
- user-visible incomplete controls are absent;
- formatting/static checks pass;
- relevant unit/integration/probe tests pass;
- Debug and/or Release builds pass as required by risk;
- new warnings are resolved or justified;
- edge cases and lifecycle paths are audited;
- performance regressions are measured;
- docs/status/test matrix/changelog are updated;
- diff is reviewed;
- commit is created and pushed.

Do not stack multiple unstable subsystems into one commit merely to move faster.

## Testing expectations

- Create tests and probes whenever they reduce uncertainty or protect subtle behavior.
- Favor deterministic unit tests for math, serialization, graph compilation, asset references, transactions, units, physics queries, and landscape operations.
- Add integration tests for window/input/resource lifecycle and renderer passes.
- Add golden/reference rendering tests where stable and useful.
- Exercise empty, malformed, missing, duplicate, huge, tiny, resized, minimized, device-lost, compile-failed, import-failed, undo/redo, and recovery paths.
- Keep tests fast enough for mini-milestone gates and maintain a slower full suite for macro gates.

## Performance expectations

- Capture and preserve a baseline before major changes.
- Keep simple-scene RHI throughput and presentation telemetry separate from full editor/UI throughput.
- Track CPU frame, GPU frame, UI, present, memory, allocations, draw calls, triangles, shader/PSO cache, and hitches.
- Use rolling active windows that do not confuse idle gaps with frame stalls.
- Heavy GI/post effects use explicit scalability profiles and honest budgets.
- Profiling instrumentation must be cheap when disabled.

## Scope exclusions

- No skeletal animation system in this mega-update.
- No Niagara/particle editor in this mega-update.
- No cloth, ragdoll, or destruction dependent on excluded animation systems.
- No fake Nanite, fake Zen, fake revision control, fake platform manager, or disabled UE-lookalike controls.

## Completion

The macro milestone is complete only when all exposed features in `FEATURE_SPECIFICATION.md` are functional, persisted, testable, documented, and integrated without regressing the existing AI-details workflow. Anything unfinished remains unexposed and is documented with its safe commit boundary and next steps.

