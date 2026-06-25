#include "ArhqenCognitionEngine/Ui/D2D/D2DScrollController.h"

#include <algorithm>
#include <cmath>

namespace am::ui
{
    void D2DScrollController::setViewport(float viewportHeight)
    {
        viewportHeight_ = std::max(1.0f, viewportHeight);
        clamp();
    }

    void D2DScrollController::setContentHeight(float contentHeight)
    {
        contentHeight_ = std::max(1.0f, contentHeight);
        clamp();
    }

    void D2DScrollController::scrollBy(float delta)
    {
        targetOffset_ = std::clamp(targetOffset_ + delta, 0.0f, maxOffset());

        if (!smoothEnabled_)
        {
            offset_ = targetOffset_;
        }
    }

    void D2DScrollController::scrollToBottom()
    {
        targetOffset_ = maxOffset();

        if (!smoothEnabled_)
        {
            offset_ = targetOffset_;
        }
    }

    void D2DScrollController::jumpTo(float offset)
    {
        targetOffset_ = std::clamp(offset, 0.0f, maxOffset());
        offset_ = targetOffset_;
    }

    void D2DScrollController::update(float dtSeconds)
    {
        if (!smoothEnabled_)
        {
            offset_ = targetOffset_;
            return;
        }

        const float stiffness = 18.0f;
        const float alpha = 1.0f - std::exp(-stiffness * std::max(0.0f, dtSeconds));
        offset_ += (targetOffset_ - offset_) * alpha;

        if (std::abs(targetOffset_ - offset_) < 0.25f)
        {
            offset_ = targetOffset_;
        }

        clamp();
    }

    float D2DScrollController::offset() const
    {
        return offset_;
    }

    float D2DScrollController::targetOffset() const
    {
        return targetOffset_;
    }

    float D2DScrollController::maxOffset() const
    {
        return std::max(0.0f, contentHeight_ - viewportHeight_);
    }

    bool D2DScrollController::atBottom() const
    {
        return targetOffset_ >= maxOffset() - 1.0f;
    }

    bool D2DScrollController::animating() const
    {
        return std::abs(targetOffset_ - offset_) > 0.25f;
    }

    void D2DScrollController::setSmoothEnabled(bool enabled)
    {
        smoothEnabled_ = enabled;

        if (!smoothEnabled_)
        {
            offset_ = targetOffset_;
        }
    }

    void D2DScrollController::clamp()
    {
        targetOffset_ = std::clamp(targetOffset_, 0.0f, maxOffset());
        offset_ = std::clamp(offset_, 0.0f, maxOffset());
    }
}
