#include "ArhqenCognitionEngine/Ui/D2D/D2DWorkspacePanel.h"

#include <algorithm>
#include <utility>

namespace am::ui
{
    void D2DWorkspacePanel::setRect(UiRect rect)
    {
        rect_ = rect;

        const float pad = 16.0f;
        const float headerHeight = 64.0f;
        const float metricsHeight = 104.0f;
        const float gap = 12.0f;

        metricsRect_ = makeUiRect(
            rect_.left + pad,
            rect_.top + headerHeight,
            rect_.right - pad,
            rect_.top + headerHeight + metricsHeight
        );

        graphRect_ = makeUiRect(
            metricsRect_.left,
            metricsRect_.bottom + gap,
            metricsRect_.right,
            rect_.bottom - pad
        );

        // M22 clean shell intentionally removes the cramped concept/question list blocks
        // from the right panel. The inspector below handles item details without choking the graph.
        conceptsRect_ = makeUiRect(0.0f, 0.0f, 0.0f, 0.0f);
        questionsRect_ = makeUiRect(0.0f, 0.0f, 0.0f, 0.0f);

        graphPreview_.setRect(graphRect_);
        graphCanvas_.setRect(graphRect_);
    }

    void D2DWorkspacePanel::setSnapshot(am::core::AceUiSnapshot snapshot)
    {
        snapshot_ = std::move(snapshot);
        graphPreview_.setGraph(snapshot_.graphNodes, snapshot_.graphEdges);
        graphCanvas_.setGraph(snapshot_.graphNodes, snapshot_.graphEdges);
    }

    std::optional<am::core::AceUiSelection> D2DWorkspacePanel::hitTest(float x, float y) const
    {
        if (auto graph = graphPreview_.hitTest(x, y))
        {
            return graph;
        }

        if (auto conceptSelection = hitTestList(conceptsRect_, snapshot_.concepts, 4, x, y))
        {
            return conceptSelection;
        }

        const auto& secondary = snapshot_.questions.empty() ? snapshot_.hypotheses : snapshot_.questions;
        if (auto item = hitTestList(questionsRect_, secondary, 5, x, y))
        {
            return item;
        }

        return std::nullopt;
    }


    bool D2DWorkspacePanel::onMouseDown(float x, float y)
    {
        return graphCanvas_.onMouseDown(x, y);
    }

    bool D2DWorkspacePanel::onMouseMove(float x, float y)
    {
        return graphCanvas_.onMouseMove(x, y);
    }

    bool D2DWorkspacePanel::onMouseUp(float x, float y)
    {
        return graphCanvas_.onMouseUp(x, y);
    }

    bool D2DWorkspacePanel::onMouseWheel(float x, float y, int wheelDelta)
    {
        return graphCanvas_.onMouseWheel(x, y, wheelDelta);
    }

    bool D2DWorkspacePanel::onKeyDown(WPARAM key)
    {
        return graphCanvas_.onKeyDown(key);
    }

    std::optional<am::core::AceUiSelection> D2DWorkspacePanel::takeGraphSelection()
    {
        return graphCanvas_.takeSelection();
    }

    am::core::AceUiGraphStats D2DWorkspacePanel::graphStats() const
    {
        return graphCanvas_.stats();
    }


    void D2DWorkspacePanel::render(D2DRenderContext& ctx)
    {
        D2DWidgetUtils::fillRounded(ctx, rect_, 24.0f, ctx.brushes.panel, ctx.brushes.border, 1.0f);

        D2DWidgetUtils::drawTextEx(
            ctx,
            snapshot_.title.empty() ? L"Cognitive Workspace" : snapshot_.title,
            FontRole::BodyStrong,
            rect_.inset({18.0f, 16.0f, 18.0f, rect_.height() - 44.0f}),
            ctx.brushes.text
        );

        D2DWidgetUtils::drawTextEx(
            ctx,
            snapshot_.subtitle.empty() ? L"Backend state preview." : snapshot_.subtitle,
            FontRole::Small,
            rect_.inset({18.0f, 42.0f, 18.0f, rect_.height() - 66.0f}),
            ctx.brushes.muted
        );

        renderMetrics(ctx, metricsRect_);
        graphCanvas_.render(ctx);
    }

