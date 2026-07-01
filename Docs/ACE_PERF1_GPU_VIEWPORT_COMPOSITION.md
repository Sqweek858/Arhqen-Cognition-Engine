# ACE-PERF1 / ACE-UI13 - GPU Viewport Composition Pass

## Goal

ACE-PERF1 moves the Environment 3D viewport away from the worst version of the
old path: full UI repaint + per-frame GPU vertex copy + per-frame readback buffer
allocation. The target is the same architecture UE/Slate points at with
`SViewport` and SlateRHI: the viewport is a paint/layer element, not a child HWND
that fights the shell.

This milestone does **not** promise a final GPU UI compositor yet. It removes the
most self-inflicted stalls while keeping the UI layering that UI12 fixed.

## UE source notes used

The UE source supplied for this patch shows these relevant ideas:

- `SViewport::OnPaint` emits a viewport draw element via `FSlateDrawElement::MakeViewport`.
- `SViewport` invalidates paint when its viewport interface changes instead of rebuilding the whole UI tree.
- SlateRHI batches window elements, resolves viewport geometry, and treats the viewport as part of the window render list.
- Dynamic Slate geometry is updated through CPU-visible transient buffers rather than a tiny copy/fence ritual per UI element.
- When the viewport is not ready, Slate keeps a stable fallback element instead of changing native window topology mid-paint.

ACE copies the architecture, not Epic's code: stable viewport paint layer,
retained chrome, CPU-visible dynamic viewport vertices, persistent readback
staging, and `stat_fps`/`stat_rhi` counters that reveal which path is active.

## Changes

### 1. Mapped dynamic viewport vertices

The Aquarium viewport mesh now uses a persistently mapped upload vertex buffer.
For this debug scene the mesh is tiny, so a GPU-only vertex buffer plus
`CopyBufferRegion` command list every frame is worse than useless. It added a
fence point before the actual scene render, like asking a courier to file tax
forms before crossing the street.

Counters added to `Dx12GpuAllocationStats`:

- `mappedUploadBytes`
- `mappedUploadUpdates`

### 2. Persistent readback staging

The readback fallback still exists because ACE does not yet have a full DX12 UI
compositor. But the readback buffer is now reused while the viewport extent and
format stay the same.

Counters added:

- `readbackBufferReuses`
- `readbackBufferResizes`

This removes per-frame readback resource allocation churn.

### 3. Fast viewport repaint path

When RMB/WASD or runtime ticks invalidate only the viewport layer, ACE now skips
repainting the whole chrome. It repaints only:

1. viewport scene element
2. viewport HUD/telemetry
3. engine log console if open

Side panels, topbar and logs panel are retained by the D2D HWND render target.
This mirrors the Slate idea that a viewport update should not automatically mean
"rebuild every decorative rectangle humans have invented".

Counters added to `stat_ui`:

- `fast_viewport_paints`
- `full_viewport_paints`

### 4. No child-HWND regression

The old child-HWND clipping workaround stays dead. DirectComposition zero-copy is
still disabled for Environment 3D while ACE needs D2D overlays above the scene.
The fast path optimizes the parent-composited route instead of reopening the
z-order pit.

## Updated `stat_rhi`

`stat_rhi` now includes:

```text
mappedUploadBytes=...
mappedUploadUpdates=...
readbackBufferReuses=...
readbackBufferResizes=...
```

## Updated `stat_ui`

`stat_ui` now includes:

```text
fast_viewport_paints=...
full_viewport_paints=...
```

## Non-goals

- No material system.
- No lighting system.
- No terrain/assets/gizmos/particles.
- No GPU timestamp queries.
- No full UE-style render thread yet.
- No hidden return to child HWND over D2D UI.
- No new `stat_frame`; `stat_fps` remains the user-facing timing command.

## Validation

Run:

```powershell
.\Tools\validate_ace_perf1.ps1
.\Tools\validate_ace_ui12.ps1
.\Tools\validate_ace_perf0.ps1
.\Scripts\build_release.ps1
```

Manual app check:

1. Open Environment 3D.
2. Run `stat_rhi` once.
3. Move camera with RMB/WASD.
4. Open/close the log console with backtick.
5. Run `stat_rhi` and `stat_ui` again.
6. Confirm:
   - telemetry and log console stay above the scene,
   - no flicker/regression to child-HWND overlay,
   - `mappedUploadUpdates` increases,
   - `readbackBufferReuses` increases after the first resize,
   - `fast_viewport_paints` increases during viewport-only updates.

## Reading performance

Use existing commands:

- `stat_fps` for frame timing.
- `stat_rhi` for RHI/resource path counters.
- `stat_ui` for retained/dirty/fast paint counters.

Interpretation:

- `fast_viewport_paints` high + FPS still low: remaining bottleneck is likely
  readback/composite/present or synchronous RHI queue waits.
- `readbackBufferReuses` high: resource allocation churn is gone.
- `mappedUploadUpdates` high while `uploadAllocations` stays lower: per-frame
  vertex copy command lists are gone for the Aquarium viewport.
- CPU/GPU usage low + FPS low after this patch: suspect message-pump pacing,
  D2D present behavior, or the remaining synchronous readback barrier.


## ACE-PERF1R1 verified hot path

PERF1R1 adds `DX12_COMBINED_READBACK`, `submitAndReadbackBgra8`, and visible `stat_rhi` / `stat_fps` counters for the active Environment 3D path. The readback fallback now records the scene render pass and readback copy into one command list instead of paying two blocking fence waits per frame. See `Docs/ACE_PERF1R1_VERIFIED_HOT_PATH.md`.
