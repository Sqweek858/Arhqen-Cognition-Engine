# ACE-PERF3 - Viewport Readback Cache

## Scope

ACE-PERF3 reduces the worst idle cost of the Environment 3D path without touching
RHI architecture, renderer pipelines, D2D/DWrite styling, log selection, or the
DirectComposition experiments.

The current production-quality path is still:

```text
DX12_COMBINED_READBACK -> D2D_RETAINED_OVERLAY
```

That path preserves ACE's nice D2D UI, but it used to read back the DX12 viewport
on every repaint, including log-console repaints where the camera and Aquarium
world had not changed. The log made the problem visible:

```text
readback_active=true
combined_readback_frames=1796
readback_bytes=6931123200
fence_waits=1796
```

Reading back the same scene again because a D2D overlay blinked is a magnificent
waste of silicon, but apparently software likes performance crimes.

## What changed

ACE now keeps the last D2D viewport bitmap and a small cache key:

- viewport width/height
- Aquarium step index
- scenario name
- debug-truth flag
- quantized camera position/yaw/pitch

If a repaint happens and the key is unchanged, ACE draws the cached D2D viewport
bitmap directly and skips:

- Aquarium primitive rebuild
- DX12 scene render
- readback copy
- blocking fence wait
- `CopyFromMemory` bitmap upload

The path is reported as:

```text
DX12_CACHED_READBACK
```

This means the image originally came from the readback path, but the current
paint did not perform a new readback.

## Non-goals

- No DComp layered UI compositor in this patch.
- No GPU text overlay.
- No child-HWND return.
- No RHI rewrite.
- No renderer pipeline rewrite.
- No log selection/color changes.

## Validation

Run:

```powershell
.\Tools\validate_ace_perf3.ps1
```

Then build in Visual Studio and check:

```text
stat_fps
stat_rhi
```

When the camera/world are idle but the log/UI repaints, the expected behavior is:

```text
viewport_mode: DX12_CACHED_READBACK
readback_active: false
viewport_cache_hits: increasing
combined_readback_frames: not increasing every UI repaint
```

When the camera moves, scenario steps, viewport resizes, or debug truth changes,
the cache key changes and ACE falls back to:

```text
DX12_COMBINED_READBACK
```

for a fresh frame.