    void D2DWorkspacePanel::renderMetrics(D2DRenderContext& ctx, UiRect rect)
    {
        D2DWidgetUtils::fillRounded(ctx, rect, 18.0f, ctx.brushes.panelDeep, ctx.brushes.borderDim, 1.0f);
        D2DWidgetUtils::drawTextEx(ctx, L"STORE METRICS", FontRole::Small, rect.inset({14.0f, 10.0f, 14.0f, rect.height() - 32.0f}), ctx.brushes.accent);

        const float pillW = (rect.width() - 42.0f) * 0.5f;
        float x = rect.left + 14.0f;
        float y = rect.top + 42.0f;

        for (std::size_t i = 0; i < std::min<std::size_t>(snapshot_.metrics.size(), 6); ++i)
        {
            const auto& metric = snapshot_.metrics[i];
            UiRect pill{x, y, x + pillW, y + 28.0f};
            D2DWidgetUtils::drawMetricPill(ctx, pill, metric.label, metric.value, accentBrush(ctx, metric.accentIndex));

            if (i % 2 == 0)
            {
                x += pillW + 14.0f;
            }
            else
            {
                x = rect.left + 14.0f;
                y += 36.0f;
            }
        }
    }

    void D2DWorkspacePanel::renderList(D2DRenderContext& ctx, UiRect rect, const std::wstring& title, const std::vector<am::core::AceUiListItem>& items, std::size_t maxItems)
    {
        D2DWidgetUtils::fillRounded(ctx, rect, 18.0f, ctx.brushes.panelDeep, ctx.brushes.borderDim, 1.0f);
        D2DWidgetUtils::drawTextEx(ctx, title, FontRole::Small, rect.inset({14.0f, 10.0f, 14.0f, rect.height() - 32.0f}), ctx.brushes.accent);

        if (items.empty())
        {
            D2DWidgetUtils::drawTextEx(ctx, L"No items yet.", FontRole::Small, rect.inset({14.0f, 42.0f, 14.0f, 12.0f}), ctx.brushes.muted);
            return;
        }

        float y = rect.top + 38.0f;

        for (std::size_t i = 0; i < std::min(maxItems, items.size()); ++i)
        {
            const auto& item = items[i];
            UiRect row{rect.left + 12.0f, y, rect.right - 12.0f, y + 42.0f};
            D2DWidgetUtils::fillRounded(ctx, row, 12.0f, ctx.brushes.panelElevated, nullptr, 0.0f);

            UiRect stripe{row.left, row.top + 5.0f, row.left + 4.0f, row.bottom - 5.0f};
            D2DWidgetUtils::fillRounded(ctx, stripe, 2.0f, accentBrush(ctx, item.accentIndex));

            D2DWidgetUtils::drawTextEx(ctx, item.title, FontRole::Small, row.inset({12.0f, 5.0f, 12.0f, 21.0f}), ctx.brushes.text);
            D2DWidgetUtils::drawTextEx(ctx, item.subtitle, FontRole::Small, row.inset({12.0f, 23.0f, 12.0f, 4.0f}), ctx.brushes.muted);
            y += 48.0f;

            if (y > rect.bottom - 44.0f)
            {
                break;
            }
        }
    }

    std::optional<am::core::AceUiSelection> D2DWorkspacePanel::hitTestList(UiRect rect, const std::vector<am::core::AceUiListItem>& items, std::size_t maxItems, float x, float y) const
    {
        if (!rect.contains(x, y) || items.empty())
        {
            return std::nullopt;
        }

        float rowY = rect.top + 38.0f;
        const std::size_t count = std::min(maxItems, items.size());

        for (std::size_t i = 0; i < count; ++i)
        {
            UiRect row{rect.left + 12.0f, rowY, rect.right - 12.0f, rowY + 42.0f};
            if (row.contains(x, y))
            {
                const auto& item = items[i];
                am::core::AceUiSelection selection;
                selection.kind = item.kind;
                selection.id = item.id;
                selection.label = item.title;
                selection.source = L"workspace";
                return selection;
            }

            rowY += 48.0f;
        }

        return std::nullopt;
    }

    ID2D1Brush* D2DWorkspacePanel::accentBrush(D2DRenderContext& ctx, int accentIndex) const
    {
        if (accentIndex == 1)
        {
            return ctx.brushes.accentBlue;
        }

        if (accentIndex == 2)
        {
            return ctx.brushes.accentWarm;
        }

        if (accentIndex == 3)
        {
            return ctx.brushes.danger;
        }

        return ctx.brushes.accent;
    }
}
