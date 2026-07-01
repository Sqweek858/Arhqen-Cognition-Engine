# ACE-VTBRIDGE0 - Viewport Texture Bridge Foundation

## Scop

ACE-VTBRIDGE0 introduce fundația pentru modelul de viewport folosit de UE/Slate: scena 3D este un render target GPU expus ca resursă de UI, nu o imagine citită înapoi pe CPU în fiecare frame.

## Ce adaugă

- `AceViewportTextureResource`: descrie textura GPU a viewport-ului.
- `IAceViewportTextureSource`: contract asemănător cu ideea `ISlateViewport`, dar adaptat ACE.
- `AceViewportTextureBridgeStatus`: raportează dacă UI-ul poate consuma textura GPU sau dacă trebuie fallback readback.
- Aquarium GPU viewport expune `sceneColor_` ca `RHI_TEXTURE`.
- `stat_rhi` / `stat_fps` raportează bridge-ul și motivul fallback-ului.

## Non-goals

- Nu schimbă renderer-ul principal.
- Nu scoate încă readback-ul din path-ul normal.
- Nu desenează UI ca GPU glyphs.
- Nu schimbă D2D/DWrite UI-ul.
- Nu atinge log selection, FPS cache sau child HWND.

## Diagnostic așteptat acum

Până când UI renderer-ul are un backend care poate desena textura GPU direct, valorile normale sunt:

```text
viewport_texture_resource: RHI_TEXTURE
viewport_texture_bridge: READBACK_FALLBACK
viewport_bridge_fallback_reason: ui_renderer_cannot_sample_gpu_viewport_texture_yet
```

Asta este intenționat: patch-ul face problema explicită și pregătește bridge-ul real, în loc să ascundă readback-ul sub încă un cache.

## Direcția următoare

Următorul pas este un UI draw element/backing resource care poate consuma `AceViewportTextureResource` ca textured quad GPU-side, păstrând D2D/DWrite overlay-ul peste viewport.
