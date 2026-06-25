#include "ArhqenCognitionEngine/Ui/D2D/D2DGraphInteraction.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace am::ui
{
    void D2DGraphInteraction::setViewportRect(UiRect rect)
    {
        viewportRect_ = rect;
    }

    void D2DGraphInteraction::setContent(std::vector<am::core::AceUiGraphNode> nodes, std::vector<am::core::AceUiGraphEdge> edges)
    {
        nodes_ = std::move(nodes);
        edges_ = std::move(edges);

        if (selectedNode_ != 0 && !findNode(selectedNode_))
        {
            selectedNode_ = 0;
        }

        if (hoveredNode_ != 0 && !findNode(hoveredNode_))
        {
            hoveredNode_ = 0;
        }
    }

    const std::vector<am::core::AceUiGraphNode>& D2DGraphInteraction::nodes() const
    {
        return nodes_;
    }

    const std::vector<am::core::AceUiGraphEdge>& D2DGraphInteraction::edges() const
    {
        return edges_;
    }

    am::core::AceUiGraphStats D2DGraphInteraction::stats() const
    {
        am::core::AceUiGraphStats stats;
        stats.nodeCount = nodes_.size();
        stats.edgeCount = edges_.size();
        stats.zoom = zoom_;
        stats.panX = panX_;
        stats.panY = panY_;
        return stats;
    }

    am::core::AceUiGraphViewport D2DGraphInteraction::viewport() const
    {
        am::core::AceUiGraphViewport viewport;
        viewport.panX = panX_;
        viewport.panY = panY_;
        viewport.zoom = zoom_;
        viewport.hoveredNode = hoveredNode_;
        viewport.selectedNode = selectedNode_;
        viewport.dragging = draggingCanvas_ || draggingNode_;
        return viewport;
    }

    void D2DGraphInteraction::resetView()
    {
        panX_ = 0.0f;
        panY_ = 0.0f;
        zoom_ = 1.0f;
        draggingCanvas_ = false;
        draggingNode_ = false;
        hoveredNode_ = 0;
    }

    void D2DGraphInteraction::fitToView()
    {
        if (nodes_.empty())
        {
            resetView();
            return;
        }

        float minX = std::numeric_limits<float>::max();
        float minY = std::numeric_limits<float>::max();
        float maxX = std::numeric_limits<float>::lowest();
        float maxY = std::numeric_limits<float>::lowest();

        for (const auto& node : nodes_)
        {
            minX = std::min(minX, node.x);
            minY = std::min(minY, node.y);
            maxX = std::max(maxX, node.x);
            maxY = std::max(maxY, node.y);
        }

        const float worldWidth = std::max(0.1f, maxX - minX);
        const float worldHeight = std::max(0.1f, maxY - minY);
        const float scaleX = viewportRect_.width() / (worldWidth * 420.0f);
        const float scaleY = viewportRect_.height() / (worldHeight * 320.0f);

        zoom_ = std::clamp(std::min(scaleX, scaleY), 0.55f, 1.8f);
        const float centerX = (minX + maxX) * 0.5f;
        const float centerY = (minY + maxY) * 0.5f;
        const auto screen = worldToScreen(centerX, centerY);
        panX_ += (viewportRect_.left + viewportRect_.width() * 0.5f) - screen.x;
        panY_ += (viewportRect_.top + viewportRect_.height() * 0.5f) - screen.y;
        clampZoom();
    }

    void D2DGraphInteraction::setSelectedNode(std::uint64_t id)
    {
        selectedNode_ = findNode(id) ? id : 0;
    }

    void D2DGraphInteraction::clearSelection()
    {
        selectedNode_ = 0;
    }

    bool D2DGraphInteraction::onMouseDown(float x, float y)
    {
        if (!viewportRect_.contains(x, y))
        {
            return false;
        }

        const auto hit = hitTest(x, y);
        dragStartX_ = x;
        dragStartY_ = y;
        dragOriginPanX_ = panX_;
        dragOriginPanY_ = panY_;

        if (hit.nodeId != 0)
        {
            selectedNode_ = hit.nodeId;
            draggingNode_ = false;
            draggingCanvas_ = false;
            return true;
        }

        draggingCanvas_ = true;
        draggingNode_ = false;
        return true;
    }

    bool D2DGraphInteraction::onMouseMove(float x, float y)
    {
        if (draggingCanvas_)
        {
            panX_ = dragOriginPanX_ + (x - dragStartX_);
            panY_ = dragOriginPanY_ + (y - dragStartY_);
            updateHover(x, y);
            return true;
        }

        const auto oldHover = hoveredNode_;
        updateHover(x, y);
        return oldHover != hoveredNode_;
    }

    bool D2DGraphInteraction::onMouseUp(float x, float y)
    {
        const bool wasDragging = draggingCanvas_ || draggingNode_;
        draggingCanvas_ = false;
        draggingNode_ = false;
        updateHover(x, y);
        return wasDragging;
    }

    bool D2DGraphInteraction::onMouseWheel(float x, float y, int wheelDelta)
    {
        if (!viewportRect_.contains(x, y))
        {
            return false;
        }

        const auto before = screenToWorld(x, y);
        const float factor = wheelDelta > 0 ? 1.12f : 0.88f;
        zoom_ *= factor;
        clampZoom();

        const auto after = screenToWorld(x, y);
        panX_ += (after.x - before.x) * 420.0f * zoom_;
        panY_ += (after.y - before.y) * 320.0f * zoom_;
        updateHover(x, y);
        return true;
    }

    bool D2DGraphInteraction::onKeyDown(WPARAM key)
    {
        if (key == '0')
        {
            resetView();
            return true;
        }

        if (key == 'F')
        {
            fitToView();
            return true;
        }

        if (key == VK_ESCAPE)
        {
            clearSelection();
            return true;
        }

        const float panStep = 24.0f;
        if (key == VK_LEFT)
        {
            panX_ += panStep;
            return true;
        }

        if (key == VK_RIGHT)
        {
            panX_ -= panStep;
            return true;
        }

        if (key == VK_UP)
        {
            panY_ += panStep;
            return true;
        }

        if (key == VK_DOWN)
        {
            panY_ -= panStep;
            return true;
        }

        return false;
    }

    D2DGraphHitResult D2DGraphInteraction::hitTest(float x, float y) const
    {
        D2DGraphHitResult result;

        if (!viewportRect_.contains(x, y))
        {
            return result;
        }

        for (const auto& node : nodes_)
        {
            const auto rect = nodeScreenRect(node);
            if (rect.contains(x, y))
            {
                result.hit = true;
                result.nodeId = node.id;
                result.selection.kind = node.kind;
                result.selection.id = node.id;
                result.selection.label = node.label;
                result.selection.source = L"graph_canvas";
                result.tooltip = node.label + L" • node #" + std::to_wstring(node.id);
                return result;
            }
        }

        const D2D1_POINT_2F p = D2D1::Point2F(x, y);
        for (const auto& edge : edges_)
        {
            const auto* from = findNode(edge.from);
            const auto* to = findNode(edge.to);
            if (!from || !to)
            {
                continue;
            }

            const auto a = worldToScreen(from->x, from->y);
            const auto b = worldToScreen(to->x, to->y);
            const float distance = distanceToSegment(p, a, b);

            if (distance < 8.0f)
            {
                result.hit = true;
                result.edgeFrom = edge.from;
                result.edgeTo = edge.to;
                result.tooltip = edge.label.empty() ? L"edge" : edge.label;
                return result;
            }
        }

        return result;
    }

    std::optional<am::core::AceUiSelection> D2DGraphInteraction::selectedSelection() const
    {
        const auto* node = findNode(selectedNode_);
        if (!node)
        {
            return std::nullopt;
        }

        am::core::AceUiSelection selection;
        selection.kind = node->kind;
        selection.id = node->id;
        selection.label = node->label;
        selection.source = L"graph_canvas";
        return selection;
    }

    std::uint64_t D2DGraphInteraction::hoveredNode() const
    {
        return hoveredNode_;
    }

    std::uint64_t D2DGraphInteraction::selectedNode() const
    {
        return selectedNode_;
    }

    bool D2DGraphInteraction::dragging() const
    {
        return draggingCanvas_ || draggingNode_;
    }

    D2D1_POINT_2F D2DGraphInteraction::worldToScreen(float x, float y) const
    {
        const float graphLeft = viewportRect_.left + viewportRect_.width() * 0.5f;
        const float graphTop = viewportRect_.top + viewportRect_.height() * 0.5f;
        return D2D1::Point2F(
            graphLeft + panX_ + (x - 0.5f) * 420.0f * zoom_,
            graphTop + panY_ + (y - 0.5f) * 320.0f * zoom_
        );
    }

    D2DGraphPoint D2DGraphInteraction::screenToWorld(float x, float y) const
    {
        const float graphLeft = viewportRect_.left + viewportRect_.width() * 0.5f;
        const float graphTop = viewportRect_.top + viewportRect_.height() * 0.5f;

        D2DGraphPoint result;
        result.x = ((x - graphLeft - panX_) / (420.0f * zoom_)) + 0.5f;
        result.y = ((y - graphTop - panY_) / (320.0f * zoom_)) + 0.5f;
        return result;
    }

    UiRect D2DGraphInteraction::nodeScreenRect(const am::core::AceUiGraphNode& node) const
    {
        const auto p = worldToScreen(node.x, node.y);
        const float w = 108.0f * std::clamp(zoom_, 0.75f, 1.25f);
        const float h = 46.0f * std::clamp(zoom_, 0.75f, 1.20f);
        return makeUiRect(p.x - w * 0.5f, p.y - h * 0.5f, p.x + w * 0.5f, p.y + h * 0.5f);
    }

    const am::core::AceUiGraphNode* D2DGraphInteraction::findNode(std::uint64_t id) const
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

    float D2DGraphInteraction::distanceToSegment(D2D1_POINT_2F p, D2D1_POINT_2F a, D2D1_POINT_2F b) const
    {
        const float vx = b.x - a.x;
        const float vy = b.y - a.y;
        const float wx = p.x - a.x;
        const float wy = p.y - a.y;
        const float lengthSq = vx * vx + vy * vy;

        if (lengthSq <= 0.0001f)
        {
            const float dx = p.x - a.x;
            const float dy = p.y - a.y;
            return std::sqrt(dx * dx + dy * dy);
        }

        const float t = std::clamp((wx * vx + wy * vy) / lengthSq, 0.0f, 1.0f);
        const float projX = a.x + t * vx;
        const float projY = a.y + t * vy;
        const float dx = p.x - projX;
        const float dy = p.y - projY;
        return std::sqrt(dx * dx + dy * dy);
    }

    void D2DGraphInteraction::clampZoom()
    {
        zoom_ = std::clamp(zoom_, 0.35f, 3.25f);
    }

    void D2DGraphInteraction::updateHover(float x, float y)
    {
        const auto hit = hitTest(x, y);
        hoveredNode_ = hit.nodeId;
    }
}
