#pragma once

#include "ArhqenCognitionEngine/Ui/D2D/D2DGraphForceLayout.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DGraphInteraction.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DGraphMinimap.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DGraphLegend.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DGraphTooltip.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DGraphSelectionTrail.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DWidgetUtils.h"

namespace am::ui
{
    class D2DGraphCanvas
    {
    public:
        void setRect(UiRect rect);
        void setGraph(std::vector<am::core::AceUiGraphNode> nodes, std::vector<am::core::AceUiGraphEdge> edges);

        bool onMouseDown(float x, float y);
        bool onMouseMove(float x, float y);
        bool onMouseUp(float x, float y);
        bool onMouseWheel(float x, float y, int wheelDelta);
        bool onKeyDown(WPARAM key);

        std::optional<am::core::AceUiSelection> takeSelection();
        am::core::AceUiGraphStats stats() const;
        void applyForceLayout();
        void resetView();
        void fitToView();

        void render(D2DRenderContext& ctx);

    private:
        void renderGrid(D2DRenderContext& ctx, UiRect viewport);
        void renderEdges(D2DRenderContext& ctx);
        void renderNodes(D2DRenderContext& ctx);
        void renderHud(D2DRenderContext& ctx);
        void renderEdgeLabel(D2DRenderContext& ctx, const am::core::AceUiGraphEdge& edge, D2D1_POINT_2F a, D2D1_POINT_2F b);
        ID2D1Brush* accentBrush(D2DRenderContext& ctx, int accentIndex) const;

        UiRect rect_{};
        UiRect viewportRect_{};
        UiRect minimapRect_{};
        D2DGraphInteraction interaction_;
        D2DGraphMinimap minimap_;
        D2DGraphLegend legend_;
        D2DGraphTooltip tooltip_;
        D2DGraphSelectionTrail selectionTrail_;
        std::optional<am::core::AceUiSelection> pendingSelection_;
        bool hasGraph_ = false;
    };
}
