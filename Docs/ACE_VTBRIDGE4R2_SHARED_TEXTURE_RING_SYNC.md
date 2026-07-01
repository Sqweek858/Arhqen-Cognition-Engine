# ACE-VTBRIDGE4R2: shared texture ring sync

VTBRIDGE4R2 keeps the VTBRIDGE4 no-fallback rule and hardens the VTBRIDGE4R1 GPU-only path against flicker.

The working path remains:

```text
DX12 viewport texture
-> D3D11On12 wrapped resource
-> shared D3D11 UI texture
-> D2D DeviceContext DrawBitmap
```

R2 changes the shared intermediate from a single texture into a small ring of shared textures. The bridge writes one slot with `CopyResource` and draws a completed slot, so Direct2D no longer samples the same intermediate surface that the bridge is updating.

When the driver supports it, R2 creates the shared slots with `D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX` and uses `IDXGIKeyedMutex::AcquireSync` / `ReleaseSync` around the GPU copy and D2D draw. If keyed mutex creation is not available, the bridge stays GPU-only and uses the ring as a non-keyed compatibility path. It does not fall back to readback and it does not revive `ID2D1HwndRenderTarget`.

New stats:

```text
d2d_shared_buffer_count
d2d_shared_write_index
d2d_shared_draw_index
d2d_shared_bitmap_recreates
d2d_shared_copies
d2d_shared_mutex_acquires
d2d_shared_mutex_contentions
d2d_shared_d2d_flushes
```

Expected healthy path:

```text
viewport_mode=DX12_D2D_DEVICE_CONTEXT
viewport_texture_bridge=GPU_SAMPLED_D2D
viewport_required_interop=D3D11ON12_SHARED_D3D11_TEXTURE
readback_active=false
legacy_fallback=false
selected=shared_ui_d3d11_texture_ring_keyed
```

R2 is intentionally not a visual/UI polish patch. It only touches bridge synchronization, shared intermediate lifetime, and stats that prove the bridge is not sneaking back to CPU readback.
