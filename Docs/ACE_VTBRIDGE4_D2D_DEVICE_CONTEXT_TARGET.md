# ACE-VTBRIDGE4: D2D DeviceContext Target

VTBRIDGE4 replaces the old parent-window `ID2D1HwndRenderTarget` UI target with a modern `ID2D1DeviceContext` backed by the HWND DXGI swapchain backbuffer.

The intended path is:

```text
DXGI swapchain/backbuffer
-> ID2D1DeviceContext target bitmap
-> DrawBitmap(viewport GPU texture from D3D11On12/IDXGISurface)
-> D2D/DWrite UI over the viewport
-> Present
```

## Rule

There is no legacy fallback for the 3D workspace viewport in this milestone.

If the D2D DeviceContext bridge cannot initialize or draw the GPU texture, the viewport reports a failure state instead of silently switching back to CPU readback. Debugging a masked fallback is how software becomes haunted furniture.

## Success stats

```text
viewport_mode=DX12_D2D_DEVICE_CONTEXT
viewport_texture_bridge=GPU_SAMPLED_D2D
viewport_ui_renderer=D2D_DEVICE_CONTEXT
readback_active=false
legacy_fallback=false
combined_readback_frames=0
```

## Failure stats

```text
viewport_mode=FAILED_D2D_DEVICE_CONTEXT
viewport_texture_bridge=FAILED
viewport_ui_renderer=D2D_DEVICE_CONTEXT
readback_active=false
legacy_fallback=false
fatal_bridge_step=<step>
fatal_bridge_hresult=<HRESULT or reason>
```

## Explicitly banned in this path

- `CreateSharedBitmap` into `ID2D1HwndRenderTarget`
- `DX12_COMBINED_READBACK` fallback after bridge failure
- cached CPU readback replay as the active fallback
- child HWND viewport composition
- GPU debug glyph UI as a substitute for D2D/DWrite

## UE source reading anchor

The model here follows the useful part of Unreal Slate rather than the accidental old ACE fallback: a viewport is an ordered draw element carrying a GPU render target texture, and the window renderer batches/composites that texture into the final window target. ACE mirrors that with a D2D DeviceContext target and a GPU-sampled viewport bitmap.
