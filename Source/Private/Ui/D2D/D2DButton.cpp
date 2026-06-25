#include "ArhqenCognitionEngine/Ui/D2D/D2DButton.h"

#include <utility>

namespace am::ui
{
    void D2DButton::setRect(UiRect rect)
    {
        rect_ = rect;
    }

    void D2DButton::setLabel(std::wstring label)
    {
        label_ = std::move(label);
    }

    void D2DButton::setEnabled(bool enabled)
    {
        enabled_ = enabled;
        if (!enabled_)
        {
            hovered_ = false;
            pressed_ = false;
        }
    }

    void D2DButton::render(D2DRenderContext& ctx)
    {
        ID2D1Brush* fill = hovered_ || pressed_ ? ctx.brushes.buttonHoverGradient : ctx.brushes.buttonGradient;
        ID2D1Brush* stroke = hovered_ ? ctx.brushes.accent : ctx.brushes.border;
        const float strokeWidth = hovered_ ? 2.0f : 1.0f;

        if (!enabled_)
        {
            fill = ctx.brushes.panelDeep;
            stroke = ctx.brushes.borderDim;
        }

        D2DWidgetUtils::fillRounded(ctx, rect_, 16.0f, fill, stroke, strokeWidth);

        UiRect shine = rect_.inset({2.0f, 2.0f, 2.0f, rect_.height() * 0.62f});
        D2DWidgetUtils::fillRounded(ctx, shine, 14.0f, ctx.brushes.panelSoft, nullptr, 0.0f);

        D2DWidgetUtils::drawTextEx(
            ctx,
            label_,
            FontRole::Button,
            rect_,
            enabled_ ? ctx.brushes.text : ctx.brushes.muted,
            DWRITE_TEXT_ALIGNMENT_CENTER,
            DWRITE_PARAGRAPH_ALIGNMENT_CENTER
        );
    }

    bool D2DButton::onMouseMove(float x, float y)
    {
        const bool next = enabled_ && rect_.contains(x, y);
        const bool changed = next != hovered_;
        hovered_ = next;
        return changed;
    }

    bool D2DButton::onMouseDown(float x, float y)
    {
        if (!enabled_ || !rect_.contains(x, y))
        {
            return false;
        }

        pressed_ = true;
        return true;
    }

    bool D2DButton::onMouseUp(float x, float y)
    {
        const bool clicked = enabled_ && pressed_ && rect_.contains(x, y);
        pressed_ = false;
        return clicked;
    }

    bool D2DButton::hovered() const
    {
        return hovered_;
    }

    bool D2DButton::pressed() const
    {
        return pressed_;
    }

    UiRect D2DButton::rect() const
    {
        return rect_;
    }
}
