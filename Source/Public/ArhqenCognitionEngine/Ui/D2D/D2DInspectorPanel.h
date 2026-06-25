#pragma once

#include "ArhqenCognitionEngine/Core/AceUiModel.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DPropertyGrid.h"

namespace am::ui
{
    class D2DInspectorPanel
    {
    public:
        void setRect(UiRect rect);
        void setRecord(am::core::AceUiInspectorRecord record);
        void render(D2DRenderContext& ctx);

    private:
        void renderRelated(D2DRenderContext& ctx, UiRect rect);
        ID2D1Brush* accentBrush(D2DRenderContext& ctx, int accentIndex) const;

        UiRect rect_{};
        UiRect bodyRect_{};
        UiRect propertiesRect_{};
        UiRect relatedRect_{};
        am::core::AceUiInspectorRecord record_;
        D2DPropertyGrid propertyGrid_;
    };
}
