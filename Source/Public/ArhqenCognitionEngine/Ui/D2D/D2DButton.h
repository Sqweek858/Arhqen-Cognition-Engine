#pragma once

#include "ArhqenCognitionEngine/Ui/D2D/D2DWidgetUtils.h"

namespace am::ui
{
    class D2DButton
    {
    public:
        void setRect(UiRect rect);
        void setLabel(std::wstring label);
        void setEnabled(bool enabled);
        void render(D2DRenderContext& ctx);

        bool onMouseMove(float x, float y);
        bool onMouseDown(float x, float y);
        bool onMouseUp(float x, float y);

        bool hovered() const;
        bool pressed() const;
        UiRect rect() const;

    private:
        UiRect rect_{};
        std::wstring label_;
        bool enabled_ = true;
        bool hovered_ = false;
        bool pressed_ = false;
    };
}
