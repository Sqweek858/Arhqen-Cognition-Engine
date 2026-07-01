# ACE-VTBRIDGE4R1: D2D shared intermediate GPU texture

VTBRIDGE4 proved that the main UI can render through `ID2D1DeviceContext`, but the viewport bridge still failed at `ID2D1DeviceContext::CreateBitmapFromDxgiSurface` when the UI device context tried to consume the raw D3D11On12 wrapped viewport surface.

VTBRIDGE4R1 keeps the no-fallback rule and adds a compatibility path that stays fully GPU-side:

```text
DX12 SceneColor
-> D3D11On12 wrapped resource
-> shared D3D11 texture opened on the D3D11On12 device
-> GPU CopyResource into the shared texture
-> same shared texture surface on the UI D3D11 device
-> ID2D1DeviceContext::CreateBitmapFromDxgiSurface
-> DrawBitmap into the parent D2D DeviceContext target
```

Rules preserved:

- no legacy `ID2D1HwndRenderTarget` fallback
- no combined readback fallback
- no CPU pixel copy fallback
- no `CreateSharedBitmap` back into the old HWND render target
- failure remains fatal and diagnostic

The patch also separates bridge render attempts from actual UI-side bridge success:

```text
d2d_texture_bridge_frames=<DX12 produced GPU texture frames>
d2d_texture_bridge_attempts=<UI bridge draw attempts>
d2d_texture_bridge_successes=<successful UI DrawBitmap bridge frames>
d2d_texture_bridge_direct_successes=<direct wrapped surface path successes>
d2d_texture_bridge_shared_successes=<shared intermediate path successes>
d2d_texture_bridge_surface_diagnostics=<D3D12/DXGI/D2D probe details>
```

If the direct wrapped surface succeeds, R1 uses it. If Direct2D rejects that surface with `D2DERR_UNSUPPORTED_OPERATION`, R1 tries the shared UI-device D3D11 texture instead. If the shared intermediate also fails, the viewport stays in `FAILED_D2D_DEVICE_CONTEXT` with the surface diagnostics attached.
