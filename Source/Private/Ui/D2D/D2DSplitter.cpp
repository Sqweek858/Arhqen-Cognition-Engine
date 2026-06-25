#include "ArhqenCognitionEngine/Ui/D2D/D2DSplitter.h"

namespace am::ui
{
    void D2DSplitter::setRect(UiRect rect)
    {
        rect_ = rect;
    }

    void D2DSplitter::setOrientation(D2DSplitterOrientation orientation)
    {
        orientation_ = orientation;
    }

    void D2DSplitter::setEnabled(bool enabled)
    {
        enabled_ = enabled;
        if (!enabled_)
        {
            dragging_ = false;
            hovered_ = false;
        }
    }

    UiRect D2DSplitter::rect() const
    {
        return rect_;
    }

    D2DSplitterOrientation D2DSplitter::orientation() const
    {
        return orientation_;
    }

    bool D2DSplitter::enabled() const
    {
        return enabled_;
    }

    bool D2DSplitter::dragging() const
    {
        return dragging_;
    }

    bool D2DSplitter::hitTest(float x, float y) const
    {
        return enabled_ && rect_.contains(x, y);
    }

    bool D2DSplitter::onMouseDown(float x, float y)
    {
        if (!hitTest(x, y))
        {
            return false;
        }

        dragging_ = true;
        hovered_ = true;
        dragX_ = x;
        dragY_ = y;
        return true;
    }

    bool D2DSplitter::onMouseMove(float x, float y)
    {
        const bool oldHovered = hovered_;
        hovered_ = hitTest(x, y) || dragging_;

        if (dragging_)
        {
            dragX_ = x;
            dragY_ = y;
            return true;
        }

        return hovered_ != oldHovered;
    }

    bool D2DSplitter::onMouseUp(float x, float y)
    {
        const bool wasDragging = dragging_;
        dragging_ = false;
        hovered_ = hitTest(x, y);
        dragX_ = x;
        dragY_ = y;
        return wasDragging;
    }

    float D2DSplitter::dragX() const
    {
        return dragX_;
    }

    float D2DSplitter::dragY() const
    {
        return dragY_;
    }

    void D2DSplitter::render(D2DRenderContext& ctx)
    {
        if (!enabled_ || rect_.empty())
        {
            return;
        }

        ID2D1Brush* fill = dragging_ ? ctx.brushes.accentBlue : (hovered_ ? ctx.brushes.border : ctx.brushes.borderDim);
        D2DWidgetUtils::fillRounded(ctx, rect_, 3.0f, fill);

        if (orientation_ == D2DSplitterOrientation::Vertical)
        {
            const float center = (rect_.left + rect_.right) * 0.5f;
            UiRect inner{center - 1.0f, rect_.top + 18.0f, center + 1.0f, rect_.bottom - 18.0f};
            D2DWidgetUtils::fillRounded(ctx, inner, 1.0f, ctx.brushes.panelElevated);
        }
        else
        {
            const float center = (rect_.top + rect_.bottom) * 0.5f;
            UiRect inner{rect_.left + 18.0f, center - 1.0f, rect_.right - 18.0f, center + 1.0f};
            D2DWidgetUtils::fillRounded(ctx, inner, 1.0f, ctx.brushes.panelElevated);
        }
    }
}
