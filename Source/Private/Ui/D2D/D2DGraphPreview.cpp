#include "ArhqenCognitionEngine/Ui/D2D/D2DGraphPreview.h"

namespace am::ui
{
    void D2DGraphPreview::setRect(UiRect rect)
    {
        rect_ = rect;
    }

    void D2DGraphPreview::setGraph(std::vector<am::core::AceUiGraphNode> nodes, std::vector<am::core::AceUiGraphEdge> edges)
    {
        nodes_ = std::move(nodes);
        edges_ = std::move(edges);
    }

    std::optional<am::core::AceUiSelection> D2DGraphPreview::hitTest(float x, float y) const
    {
        const UiRect graphRect = rect_.inset({14.0f, 42.0f, 14.0f, 14.0f});

        for (const auto& node : nodes_)
        {
            const auto r = nodeRect(graphRect, node);
            if (r.contains(x, y))
            {
                am::core::AceUiSelection selection;
                selection.kind = node.kind;
                selection.id = node.id;
                selection.label = node.label;
                selection.source = L"graph";
                return selection;
            }
        }

        return std::nullopt;
    }

    void D2DGraphPreview::render(D2DRenderContext& ctx)
    {
        D2DWidgetUtils::fillRounded(ctx, rect_, 18.0f, ctx.brushes.panelDeep, ctx.brushes.borderDim, 1.0f);

        D2DWidgetUtils::drawTextEx(
            ctx,
            L"BELIEF GRAPH PREVIEW",
            FontRole::Small,
            rect_.inset({16.0f, 12.0f, 16.0f, rect_.height() - 34.0f}),
            ctx.brushes.accent
        );

        const UiRect graphRect = rect_.inset({14.0f, 42.0f, 14.0f, 14.0f});

        if (nodes_.empty())
        {
            D2DWidgetUtils::drawTextEx(
                ctx,
                L"No concepts/relations available yet.",
                FontRole::Small,
                graphRect.inset(8.0f),
                ctx.brushes.muted
            );
            return;
        }

        for (const auto& edge : edges_)
        {
            const auto* from = findNode(edge.from);
            const auto* to = findNode(edge.to);

            if (!from || !to)
            {
                continue;
            }

            const auto a = nodeRect(graphRect, *from);
            const auto b = nodeRect(graphRect, *to);
            const D2D1_POINT_2F p0 = D2D1::Point2F((a.left + a.right) * 0.5f, (a.top + a.bottom) * 0.5f);
            const D2D1_POINT_2F p1 = D2D1::Point2F((b.left + b.right) * 0.5f, (b.top + b.bottom) * 0.5f);

            ctx.target->DrawLine(p0, p1, accentBrush(ctx, edge.accentIndex), 1.4f);
        }

        for (const auto& node : nodes_)
        {
            const auto rect = nodeRect(graphRect, node);
            D2DWidgetUtils::fillRounded(ctx, rect, 14.0f, ctx.brushes.panelElevated, accentBrush(ctx, node.accentIndex), 1.5f);
            D2DWidgetUtils::drawTextEx(
                ctx,
                node.label,
                FontRole::Small,
                rect.inset({10.0f, 8.0f, 10.0f, 6.0f}),
                ctx.brushes.text,
                DWRITE_TEXT_ALIGNMENT_CENTER,
                DWRITE_PARAGRAPH_ALIGNMENT_CENTER
            );
        }
    }

    ID2D1Brush* D2DGraphPreview::accentBrush(D2DRenderContext& ctx, int accentIndex) const
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

    UiRect D2DGraphPreview::nodeRect(UiRect graphRect, const am::core::AceUiGraphNode& node) const
    {
        const float w = 96.0f;
        const float h = 44.0f;
        const float x = graphRect.left + graphRect.width() * node.x;
        const float y = graphRect.top + graphRect.height() * node.y;
        return makeUiRect(x - w * 0.5f, y - h * 0.5f, x + w * 0.5f, y + h * 0.5f);
    }

    const am::core::AceUiGraphNode* D2DGraphPreview::findNode(std::uint64_t id) const
    {
        for (const auto& node : nodes_)
        {
            if (node.id == id)
            {
                return &node;
            }
        }

        return nullptr;
    }
}
