#pragma once

#include "ArhqenCognitionEngine/Ui/D2D/D2DWidgetUtils.h"

namespace am::ui
{
    class D2DGraphTooltip
    {
    public:
        void setText(std::wstring text);
        void setPosition(float x, float y);
        void clear();
        bool visible() const;
        void render(D2DRenderContext& ctx);

    private:
        std::wstring text_;
        float x_ = 0.0f;
        float y_ = 0.0f;
    };
}
