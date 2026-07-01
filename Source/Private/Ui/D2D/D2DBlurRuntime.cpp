#include "ArhqenCognitionEngine/Ui/D2D/D2DBlurRuntime.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DCachedEffects.h"

namespace am::ui
{
    D2DBlurRuntimeStatus D2DBlurRuntime::evaluate(D2DRenderContext& ctx, float requestedRadius)
    {
        D2DBlurRuntimeStatus status;
        status.requestedRadius = requestedRadius;

        if (!ctx.target)
        {
            status.activeMode = D2DBlurMode::Disabled;
            status.realBlurAvailable = false;
            status.fallbackAvailable = false;
            status.reason = "No Direct2D target is available.";
            return status;
        }

        // Current renderer path uses an ID2D1DeviceContext target. Real Gaussian blur through D2D effects still needs
        // an ID2D1DeviceContext + offscreen bitmap/effect chain or a DirectComposition-backed blur path.
        // M26D records the policy and keeps fallback active until the renderer backend is upgraded.
        const auto cacheStats = D2DCachedEffects::stats();
        status.activeMode = D2DBlurMode::CachedFrostedFallback;
        status.realBlurAvailable = false;
        status.fallbackAvailable = true;
        status.cachedFallbackAvailable = true;
        status.effectCacheHits = cacheStats.hitCount;
        status.effectCacheMisses = cacheStats.missCount;
        status.reason = "Cached frosted fallback active. Real blur still needs an ID2D1DeviceContext/effect-chain renderer upgrade.";
        return status;
    }

    const char* D2DBlurRuntime::modeName(D2DBlurMode mode)
    {
        switch (mode)
        {
        case D2DBlurMode::Disabled:
            return "disabled";
        case D2DBlurMode::FallbackGlass:
            return "fallback_glass";
        case D2DBlurMode::CachedFrostedFallback:
            return "cached_frosted_fallback";
        case D2DBlurMode::RealDeviceContext:
            return "real_device_context";
        default:
            return "unknown";
        }
    }
}
