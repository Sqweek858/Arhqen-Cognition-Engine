#include "ArhqenCognitionEngine/Ui/D2D/D2DAquariumTelemetryWidgets.h"

#include "ArhqenCognitionEngine/Ui/D2D/D2DGlassEffects.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DTextLayoutFoundation.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DWidgetUtils.h"

#include <algorithm>
#include <cmath>

namespace am::ui
{
    void D2DAquariumTelemetryWidgets::RenderOverlay(D2DRenderContext& ctx, UiRect viewportRect, const ace::aquarium_ui::AceAquariumUiSnapshot& snapshot)
    {
        if (!ctx.target || viewportRect.empty())
        {
            return;
        }

        ++stats_.renderCount;
        const float width = std::min(420.0f, std::max(310.0f, viewportRect.width() * 0.26f));
        const UiRect panel = makeUiRect(viewportRect.left + 16.0f, viewportRect.bottom - 138.0f, viewportRect.left + 16.0f + width, viewportRect.bottom - 16.0f);
        RenderPanel(ctx, panel, snapshot);
    }

    void D2DAquariumTelemetryWidgets::RenderPanel(D2DRenderContext& ctx, UiRect rect, const ace::aquarium_ui::AceAquariumUiSnapshot& snapshot)
    {
        if (!ctx.target || rect.empty())
        {
            return;
        }

        D2DGlassMaterial material;
        material.radius = 14.0f;
        material.fillAlpha = 0.42f;
        material.borderAlpha = 0.36f;
        material.glowAlpha = 0.10f;
        material.blurFallbackAlpha = 0.12f;
        D2DGlassEffects::drawGlassPanel(ctx, rect, material);

        D2DTextLayoutFoundation::Draw(ctx, L"Aquarium telemetry", FontRole::Small, makeUiRect(rect.left + 12.0f, rect.top + 8.0f, rect.right - 12.0f, rect.top + 26.0f), ctx.brushes.text);
        const float barTop = rect.top + 34.0f;
        const float barH = 16.0f;
        RenderBar(ctx, makeUiRect(rect.left + 12.0f, barTop, rect.right - 12.0f, barTop + barH), L"Hydration", snapshot.body.hydration, ctx.brushes.accent);
        RenderBar(ctx, makeUiRect(rect.left + 12.0f, barTop + 22.0f, rect.right - 12.0f, barTop + 22.0f + barH), L"Nutrition", snapshot.body.nutrition, ctx.brushes.accentBlue);
        RenderBar(ctx, makeUiRect(rect.left + 12.0f, barTop + 44.0f, rect.right - 12.0f, barTop + 44.0f + barH), L"Integrity", snapshot.body.integrity, ctx.brushes.accentWarm);
        RenderMiniTimeline(ctx, makeUiRect(rect.left + 12.0f, rect.bottom - 22.0f, rect.right - 12.0f, rect.bottom - 10.0f), snapshot);
    }

    void D2DAquariumTelemetryWidgets::RenderBar(D2DRenderContext& ctx, UiRect rect, const std::wstring& label, double value, ID2D1Brush* accent)
    {
        ++stats_.barCount;
        const double clamped = std::clamp(value, 0.0, 1.0);
        D2DWidgetUtils::fillRounded(ctx, rect, 7.0f, ctx.brushes.panelDeep, ctx.brushes.borderDim, 1.0f);
        UiRect fill = rect.inset({2.0f, 2.0f, 2.0f, 2.0f});
        fill.right = fill.left + fill.width() * static_cast<float>(clamped);
        D2DWidgetUtils::fillRounded(ctx, fill, 6.0f, accent);
        D2DTextLayoutFoundation::Draw(ctx, label, FontRole::Small, rect.inset({8.0f, 0.0f, 8.0f, 0.0f}), ctx.brushes.text, D2DTextOverflowMode::Ellipsis, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }

    void D2DAquariumTelemetryWidgets::RenderMiniTimeline(D2DRenderContext& ctx, UiRect rect, const ace::aquarium_ui::AceAquariumUiSnapshot& snapshot)
    {
        ++stats_.timelineCount;
        if (!ctx.target || rect.empty())
        {
            return;
        }
        D2DWidgetUtils::fillRounded(ctx, rect, 5.0f, ctx.brushes.panelDeep);
        const int count = std::clamp<int>(static_cast<int>(snapshot.logLines.size()), 1, 24);
        const float stepW = rect.width() / static_cast<float>(count);
        for (int i = 0; i < count; ++i)
        {
            UiRect tick = makeUiRect(rect.left + i * stepW + 1.0f, rect.top + 2.0f, rect.left + (i + 1) * stepW - 1.0f, rect.bottom - 2.0f);
            D2DWidgetUtils::fillRounded(ctx, tick, 2.0f, (i + snapshot.step) % 3 == 0 ? ctx.brushes.accent : ctx.brushes.borderDim);
        }
    }

    D2DAquariumTelemetryStats D2DAquariumTelemetryWidgets::Stats() const
    {
        return stats_;
    }
}
