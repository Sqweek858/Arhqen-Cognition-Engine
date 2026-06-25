#include "ArhqenCognitionEngine/Ui/D2D/D2DCyberEffects.h"

#include "ArhqenCognitionEngine/Ui/D2D/D2DWidgetUtils.h"

#include <algorithm>

namespace am::ui
{
    namespace
    {
        void setBrushAlpha(ID2D1SolidColorBrush* brush, float alpha)
        {
            if (brush)
            {
                brush->SetOpacity(std::clamp(alpha, 0.0f, 1.0f));
            }
        }

        void restoreBrushAlpha(ID2D1SolidColorBrush* brush)
        {
            if (brush)
            {
                brush->SetOpacity(1.0f);
            }
        }
    }

    void D2DCyberEffects::drawBackgroundGrid(D2DRenderContext& ctx, UiRect rect, float spacing, float alpha)
    {
        if (!ctx.target || !ctx.brushes.border)
        {
            return;
        }

        spacing = std::max(8.0f, spacing);
        setBrushAlpha(ctx.brushes.border, alpha);

        for (float x = rect.left; x <= rect.right; x += spacing)
        {
            ctx.target->DrawLine(D2D1::Point2F(x, rect.top), D2D1::Point2F(x, rect.bottom), ctx.brushes.border, 0.6f);
        }

        for (float y = rect.top; y <= rect.bottom; y += spacing)
        {
            ctx.target->DrawLine(D2D1::Point2F(rect.left, y), D2D1::Point2F(rect.right, y), ctx.brushes.border, 0.6f);
        }

        restoreBrushAlpha(ctx.brushes.border);
    }

    void D2DCyberEffects::drawScanlines(D2DRenderContext& ctx, UiRect rect, float spacing, float alpha)
    {
        if (!ctx.target || !ctx.brushes.panelSoft)
        {
            return;
        }

        spacing = std::max(3.0f, spacing);
        setBrushAlpha(ctx.brushes.panelSoft, alpha);

        for (float y = rect.top; y <= rect.bottom; y += spacing)
        {
            ctx.target->DrawLine(D2D1::Point2F(rect.left, y), D2D1::Point2F(rect.right, y), ctx.brushes.panelSoft, 1.0f);
        }

        restoreBrushAlpha(ctx.brushes.panelSoft);
    }

    void D2DCyberEffects::drawBorderGlow(D2DRenderContext& ctx, UiRect rect, float radius, float alpha)
    {
        if (!ctx.target || !ctx.brushes.accent)
        {
            return;
        }

        radius = std::max(1.0f, radius);
        setBrushAlpha(ctx.brushes.accent, alpha * 0.36f);

        for (int i = 0; i < 4; ++i)
        {
            const float inset = static_cast<float>(i) * 2.0f;
            UiRect glowRect = makeUiRect(rect.left - inset, rect.top - inset, rect.right + inset, rect.bottom + inset);
            ctx.target->DrawRoundedRectangle(D2DWidgetUtils::rounded(glowRect, radius + inset), ctx.brushes.accent, 1.0f);
        }

        restoreBrushAlpha(ctx.brushes.accent);
    }

    void D2DCyberEffects::drawSoftShadow(D2DRenderContext& ctx, UiRect rect, float radius, float alpha)
    {
        if (!ctx.target || !ctx.brushes.panelDeep)
        {
            return;
        }

        radius = std::max(1.0f, radius);
        setBrushAlpha(ctx.brushes.panelDeep, alpha);

        for (int i = 4; i >= 1; --i)
        {
            const float spread = static_cast<float>(i) * 3.0f;
            UiRect shadowRect = makeUiRect(rect.left - spread * 0.4f, rect.top + spread * 0.5f, rect.right + spread * 0.4f, rect.bottom + spread);
            ctx.target->DrawRoundedRectangle(D2DWidgetUtils::rounded(shadowRect, radius + spread), ctx.brushes.panelDeep, 1.0f);
        }

        restoreBrushAlpha(ctx.brushes.panelDeep);
    }

    void D2DCyberEffects::drawCornerTicks(D2DRenderContext& ctx, UiRect rect, float length, float alpha)
    {
        if (!ctx.target || !ctx.brushes.accent)
        {
            return;
        }

        length = std::max(4.0f, length);
        setBrushAlpha(ctx.brushes.accent, alpha);

        ctx.target->DrawLine(D2D1::Point2F(rect.left, rect.top), D2D1::Point2F(rect.left + length, rect.top), ctx.brushes.accent, 1.0f);
        ctx.target->DrawLine(D2D1::Point2F(rect.left, rect.top), D2D1::Point2F(rect.left, rect.top + length), ctx.brushes.accent, 1.0f);

        ctx.target->DrawLine(D2D1::Point2F(rect.right, rect.top), D2D1::Point2F(rect.right - length, rect.top), ctx.brushes.accent, 1.0f);
        ctx.target->DrawLine(D2D1::Point2F(rect.right, rect.top), D2D1::Point2F(rect.right, rect.top + length), ctx.brushes.accent, 1.0f);

        ctx.target->DrawLine(D2D1::Point2F(rect.left, rect.bottom), D2D1::Point2F(rect.left + length, rect.bottom), ctx.brushes.accent, 1.0f);
        ctx.target->DrawLine(D2D1::Point2F(rect.left, rect.bottom), D2D1::Point2F(rect.left, rect.bottom - length), ctx.brushes.accent, 1.0f);

        ctx.target->DrawLine(D2D1::Point2F(rect.right, rect.bottom), D2D1::Point2F(rect.right - length, rect.bottom), ctx.brushes.accent, 1.0f);
        ctx.target->DrawLine(D2D1::Point2F(rect.right, rect.bottom), D2D1::Point2F(rect.right, rect.bottom - length), ctx.brushes.accent, 1.0f);

        restoreBrushAlpha(ctx.brushes.accent);
    }
}
