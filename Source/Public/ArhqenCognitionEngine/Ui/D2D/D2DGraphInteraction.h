#pragma once

#include "ArhqenCognitionEngine/Core/AceUiModel.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DUiTypes.h"

#include <optional>

namespace am::ui
{
    struct D2DGraphPoint
    {
        float x = 0.0f;
        float y = 0.0f;
    };

    struct D2DGraphHitResult
    {
        bool hit = false;
        am::core::AceUiSelection selection;
        std::uint64_t nodeId = 0;
        std::uint64_t edgeFrom = 0;
        std::uint64_t edgeTo = 0;
        std::wstring tooltip;
    };

    class D2DGraphInteraction
    {
    public:
        void setViewportRect(UiRect rect);
        void setContent(std::vector<am::core::AceUiGraphNode> nodes, std::vector<am::core::AceUiGraphEdge> edges);

        const std::vector<am::core::AceUiGraphNode>& nodes() const;
        const std::vector<am::core::AceUiGraphEdge>& edges() const;

        am::core::AceUiGraphStats stats() const;
        am::core::AceUiGraphViewport viewport() const;

        void resetView();
        void fitToView();
        void setSelectedNode(std::uint64_t id);
        void clearSelection();

        bool onMouseDown(float x, float y);
        bool onMouseMove(float x, float y);
        bool onMouseUp(float x, float y);
        bool onMouseWheel(float x, float y, int wheelDelta);
        bool onKeyDown(WPARAM key);

        D2DGraphHitResult hitTest(float x, float y) const;
        std::optional<am::core::AceUiSelection> selectedSelection() const;
        std::uint64_t hoveredNode() const;
        std::uint64_t selectedNode() const;
        bool dragging() const;

        D2D1_POINT_2F worldToScreen(float x, float y) const;
        D2DGraphPoint screenToWorld(float x, float y) const;
        UiRect nodeScreenRect(const am::core::AceUiGraphNode& node) const;

    private:
        const am::core::AceUiGraphNode* findNode(std::uint64_t id) const;
        float distanceToSegment(D2D1_POINT_2F p, D2D1_POINT_2F a, D2D1_POINT_2F b) const;
        void clampZoom();
        void updateHover(float x, float y);

        UiRect viewportRect_{};
        std::vector<am::core::AceUiGraphNode> nodes_;
        std::vector<am::core::AceUiGraphEdge> edges_;

        float panX_ = 0.0f;
        float panY_ = 0.0f;
        float zoom_ = 1.0f;
        float dragStartX_ = 0.0f;
        float dragStartY_ = 0.0f;
        float dragOriginPanX_ = 0.0f;
        float dragOriginPanY_ = 0.0f;
        bool draggingCanvas_ = false;
        bool draggingNode_ = false;
        std::uint64_t hoveredNode_ = 0;
        std::uint64_t selectedNode_ = 0;
    };
}
