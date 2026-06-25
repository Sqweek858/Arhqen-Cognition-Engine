#pragma once

#include "ArhqenCognitionEngine/Ui/D2D/D2DRenderContext.h"

#include <string>

namespace am::ui
{
    enum class D2DBlurMode
    {
        Disabled,
        FallbackGlass,
        RealDeviceContext
    };

    struct D2DBlurRuntimeStatus
    {
        D2DBlurMode activeMode = D2DBlurMode::FallbackGlass;
        bool realBlurAvailable = false;
        bool fallbackAvailable = true;
        float requestedRadius = 18.0f;
        std::string reason;
    };

    class D2DBlurRuntime
    {
    public:
        static D2DBlurRuntimeStatus evaluate(D2DRenderContext& ctx, float requestedRadius);
        static const char* modeName(D2DBlurMode mode);
    };
}
