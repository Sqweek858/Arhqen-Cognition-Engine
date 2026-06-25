#include "ArhqenCognitionEngine/Ui/D2D/D2DGraphTooltip.h"

#include <algorithm>
#include <utility>

namespace am::ui
{
    void D2DGraphTooltip::setText(std::wstring text)
    {
        text_ = std::move(text);
    }

    void D2DGraphTooltip::setPosition(float x, float y)
    {
        x_ = x;
        y_ = y;
    }

    void D2DGraphTooltip::clear()
    {
        text_.clear();
    }

    bool D2DGraphTooltip::visible() const
    {
        return !text_.empty();
    }

    void D2DGraphTooltip::render(D2DRenderContext& ctx)
    {
        if (text_.empty())
        {
            return;
        }

        const float width = std::clamp(24.0f + static_cast<float>(text_.size()) * 7.0f, 160.0f, 360.0f);
        const float height = 44.0f;
        UiRect rect{x_ + 14.0f, y_ + 14.0f, x_ + 14.0f + width, y_ + 14.0f + height};

        if (rect.right > ctx.width - 12.0f)
        {
            rect.left = x_ - width - 14.0f;
            rect.right = x_ - 14.0f;
        }

        if (rect.bottom > ctx.height - 12.0f)
        {
            rect.top = y_ - height - 14.0f;
            rect.bottom = y_ - 14.0f;
        }

        D2DWidgetUtils::fillRounded(ctx, rect, 12.0f, ctx.brushes.panelElevated, ctx.brushes.accentBlue, 1.0f);
        D2DWidgetUtils::drawTextEx(ctx, text_, FontRole::Small, rect.inset({12.0f, 10.0f, 12.0f, 8.0f}), ctx.brushes.text);
    }
}
