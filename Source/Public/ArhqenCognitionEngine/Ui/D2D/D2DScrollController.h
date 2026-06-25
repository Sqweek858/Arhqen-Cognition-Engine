#pragma once

#include "ArhqenCognitionEngine/Ui/D2D/D2DUiTypes.h"

namespace am::ui
{
    class D2DScrollController
    {
    public:
        void setViewport(float viewportHeight);
        void setContentHeight(float contentHeight);

        void scrollBy(float delta);
        void scrollToBottom();
        void jumpTo(float offset);
        void update(float dtSeconds);

        float offset() const;
        float targetOffset() const;
        float maxOffset() const;
        bool atBottom() const;
        bool animating() const;
        void setSmoothEnabled(bool enabled);

    private:
        void clamp();

        float viewportHeight_ = 1.0f;
        float contentHeight_ = 1.0f;
        float offset_ = 0.0f;
        float targetOffset_ = 0.0f;
        bool smoothEnabled_ = true;
    };
}
