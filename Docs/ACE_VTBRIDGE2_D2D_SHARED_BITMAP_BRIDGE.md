# ACE-VTBRIDGE2 - D2D Shared Bitmap Viewport Bridge

## Goal

Move the Environment 3D viewport one step closer to the UE/Slate model without
sacrificing the existing D2D/DWrite UI. UE paints a viewport as a GPU texture
UI element; ACE previously exposed the DX12 SceneColor texture but still fell
back to CPU readback because the D2D UI renderer could not sample it.

VTBRIDGE2 adds a first runtime bridge attempt:

- render the DX12 scene into the persistent SceneColor render target;
- transition SceneColor to shader-read state;
- wrap the native D3D12 resource through D3D11On12;
- query an IDXGISurface;
- create an ID2D1Bitmap via CreateSharedBitmap;
- draw that bitmap through the existing retained D2D UI path.

The D2D/DWrite overlay, telemetry, log console, fonts and selection UI remain
unchanged. The GPU debug-glyph overlay is not used.

## Runtime modes

Expected successful mode:

- `viewport_mode=DX12_D2D_TEXTURE_BRIDGE`
- `viewport_texture_bridge=GPU_SAMPLED_D2D`
- `readback_active=false`
- `d2dTextureBridgeFrames` increases
- `combinedReadbackFrames` should stop increasing on the normal 3D viewport path

Fallback mode remains explicit:

- `viewport_texture_bridge=READBACK_FALLBACK`
- `viewport_bridge_fallback_reason=<D3D11On12/CreateSharedBitmap failure>`
- `DX12_COMBINED_READBACK` remains available as the safe path

## Non-goals

- No RHI renderer rewrite.
- No child HWND return.
- No GPU-baked text UI.
- No log selection/color changes.
- No DComp layer rewrite in this milestone.

## Validation

Run:

```powershell
.\Tools\validate_ace_vtbridge2.ps1
```

Then build in Visual Studio and check:

```text
stat_rhi
stat_fps
```

If the bridge fails, the fallback reason should be visible rather than hidden
behind a generic readback fallback.
