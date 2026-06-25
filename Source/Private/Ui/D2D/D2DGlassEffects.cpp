#include "ArhqenCognitionEngine/Ui/D2D/D2DGlassEffects.h"

#include "ArhqenCognitionEngine/Ui/D2D/D2DCyberEffects.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DWidgetUtils.h"

#include <algorithm>
#include <cmath>

namespace am::ui
{
    namespace
    {
        void setOpacity(ID2D1Brush* brush, float alpha)
        {
            if (brush)
            {
                brush->SetOpacity(std::clamp(alpha, 0.0f, 1.0f));
            }
        }

        void restoreOpacity(ID2D1Brush* brush)
        {
            if (brush)
            {
                brush->SetOpacity(1.0f);
            }
        }
    }

    void D2DGlassEffects::drawGlassPanel(D2DRenderContext& ctx, UiRect rect, const D2DGlassMaterial& material)
    {
        if (!ctx.target)
        {
            return;
        }

        drawLayeredShadow(ctx, rect, material.radius, material.shadowAlpha);
        drawBlurFallback(ctx, rect, material.radius, material.blurFallbackAlpha);

        if (ctx.brushes.panel)
        {
            setOpacity(ctx.brushes.panel, material.fillAlpha);
            D2DWidgetUtils::fillRounded(ctx, rect, material.radius, ctx.brushes.panel);
            restoreOpacity(ctx.brushes.panel);
        }

        D2DCyberEffects::drawBorderGlow(ctx, rect, material.radius, material.glowAlpha);

        if (ctx.brushes.accent)
        {
            setOpacity(ctx.brushes.accent, material.borderAlpha);
            ctx.target->DrawRoundedRectangle(D2DWidgetUtils::rounded(rect, material.radius), ctx.brushes.accent, 0.9f);
            restoreOpacity(ctx.brushes.accent);
        }

        drawInnerHighlight(ctx, rect, material.radius, material.highlightAlpha);

        if (material.useCornerTicks)
        {
            D2DCyberEffects::drawCornerTicks(ctx, rect.inset(2.0f), 11.0f, material.borderAlpha * 0.48f);
        }
    }

    void D2DGlassEffects::drawGlassOverlay(D2DRenderContext& ctx, UiRect rect, float alpha)
    {
        if (!ctx.target || !ctx.brushes.panelDeep)
        {
            return;
        }

        setOpacity(ctx.brushes.panelDeep, alpha);
        D2DWidgetUtils::fillRounded(ctx, rect, 0.0f, ctx.brushes.panelDeep);
        restoreOpacity(ctx.brushes.panelDeep);

        drawBlurFallback(ctx, rect.inset(22.0f), 26.0f, alpha * 0.42f);
    }

    void D2DGlassEffects::drawLayeredShadow(D2DRenderContext& ctx, UiRect rect, float radius, float alpha)
    {
        if (!ctx.target || !ctx.brushes.panelDeep)
        {
            return;
        }

        for (int i = 6; i >= 1; --i)
        {
            const float spread = static_cast<float>(i) * 4.0f;
            const float yOffset = static_cast<float>(i) * 2.2f;
            UiRect shadowRect = makeUiRect(
                rect.left - spread * 0.60f,
                rect.top + yOffset * 0.30f,
                rect.right + spread * 0.60f,
                rect.bottom + yOffset + spread * 0.45f
            );

            setOpacity(ctx.brushes.panelDeep, alpha * (0.04f + 0.035f * static_cast<float>(7 - i)));
            ctx.target->DrawRoundedRectangle(D2DWidgetUtils::rounded(shadowRect, radius + spread), ctx.brushes.panelDeep, 1.0f);
        }

        restoreOpacity(ctx.brushes.panelDeep);
    }

    void D2DGlassEffects::drawInnerHighlight(D2DRenderContext& ctx, UiRect rect, float radius, float alpha)
    {
        if (!ctx.target || !ctx.brushes.text)
        {
            return;
        }

        const UiRect topEdge = makeUiRect(rect.left + radius * 0.85f, rect.top + 1.5f, rect.right - radius * 0.85f, rect.top + 2.8f);
        setOpacity(ctx.brushes.text, alpha);
        D2DWidgetUtils::fillRounded(ctx, topEdge, 2.0f, ctx.brushes.text);
        restoreOpacity(ctx.brushes.text);
    }

    void D2DGlassEffects::drawCyanEdgeSweep(D2DRenderContext& ctx, UiRect rect, float radius, float phase, float alpha)
    {
        if (!ctx.target || !ctx.brushes.accent)
        {
            return;
        }

        const float sweep = 0.5f + 0.5f * std::sin(phase);
        const float width = std::clamp(rect.width() * 0.25f, 36.0f, 160.0f);
        const float start = rect.left + (rect.width() - width) * sweep;

        UiRect sweepRect = makeUiRect(start, rect.top + 1.0f, start + width, rect.top + 3.0f);
        setOpacity(ctx.brushes.accent, alpha);
        D2DWidgetUtils::fillRounded(ctx, sweepRect, 2.0f, ctx.brushes.accent);
        restoreOpacity(ctx.brushes.accent);

        ctx.target->DrawRoundedRectangle(D2DWidgetUtils::rounded(rect, radius), ctx.brushes.accent, 0.5f);
    }

    void D2DGlassEffects::drawBlurFallback(D2DRenderContext& ctx, UiRect rect, float radius, float alpha)
    {
        if (!ctx.target || !ctx.brushes.accentBlue || !ctx.brushes.accent)
        {
            return;
        }

        // This is not true Gaussian blur. It is a cheap frosted-light fallback for the current HwndRenderTarget renderer.
        // M26D wires the real blur policy separately so the renderer can later switch to DeviceContext effects.
        for (int i = 5; i >= 1; --i)
        {
            const float spread = static_cast<float>(i) * 3.5f;
            UiRect layer = makeUiRect(rect.left - spread, rect.top - spread * 0.45f, rect.right + spread, rect.bottom + spread * 0.75f);
            ID2D1SolidColorBrush* brush = (i % 2 == 0) ? ctx.brushes.accent : ctx.brushes.accentBlue;
            setOpacity(brush, alpha * (0.035f + 0.018f * static_cast<float>(6 - i)));
            ctx.target->FillRoundedRectangle(D2DWidgetUtils::rounded(layer, radius + spread), brush);
        }

        restoreOpacity(ctx.brushes.accent);
        restoreOpacity(ctx.brushes.accentBlue);
    }

    void D2DGlassEffects::drawDepthSeparator(D2DRenderContext& ctx, UiRect rect, float alpha)
    {
        if (!ctx.target || !ctx.brushes.accent)
        {
            return;
        }

        setOpacity(ctx.brushes.accent, alpha);
        D2DWidgetUtils::fillRounded(ctx, rect, 2.0f, ctx.brushes.accent);
        restoreOpacity(ctx.brushes.accent);
    }
}
