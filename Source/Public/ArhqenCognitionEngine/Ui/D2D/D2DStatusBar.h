#pragma once

#include "ArhqenCognitionEngine/Ui/D2D/D2DWidgetUtils.h"

namespace am::ui
{
    class D2DStatusBar
    {
    public:
        void setRect(UiRect rect);
        void setText(std::wstring text);
        void render(D2DRenderContext& ctx);

    private:
        UiRect rect_{};
        std::wstring text_;
    };
}
