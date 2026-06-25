#pragma once

#include "ArhqenCognitionEngine/Ui/D2D/D2DWidgetUtils.h"

namespace am::ui
{
    enum class D2DSplitterOrientation
    {
        Vertical,
        Horizontal
    };

    class D2DSplitter
    {
    public:
        void setRect(UiRect rect);
        void setOrientation(D2DSplitterOrientation orientation);
        void setEnabled(bool enabled);

        UiRect rect() const;
        D2DSplitterOrientation orientation() const;
        bool enabled() const;
        bool dragging() const;
        bool hitTest(float x, float y) const;

        bool onMouseDown(float x, float y);
        bool onMouseMove(float x, float y);
        bool onMouseUp(float x, float y);

        float dragX() const;
        float dragY() const;

        void render(D2DRenderContext& ctx);

    private:
        UiRect rect_{};
        D2DSplitterOrientation orientation_ = D2DSplitterOrientation::Vertical;
        bool enabled_ = true;
        bool hovered_ = false;
        bool dragging_ = false;
        float dragX_ = 0.0f;
        float dragY_ = 0.0f;
    };
}
