#include "ArhqenCognitionEngine/Ui/D2D/D2DToolbar.h"

#include <algorithm>
#include <utility>

namespace am::ui
{
    void D2DToolbar::setRect(UiRect rect)
    {
        rect_ = rect;
    }

    void D2DToolbar::setItems(std::vector<D2DToolbarItem> items)
    {
        items_ = std::move(items);
    }

    void D2DToolbar::setToggled(const std::wstring& id, bool toggled)
    {
        for (auto& item : items_)
        {
            if (item.id == id)
            {
                item.toggled = toggled;
                return;
            }
        }
    }

    void D2DToolbar::setEnabled(const std::wstring& id, bool enabled)
    {
        for (auto& item : items_)
        {
            if (item.id == id)
            {
                item.enabled = enabled;
                return;
            }
        }
    }

    bool D2DToolbar::onMouseMove(float x, float y)
    {
        const auto old = hovered_;
        hovered_ = hitIndex(x, y);
        return hovered_ != old;
    }

    bool D2DToolbar::onMouseDown(float x, float y)
    {
        pressed_ = hitIndex(x, y);

        if (pressed_ < items_.size() && items_[pressed_].enabled)
        {
            return true;
        }

        pressed_ = static_cast<std::size_t>(-1);
        return false;
    }

    bool D2DToolbar::onMouseUp(float x, float y)
    {
        const auto released = hitIndex(x, y);

        if (pressed_ < items_.size() && released == pressed_ && items_[pressed_].enabled)
        {
            activated_ = items_[pressed_].id;
            pressed_ = static_cast<std::size_t>(-1);
            return true;
        }

        pressed_ = static_cast<std::size_t>(-1);
        return false;
    }

    std::optional<std::wstring> D2DToolbar::takeActivated()
    {
        auto result = activated_;
        activated_.reset();
        return result;
    }

    void D2DToolbar::render(D2DRenderContext& ctx)
    {
        D2DWidgetUtils::fillRounded(ctx, rect_, 18.0f, ctx.brushes.panelDeep, ctx.brushes.borderDim, 1.0f);

        if (items_.empty())
        {
            D2DWidgetUtils::drawTextEx(ctx, L"No toolbar items.", FontRole::Small, rect_.inset(12.0f), ctx.brushes.muted);
            return;
        }

        for (std::size_t i = 0; i < items_.size(); ++i)
        {
            const auto& item = items_[i];
            const auto r = itemRect(i);
            const bool hover = i == hovered_;
            const bool press = i == pressed_;
            const bool active = item.toggled || press;

            ID2D1Brush* fill = active ? ctx.brushes.panelSoft : (hover ? ctx.brushes.panelElevated : ctx.brushes.panelDeep);
            ID2D1Brush* stroke = active ? accentBrush(ctx, item.accentIndex) : (hover ? ctx.brushes.border : ctx.brushes.borderDim);

            if (!item.enabled)
            {
                fill = ctx.brushes.panelDeep;
                stroke = ctx.brushes.borderDim;
            }

            D2DWidgetUtils::fillRounded(ctx, r, 13.0f, fill, stroke, active ? 1.6f : 1.0f);

            UiRect stripe{r.left + 7.0f, r.top + 7.0f, r.left + 10.0f, r.bottom - 7.0f};
            D2DWidgetUtils::fillRounded(ctx, stripe, 2.0f, accentBrush(ctx, item.accentIndex));

            const UiRect labelRect = makeUiRect(r.left + 16.0f, r.top + 7.0f, r.right - 10.0f, r.top + 27.0f);
            const UiRect hintRect = makeUiRect(r.left + 16.0f, r.top + 30.0f, r.right - 10.0f, r.bottom - 5.0f);

            D2DWidgetUtils::drawTextEx(
                ctx,
                item.label,
                FontRole::Small,
                labelRect,
                item.enabled ? ctx.brushes.text : ctx.brushes.muted
            );

            D2DWidgetUtils::drawTextEx(
                ctx,
                item.hint,
                FontRole::Small,
                hintRect,
                ctx.brushes.muted
            );
        }
    }

    ID2D1Brush* D2DToolbar::accentBrush(D2DRenderContext& ctx, int accentIndex) const
    {
        if (accentIndex == 1)
        {
            return ctx.brushes.accentBlue;
        }

        if (accentIndex == 2)
        {
            return ctx.brushes.accentWarm;
        }

        if (accentIndex == 3)
        {
            return ctx.brushes.danger;
        }

        return ctx.brushes.accent;
    }

    UiRect D2DToolbar::itemRect(std::size_t index) const
    {
        if (items_.empty())
        {
            return {};
        }

        const float gap = 8.0f;
        const float count = static_cast<float>(std::max<std::size_t>(1, items_.size()));
        const float available = std::max(1.0f, rect_.width() - 18.0f - gap * (count - 1.0f));
        const float width = std::max(84.0f, available / count);
        const float left = rect_.left + 9.0f + static_cast<float>(index) * (width + gap);
        const float right = std::min(left + width, rect_.right - 9.0f);
        const float top = rect_.top + 7.0f;
        const float bottom = rect_.bottom - 7.0f;
        return {left, top, right, bottom};
    }

    std::size_t D2DToolbar::hitIndex(float x, float y) const
    {
        if (!rect_.contains(x, y))
        {
            return static_cast<std::size_t>(-1);
        }

        for (std::size_t i = 0; i < items_.size(); ++i)
        {
            if (itemRect(i).contains(x, y))
            {
                return i;
            }
        }

        return static_cast<std::size_t>(-1);
    }
}
