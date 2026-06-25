#include "ArhqenCognitionEngine/Ui/D2D/D2DBlurRuntime.h"

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

        // Current renderer path uses ID2D1HwndRenderTarget. Real Gaussian blur through D2D effects needs
        // an ID2D1DeviceContext + offscreen bitmap/effect chain or a DirectComposition-backed blur path.
        // M26D records the policy and keeps fallback active until the renderer backend is upgraded.
        status.activeMode = D2DBlurMode::FallbackGlass;
        status.realBlurAvailable = false;
        status.fallbackAvailable = true;
        status.reason = "Fallback glass blur active. Real blur requires ID2D1DeviceContext/effect-chain renderer upgrade.";
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
        case D2DBlurMode::RealDeviceContext:
            return "real_device_context";
        default:
            return "unknown";
        }
    }
}
