#pragma once

#include "ArhqenCognitionEngine/Ui/D2D/D2DWidgetUtils.h"

namespace am::ui
{
    struct SidebarMetric
    {
        std::wstring label;
        std::wstring value;
        int accentIndex = 0;
    };

    class D2DSidebar
    {
    public:
        void setRect(UiRect rect);
        void setMetrics(std::vector<SidebarMetric> metrics);
        void render(D2DRenderContext& ctx);

    private:
        UiRect rect_{};
        std::vector<SidebarMetric> metrics_;
    };
}
