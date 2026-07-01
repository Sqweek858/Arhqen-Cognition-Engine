# ACE-VTBRIDGE1 - D2D texture sampling bridge contract

## Scope

ACE-VTBRIDGE1 is a small, source-verified bridge contract step after VTBRIDGE0. It does not switch the app to a new compositor yet. It makes the exact missing implementation visible in stats before touching the D2D renderer.

## UE/Slate implementation pattern used

UE's `SViewport` emits a viewport draw element. The viewport exposes a GPU texture-like render target resource, then Slate batches that resource as a normal UI draw element and paints UI overlays after it. The normal viewport path does not read the scene back to CPU every frame.

ACE now has the first half of that model: the Aquarium DX12 scene color is exported as `viewport_texture_resource=RHI_TEXTURE`. VTBRIDGE1 records the second half that is still missing: the current ACE UI renderer is still a legacy `ID2D1HwndRenderTarget`, so it cannot sample that GPU viewport resource as a D2D/DXGI surface.

## New reported fields

`stat_rhi` and `stat_fps` now report:

- `viewport_texture_resource`
- `viewport_texture_bridge`
- `viewport_ui_renderer`
- `viewport_required_ui_renderer`
- `viewport_required_interop`
- `viewport_bridge_fallback_reason`

Expected current output:

```text
viewport_texture_resource=RHI_TEXTURE
viewport_texture_bridge=READBACK_FALLBACK
viewport_ui_renderer=LEGACY_D2D_HWND_RENDER_TARGET
viewport_required_ui_renderer=D2D_DEVICE_CONTEXT
viewport_required_interop=D3D11ON12_DXGI_SURFACE
viewport_bridge_fallback_reason=ui_renderer_legacy_hwnd_render_target_needs_d2d_device_context_bridge
```

## Why no FPS jump yet

The active renderer still uses the old `ID2D1HwndRenderTarget` path. That path can draw normal D2D/DWrite UI, but it is not the UE-style viewport draw element equivalent. The next real performance step is to add a D2D device-context / D3D11On12 / DXGI-surface path so the UI layer can draw the DX12 scene texture without CPU readback.

## Non-goals

- No renderer rewrite.
- No RHI path switch.
- No GPU debug glyph UI.
- No log selection changes.
- No child HWND overlay return.
- No fake `readback=false` status.

## Validation

Run:

```powershell
.\Tools\validate_ace_vtbridge1.ps1
```

The probe checks that the bridge contract reports the current blocker explicitly and does not add a CPU-copy bridge or GPU text overlay.
