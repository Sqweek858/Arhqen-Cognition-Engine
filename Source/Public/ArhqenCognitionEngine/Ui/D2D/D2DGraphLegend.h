#pragma once

#include "ArhqenCognitionEngine/Ui/D2D/D2DWidgetUtils.h"

namespace am::ui
{
    struct D2DGraphLegendItem
    {
        std::wstring label;
        std::wstring detail;
        int accentIndex = 0;
    };

    class D2DGraphLegend
    {
    public:
        void setRect(UiRect rect);
        void setItems(std::vector<D2DGraphLegendItem> items);
        void render(D2DRenderContext& ctx);

    private:
        ID2D1Brush* accentBrush(D2DRenderContext& ctx, int accentIndex) const;

        UiRect rect_{};
        std::vector<D2DGraphLegendItem> items_;
    };
}
