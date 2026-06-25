#pragma once

#include "ArhqenCognitionEngine/Ui/D2D/D2DGraphInteraction.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DWidgetUtils.h"

namespace am::ui
{
    class D2DGraphMinimap
    {
    public:
        void setRect(UiRect rect);
        void render(D2DRenderContext& ctx, const D2DGraphInteraction& graph);

    private:
        D2D1_POINT_2F minimapPoint(const am::core::AceUiGraphNode& node) const;
        ID2D1Brush* accentBrush(D2DRenderContext& ctx, int accentIndex) const;

        UiRect rect_{};
    };
}
