#include "ArhqenCognitionEngine/Ui/D2D/D2DGraphCanvas.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace am::ui
{
    void D2DGraphCanvas::setRect(UiRect rect)
    {
        rect_ = rect;
        viewportRect_ = rect_.inset({12.0f, 40.0f, 12.0f, 12.0f});
        minimapRect_ = makeUiRect(rect_.right - 132.0f, rect_.top + 52.0f, rect_.right - 22.0f, rect_.top + 148.0f);

        interaction_.setViewportRect(viewportRect_);
        minimap_.setRect(minimapRect_);
        selectionTrail_.setRect(makeUiRect(rect_.left + 16.0f, rect_.top + 52.0f, rect_.left + 178.0f, rect_.top + 222.0f));
        legend_.setRect(makeUiRect(rect_.right - 164.0f, rect_.top + 158.0f, rect_.right - 22.0f, rect_.top + 292.0f));
    }

    void D2DGraphCanvas::setGraph(std::vector<am::core::AceUiGraphNode> nodes, std::vector<am::core::AceUiGraphEdge> edges)
    {
        hasGraph_ = !nodes.empty();
        interaction_.setContent(std::move(nodes), std::move(edges));
    }

    bool D2DGraphCanvas::onMouseDown(float x, float y)
    {
        if (!rect_.contains(x, y))
        {
            return false;
        }

        if (interaction_.onMouseDown(x, y))
        {
            if (auto selection = interaction_.selectedSelection())
            {
                pendingSelection_ = *selection;
                selectionTrail_.push(*selection);
            }

            return true;
        }

        return true;
    }

    bool D2DGraphCanvas::onMouseMove(float x, float y)
    {
        if (!rect_.contains(x, y) && !interaction_.dragging())
        {
            tooltip_.clear();
            return false;
        }

        const bool changed = interaction_.onMouseMove(x, y);
        const auto hit = interaction_.hitTest(x, y);

        if (hit.hit && !hit.tooltip.empty())
        {
            tooltip_.setText(hit.tooltip);
            tooltip_.setPosition(x, y);
        }
        else
        {
            tooltip_.clear();
        }

        return changed || hit.hit;
    }

    bool D2DGraphCanvas::onMouseUp(float x, float y)
    {
        return interaction_.onMouseUp(x, y);
    }

    bool D2DGraphCanvas::onMouseWheel(float x, float y, int wheelDelta)
    {
        return interaction_.onMouseWheel(x, y, wheelDelta);
    }

    bool D2DGraphCanvas::onKeyDown(WPARAM key)
    {
        if (key == 'L')
        {
            applyForceLayout();
            return true;
        }

        const bool handled = interaction_.onKeyDown(key);
        if (handled)
        {
            if (auto selection = interaction_.selectedSelection())
            {
                pendingSelection_ = *selection;
                selectionTrail_.push(*selection);
            }
        }

        return handled;
    }

    std::optional<am::core::AceUiSelection> D2DGraphCanvas::takeSelection()
    {
        auto result = pendingSelection_;
        pendingSelection_.reset();
        return result;
    }

    am::core::AceUiGraphStats D2DGraphCanvas::stats() const
    {
        return interaction_.stats();
    }

    void D2DGraphCanvas::applyForceLayout()
    {
        auto nodes = interaction_.nodes();
        auto edges = interaction_.edges();
        D2DGraphForceLayout::apply(nodes, edges);
        interaction_.setContent(std::move(nodes), std::move(edges));
    }

    void D2DGraphCanvas::resetView()
    {
        interaction_.resetView();
    }

    void D2DGraphCanvas::fitToView()
    {
        interaction_.fitToView();
    }

    void D2DGraphCanvas::render(D2DRenderContext& ctx)
    {
        D2DWidgetUtils::fillRounded(ctx, rect_, 18.0f, ctx.brushes.panelDeep, ctx.brushes.borderDim, 1.0f);

        D2DWidgetUtils::drawTextEx(
            ctx,
            L"INTERACTIVE GRAPH",
            FontRole::Small,
            rect_.inset({16.0f, 12.0f, 16.0f, rect_.height() - 34.0f}),
            ctx.brushes.accentBlue
        );

        if (!hasGraph_)
        {
            D2DWidgetUtils::drawTextEx(
                ctx,
                L"No graph data yet. Add concepts/relations, then refresh workspace. Computers adore prerequisites.",
                FontRole::Small,
                viewportRect_.inset(12.0f),
                ctx.brushes.muted
            );
            return;
        }

        ctx.target->PushAxisAlignedClip(viewportRect_.d2d(), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        renderGrid(ctx, viewportRect_);
        renderEdges(ctx);
        renderNodes(ctx);
        ctx.target->PopAxisAlignedClip();

        // M22 clean shell keeps the graph readable first. Minimap/legend/trail remain
        // available as components, but they no longer crowd the small right-panel canvas.
        renderHud(ctx);
        tooltip_.render(ctx);
    }

    void D2DGraphCanvas::renderGrid(D2DRenderContext& ctx, UiRect viewport)
    {
        const float spacing = 42.0f;
        const float phaseX = std::fmod(interaction_.viewport().panX, spacing);
        const float phaseY = std::fmod(interaction_.viewport().panY, spacing);

        for (float x = viewport.left + phaseX; x < viewport.right; x += spacing)
        {
            ctx.target->DrawLine(D2D1::Point2F(x, viewport.top), D2D1::Point2F(x, viewport.bottom), ctx.brushes.borderDim, 0.65f);
        }

        for (float y = viewport.top + phaseY; y < viewport.bottom; y += spacing)
        {
            ctx.target->DrawLine(D2D1::Point2F(viewport.left, y), D2D1::Point2F(viewport.right, y), ctx.brushes.borderDim, 0.65f);
        }
    }

    void D2DGraphCanvas::renderEdges(D2DRenderContext& ctx)
    {
        const auto& nodes = interaction_.nodes();

        auto findNode = [&](std::uint64_t id) -> const am::core::AceUiGraphNode*
        {
            for (const auto& node : nodes)
            {
                if (node.id == id)
                {
                    return &node;
                }
            }

            return nullptr;
        };

        for (const auto& edge : interaction_.edges())
        {
            const auto* from = findNode(edge.from);
            const auto* to = findNode(edge.to);

            if (!from || !to)
            {
                continue;
            }

            const auto a = interaction_.worldToScreen(from->x, from->y);
            const auto b = interaction_.worldToScreen(to->x, to->y);

            ctx.target->DrawLine(a, b, accentBrush(ctx, edge.accentIndex), 1.5f);
            renderEdgeLabel(ctx, edge, a, b);
        }
    }

    void D2DGraphCanvas::renderNodes(D2DRenderContext& ctx)
    {
        const auto viewport = interaction_.viewport();

        for (const auto& node : interaction_.nodes())
        {
            const auto rect = interaction_.nodeScreenRect(node);

            if (rect.right < viewportRect_.left || rect.left > viewportRect_.right ||
                rect.bottom < viewportRect_.top || rect.top > viewportRect_.bottom)
            {
                continue;
            }

            const bool selected = node.id == viewport.selectedNode;
            const bool hovered = node.id == viewport.hoveredNode;
            ID2D1Brush* fill = selected ? ctx.brushes.panelSoft : (hovered ? ctx.brushes.panelElevated : ctx.brushes.panel);
            ID2D1Brush* stroke = selected ? ctx.brushes.accentWarm : (hovered ? ctx.brushes.accentBlue : accentBrush(ctx, node.accentIndex));

            D2DWidgetUtils::fillRounded(ctx, rect, 16.0f, fill, stroke, selected ? 2.0f : 1.2f);

            UiRect stripe{rect.left + 8.0f, rect.top + 8.0f, rect.left + 12.0f, rect.bottom - 8.0f};
            D2DWidgetUtils::fillRounded(ctx, stripe, 2.0f, accentBrush(ctx, node.accentIndex));

            D2DWidgetUtils::drawTextEx(
                ctx,
                node.label,
                FontRole::Small,
                rect.inset({16.0f, 8.0f, 12.0f, 8.0f}),
                ctx.brushes.text,
                DWRITE_TEXT_ALIGNMENT_CENTER,
                DWRITE_PARAGRAPH_ALIGNMENT_CENTER
            );
        }
    }

    void D2DGraphCanvas::renderHud(D2DRenderContext& ctx)
    {
        const auto stats = interaction_.stats();
        std::wstring line =
            L"nodes=" + std::to_wstring(stats.nodeCount) +
            L" edges=" + std::to_wstring(stats.edgeCount) +
            L" zoom=" + std::to_wstring(stats.zoom).substr(0, 4) +
            L"  | wheel zoom, drag pan, F fit, 0 reset, L layout";

        UiRect hud{rect_.left + 16.0f, rect_.bottom - 34.0f, rect_.right - 16.0f, rect_.bottom - 10.0f};
        D2DWidgetUtils::fillRounded(ctx, hud, 10.0f, ctx.brushes.panelElevated, ctx.brushes.borderDim, 1.0f);
        D2DWidgetUtils::drawTextEx(ctx, line, FontRole::Small, hud.inset({10.0f, 5.0f, 10.0f, 4.0f}), ctx.brushes.muted);
    }

    void D2DGraphCanvas::renderEdgeLabel(D2DRenderContext& ctx, const am::core::AceUiGraphEdge& edge, D2D1_POINT_2F a, D2D1_POINT_2F b)
    {
        if (edge.label.empty())
        {
            return;
        }

        const float midX = (a.x + b.x) * 0.5f;
        const float midY = (a.y + b.y) * 0.5f;
        const float width = std::clamp(28.0f + static_cast<float>(edge.label.size()) * 6.2f, 58.0f, 160.0f);
        UiRect label{midX - width * 0.5f, midY - 11.0f, midX + width * 0.5f, midY + 11.0f};

        if (label.right < viewportRect_.left || label.left > viewportRect_.right ||
            label.bottom < viewportRect_.top || label.top > viewportRect_.bottom)
        {
            return;
        }

        D2DWidgetUtils::fillRounded(ctx, label, 9.0f, ctx.brushes.panelDeep, ctx.brushes.borderDim, 0.8f);
        D2DWidgetUtils::drawTextEx(ctx, edge.label, FontRole::Small, label.inset({6.0f, 3.0f, 6.0f, 2.0f}), ctx.brushes.muted, DWRITE_TEXT_ALIGNMENT_CENTER);
    }

    ID2D1Brush* D2DGraphCanvas::accentBrush(D2DRenderContext& ctx, int accentIndex) const
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
