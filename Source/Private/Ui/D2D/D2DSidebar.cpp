#include "ArhqenCognitionEngine/Ui/D2D/D2DSidebar.h"

#include <utility>

namespace am::ui
{
    void D2DSidebar::setRect(UiRect rect)
    {
        rect_ = rect;
    }

    void D2DSidebar::setMetrics(std::vector<SidebarMetric> metrics)
    {
        metrics_ = std::move(metrics);
    }

    void D2DSidebar::render(D2DRenderContext& ctx)
    {
        D2DWidgetUtils::fillRounded(ctx, rect_, 24.0f, ctx.brushes.panelDeep, ctx.brushes.border, 1.0f);

        D2DWidgetUtils::drawTextEx(ctx, L"COGNITIVE STACK", FontRole::BodyStrong, rect_.inset({22.0f, 22.0f, 22.0f, rect_.height() - 58.0f}), ctx.brushes.text);

        const wchar_t* stack =
            L"M6   Controlled Parser\n"
            L"M7   Belief Ledger\n"
            L"M8   Memory Snapshot\n"
            L"M9   Evidence Engine\n"
            L"M10  Question Generator\n"
            L"M13  UI Framework\nM14  Font Engine";

        D2DWidgetUtils::drawText(ctx, stack, ctx.fonts.body, rect_.inset({24.0f, 72.0f, 24.0f, rect_.height() - 250.0f}), ctx.brushes.muted);

        UiRect metricsPanel = rect_.inset({18.0f, 292.0f, 18.0f, 24.0f});
        D2DWidgetUtils::fillRounded(ctx, metricsPanel, 18.0f, ctx.brushes.panel, ctx.brushes.borderDim, 1.0f);

        D2DWidgetUtils::drawText(ctx, L"RUNTIME METRICS", ctx.fonts.small, metricsPanel.inset({16.0f, 14.0f, 16.0f, metricsPanel.height() - 38.0f}), ctx.brushes.text);

        float y = metricsPanel.top + 52.0f;

        for (const auto& metric : metrics_)
        {
            UiRect pill{metricsPanel.left + 14.0f, y, metricsPanel.right - 14.0f, y + 34.0f};
            ID2D1Brush* accent = ctx.brushes.accent;

            if (metric.accentIndex == 1)
            {
                accent = ctx.brushes.accentBlue;
            }
            else if (metric.accentIndex == 2)
            {
                accent = ctx.brushes.accentWarm;
            }

            D2DWidgetUtils::drawMetricPill(ctx, pill, metric.label, metric.value, accent);
            y += 42.0f;

            if (y > metricsPanel.bottom - 42.0f)
            {
                break;
            }
        }
    }
}
