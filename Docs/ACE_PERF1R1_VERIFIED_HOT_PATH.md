# ACE-PERF1R1 - Verified Viewport Hot Path

## Purpose

ACE-PERF1R1 makes the actual Environment 3D path visible and changes the readback hot path that the app was really using. The previous PERF1 work added useful support code, but the on-screen path was still `DX12_READBACK`, so the user could not see a meaningful difference. Wonderful, a performance patch that performed mostly as a rumor.

## Architecture

The Environment 3D shell still follows the UE/Slate-style rule used by ACE-UI12:

1. The scene viewport is treated as a paint/layer element.
2. HUD, telemetry, and the docked log console are painted above it.
3. Child HWND clipping is not reintroduced.
4. Full zero-copy remains reserved for a future compositor path that can respect UI layering.

Because the current UI renderer is still an `ID2D1HwndRenderTarget`, the parent-composited path still needs a CPU-visible bitmap. PERF1R1 does not pretend otherwise. Instead, it removes the double-submit/double-fence behavior from the active readback path.

## Hot path change

Old active path:

```text
DX12 render pass submit
wait fence
DX12 readback copy submit
wait fence
Map readback
CopyFromMemory into D2D bitmap
DrawBitmap
```

New active path:

```text
DX12 render pass + readback copy in one command list
wait fence once
Map readback
CopyFromMemory into D2D bitmap
DrawBitmap
```

This is reported as:

```text
DX12_COMBINED_READBACK
```

## Stats added

`stat_rhi` now surfaces the important counters early in the line:

```text
viewport_mode=DX12_COMBINED_READBACK
combinedReadback=<rhi>/<viewport>
blockingFenceWaits=<count>
blockingFenceWaitMs=<ms>
combinedRenderReadbackBytes=<bytes>
```

`stat_fps` now also includes:

```text
viewport_mode=...
combined_readback_frames=...
fence_waits=...
fence_wait_ms=...
```

## Non-goals

- No `stat_frame` command. `stat_fps` remains the FPS/perf entry point.
- No child-HWND overlay regression.
- No resolution downgrade.
- No fake 1k FPS claim.
- No full D2D device-context compositor rewrite in this patch.

## Expected visible result

When Environment 3D renders through the parent-composited UI layer, the top-left viewport label should say:

```text
GPU 3D Environment | combined render+readback ...
```

`stat_rhi` should report:

```text
path=DX12_COMBINED_READBACK
```

If it still says `DX12_READBACK`, the new hot path is not being used.

## Next real architecture step

The real long-term fix is a GPU compositor path: D3D/DXGI/DComp-compatible UI composition where the viewport texture and UI layers are both GPU-side. PERF1R1 removes one obvious synchronization tax from the current architecture, but it does not magically turn an `ID2D1HwndRenderTarget` into a UE-grade GPU compositor. Sadly, physics continues to have legal authority.
