#pragma once

#include "ArhqenCognitionEngine/Ui/D2D/D2DRenderContext.h"

#include <cstdint>
#include <vector>

namespace am::ui
{
    struct D2DFrostedLayer
    {
        UiRect rect{};
        float radius = 0.0f;
        float alpha = 0.0f;
        bool useBlueAccent = false;
    };

    struct D2DEffectCacheStats
    {
        std::uint64_t lookupCount = 0;
        std::uint64_t hitCount = 0;
        std::uint64_t missCount = 0;
        std::uint64_t resetCount = 0;
        std::size_t cachedEntryCount = 0;
    };

    class D2DCachedEffects
    {
    public:
        static void drawCachedBlurFallback(D2DRenderContext& ctx, UiRect rect, float radius, float alpha);
        static void drawCachedAcrylicPanel(D2DRenderContext& ctx, UiRect rect, float radius, float fillAlpha, float glowAlpha);
        static void reset();
        static D2DEffectCacheStats stats();
    };
}
