# ACE-PERF2R2 - Stat Console Detail Restore

## Scope

ACE-PERF2R2 restores the diagnostic usefulness of the engine log console after the D2D quality guard. PERF2R1 correctly brought back the polished D2D/DWrite UI, but the log console still displayed long one-line `stat_rhi` and `stat_fps` records. In the docked console those lines were clipped, which made the UI prettier and the diagnostics worse, an excellent way to lose arguments with a machine.

## What changed

- `stat_rhi` now returns a multi-line, human-readable block.
- `stat_fps` now returns a multi-line, human-readable block.
- The compact single-line record is still written for grep/source logs.
- The multi-line body is also written to `Build/Logs/ace_engine.log` as `*_DETAIL` records.
- The docked D2D console wraps long legacy log lines instead of silently clipping them.
- The console keeps the D2D/DWrite visual style restored by PERF2R1.

## `stat_rhi` visible fields

`stat_rhi` now exposes the important path and quality fields on separate lines:

- `path`
- `backend`
- `adapter`
- `viewport_mode`
- `ui_layer`
- `quality`
- `gpu_text_overlay`
- `readback_active`
- zero-copy/readback/gpu-composited frame counters
- combined readback counters
- fence wait counters
- resource allocation counters
- draw and command list counters
- readback bytes and buffer reuse/resize counters

## `stat_fps` visible fields

`stat_fps` now keeps the existing command surface but exposes more detail:

- sample count
- FPS last/avg/min/max
- frame ms last/avg/min/max
- UI/layout/aquarium/RHI/present timings
- fallback path
- viewport mode
- D2D quality guard fields
- readback state
- fence wait counters

## Non-goals

- No new `stat_frame` command.
- No return to GPU block-glyph UI.
- No DirectComposition architecture rewrite in this patch.
- No FPS optimization claims. This milestone restores observability.

## Validation

Run:

```powershell
.\Tools\validate_ace_perf2r2.ps1
```

Then in the app:

1. Open Environment 3D.
2. Press backtick to open the ACE Engine Log Console.
3. Run `stat_rhi`.
4. Run `stat_fps`.
5. Confirm the console shows multi-line detail blocks with `ui_layer=D2D_RETAINED_OVERLAY`, `gpu_text_overlay=false`, and `readback_active` visible.

