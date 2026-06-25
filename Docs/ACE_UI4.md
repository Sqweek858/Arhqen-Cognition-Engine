# ACE-UI4 Cached Blur / Acrylic Effects Pass

ACE-UI4 improves the existing glass/acrylic effect path without replacing the renderer. The current shell still uses an `ID2D1HwndRenderTarget`, so this milestone keeps the fallback frosted-glass look but caches repeated layer geometry and exposes effect-cache diagnostics.

## Implemented

- `D2DCachedEffects` caches quantized frosted fallback layer geometry.
- `D2DGlassEffects::drawBlurFallback` routes through the cached effect path.
- `D2DBlurRuntime` reports `cached_frosted_fallback` and cache hit/miss counters.
- `cache_stats` now reports effect cache entries, hits, misses, and display DPI scale.
- Cache resets with D2D device-resource discard.

## Why this exists

The visual direction stays cyberpunk/acrylic, but repeated panels should not rebuild the same fake-blur layer stack every frame. It is not true Gaussian blur yet. It is a stable cached fallback until the renderer grows an `ID2D1DeviceContext` effect chain.

## Non-goals

- No true Gaussian blur yet.
- No DirectComposition dependency.
- No shader/material system.
- No Slate import.

## Next work

A future `ACE-UI4R1` can add a real device-context blur backend while preserving this fallback for older or degraded render paths.
