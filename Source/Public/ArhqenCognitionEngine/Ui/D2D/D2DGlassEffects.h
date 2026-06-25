#pragma once

#include "ArhqenCognitionEngine/Ui/D2D/D2DRenderContext.h"

namespace am::ui
{
    struct D2DGlassMaterial
    {
        float radius = 18.0f;
        float fillAlpha = 0.34f;
        float borderAlpha = 0.68f;
        float highlightAlpha = 0.30f;
        float glowAlpha = 0.28f;
        float shadowAlpha = 0.26f;
        float blurFallbackAlpha = 0.16f;
        bool useCornerTicks = true;
    };

    class D2DGlassEffects
    {
    public:
        static void drawGlassPanel(D2DRenderContext& ctx, UiRect rect, const D2DGlassMaterial& material);
        static void drawGlassOverlay(D2DRenderContext& ctx, UiRect rect, float alpha);
        static void drawLayeredShadow(D2DRenderContext& ctx, UiRect rect, float radius, float alpha);
        static void drawInnerHighlight(D2DRenderContext& ctx, UiRect rect, float radius, float alpha);
        static void drawCyanEdgeSweep(D2DRenderContext& ctx, UiRect rect, float radius, float phase, float alpha);
        static void drawBlurFallback(D2DRenderContext& ctx, UiRect rect, float radius, float alpha);
        static void drawDepthSeparator(D2DRenderContext& ctx, UiRect rect, float alpha);
    };
}
