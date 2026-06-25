#pragma once

#include "ArhqenCognitionEngine/Ui/D2D/D2DRenderContext.h"

namespace am::ui
{
    class D2DCyberEffects
    {
    public:
        static void drawBackgroundGrid(D2DRenderContext& ctx, UiRect rect, float spacing, float alpha);
        static void drawScanlines(D2DRenderContext& ctx, UiRect rect, float spacing, float alpha);
        static void drawBorderGlow(D2DRenderContext& ctx, UiRect rect, float radius, float alpha);
        static void drawSoftShadow(D2DRenderContext& ctx, UiRect rect, float radius, float alpha);
        static void drawCornerTicks(D2DRenderContext& ctx, UiRect rect, float length, float alpha);
    };
}
