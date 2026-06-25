#include "ArhqenCognitionEngine/Ui/D2D/D2DGraphMinimap.h"

namespace am::ui
{
    void D2DGraphMinimap::setRect(UiRect rect)
    {
        rect_ = rect;
    }

    void D2DGraphMinimap::render(D2DRenderContext& ctx, const D2DGraphInteraction& graph)
    {
        if (rect_.empty())
        {
            return;
        }

        D2DWidgetUtils::fillRounded(ctx, rect_, 12.0f, ctx.brushes.panelDeep, ctx.brushes.borderDim, 1.0f);
        D2DWidgetUtils::drawTextEx(ctx, L"MAP", FontRole::Small, rect_.inset({8.0f, 6.0f, 8.0f, rect_.height() - 24.0f}), ctx.brushes.muted);

        const auto& nodes = graph.nodes();
        const auto& edges = graph.edges();

        for (const auto& edge : edges)
        {
            const auto* from = [&]() -> const am::core::AceUiGraphNode*
            {
                for (const auto& node : nodes)
                {
                    if (node.id == edge.from) { return &node; }
                }
                return nullptr;
            }();

            const auto* to = [&]() -> const am::core::AceUiGraphNode*
            {
                for (const auto& node : nodes)
                {
                    if (node.id == edge.to) { return &node; }
                }
                return nullptr;
            }();

            if (!from || !to)
            {
                continue;
            }

            ctx.target->DrawLine(minimapPoint(*from), minimapPoint(*to), ctx.brushes.border, 1.0f);
        }

        for (const auto& node : nodes)
        {
            const auto point = minimapPoint(node);
            const float radius = node.id == graph.selectedNode() ? 4.5f : 3.0f;
            ctx.target->FillEllipse(D2D1::Ellipse(point, radius, radius), accentBrush(ctx, node.accentIndex));
        }
    }

    D2D1_POINT_2F D2DGraphMinimap::minimapPoint(const am::core::AceUiGraphNode& node) const
    {
        const float left = rect_.left + 12.0f;
        const float top = rect_.top + 28.0f;
        const float width = rect_.width() - 24.0f;
        const float height = rect_.height() - 38.0f;

        return D2D1::Point2F(
            left + width * node.x,
            top + height * node.y
        );
    }

    ID2D1Brush* D2DGraphMinimap::accentBrush(D2DRenderContext& ctx, int accentIndex) const
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
