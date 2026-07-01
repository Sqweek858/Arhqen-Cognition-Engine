# ACE-VTBRIDGE3 - D2D DeviceContext Compatibility Bitmap Bridge

## Goal
Move the viewport bridge one step closer to the UE/Slate model without changing the
existing D2D/DWrite UI styling. The 3D scene still exposes a native DX12 scene
texture, but the UI path now creates a D2D 1.1 device-context bitmap from the
D3D11On12/DXGI surface first, then shares that bitmap with the legacy HWND render
target.

## Why this exists
VTBRIDGE2 attempted `ID2D1HwndRenderTarget::CreateSharedBitmap` directly from an
`IDXGISurface`. On this app path the renderer still reports
`LEGACY_D2D_HWND_RENDER_TARGET`, so the bridge usually falls back to readback.
VTBRIDGE3 adds the missing D2D device-context sidecar:

1. DX12 SceneColor native resource.
2. D3D11On12 wrapped resource.
3. IDXGISurface query.
4. ID2D1DeviceContext::CreateBitmapFromDxgiSurface.
5. ID2D1HwndRenderTarget::CreateSharedBitmap from the D2D bitmap.
6. DrawBitmap through the existing D2D UI target.

This keeps D2D/DWrite UI quality intact. No GPU debug text overlay is used.

## Fallback
If any step fails, ACE keeps the existing combined readback path and stores the
failing HRESULT in the viewport bridge fallback reason. That is not a final
performance solution, but it makes the remaining blocker concrete instead of
pretending readback is a feature.

## Non-goals
- No full D2D swapchain UI renderer yet.
- No renderer rewrite.
- No log selection, font, color, or console background changes.
- No child HWND reactivation.
- No CPU readback cache changes.

## Validation
Run:

```powershell
.\Tools\validate_ace_vtbridge3.ps1
```

Then in app run `stat_rhi` and `stat_fps`. Desired path:

```text
viewport_mode=DX12_D2D_TEXTURE_BRIDGE
viewport_texture_bridge=GPU_SAMPLED_D2D
readback_active=false
```

Fallback remains acceptable if the log reports the exact D2D/D3D11On12 step that
failed. VTBRIDGE3 forwards the last bridge HRESULT/error into stat_rhi/stat_fps so
the remaining blocker is visible instead of being hidden behind the legacy-target
summary.
