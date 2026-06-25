#include "ArhqenCognitionEngine/Ui/D2D/D2DTabStrip.h"

#include <utility>

namespace am::ui
{
    void D2DTabStrip::setRect(UiRect rect)
    {
        rect_ = rect;
    }

    void D2DTabStrip::setTabs(std::vector<D2DTabItem> tabs)
    {
        tabs_ = std::move(tabs);

        if (active_.empty() && !tabs_.empty())
        {
            active_ = tabs_.front().id;
        }
    }

    void D2DTabStrip::setActive(std::wstring id)
    {
        active_ = std::move(id);
    }

    const std::wstring& D2DTabStrip::active() const
    {
        return active_;
    }

    bool D2DTabStrip::onMouseMove(float x, float y)
    {
        const auto old = hovered_;
        hovered_ = static_cast<std::size_t>(-1);

        for (std::size_t i = 0; i < tabs_.size(); ++i)
        {
            if (tabRect(i).contains(x, y))
            {
                hovered_ = i;
                break;
            }
        }

        return old != hovered_;
    }

    bool D2DTabStrip::onMouseDown(float x, float y)
    {
        for (std::size_t i = 0; i < tabs_.size(); ++i)
        {
            if (tabRect(i).contains(x, y))
            {
                pressed_ = i;
                return true;
            }
        }

        pressed_ = static_cast<std::size_t>(-1);
        return false;
    }

    bool D2DTabStrip::onMouseUp(float x, float y)
    {
        if (pressed_ == static_cast<std::size_t>(-1))
        {
            return false;
        }

        const auto pressed = pressed_;
        pressed_ = static_cast<std::size_t>(-1);

        if (pressed < tabs_.size() && tabRect(pressed).contains(x, y))
        {
            active_ = tabs_[pressed].id;
            activated_ = active_;
            return true;
        }

        return false;
    }

    std::optional<std::wstring> D2DTabStrip::takeActivated()
    {
        auto result = activated_;
        activated_.reset();
        return result;
    }

    void D2DTabStrip::render(D2DRenderContext& ctx)
    {
        D2DWidgetUtils::fillRounded(ctx, rect_, 16.0f, ctx.brushes.panelDeep, ctx.brushes.borderDim, 1.0f);

        for (std::size_t i = 0; i < tabs_.size(); ++i)
        {
            const auto& tab = tabs_[i];
            const UiRect r = tabRect(i);
            const bool isActive = tab.id == active_;
            const bool isHover = i == hovered_;

            ID2D1Brush* fill = isActive ? ctx.brushes.panelSoft : (isHover ? ctx.brushes.panelElevated : ctx.brushes.panelDeep);
            ID2D1Brush* stroke = isActive ? accentBrush(ctx, tab.accentIndex) : ctx.brushes.borderDim;

            D2DWidgetUtils::fillRounded(ctx, r, 12.0f, fill, stroke, isActive ? 1.4f : 1.0f);

            if (isActive)
            {
                UiRect stripe{r.left + 8.0f, r.bottom - 5.0f, r.right - 8.0f, r.bottom - 2.0f};
                D2DWidgetUtils::fillRounded(ctx, stripe, 2.0f, accentBrush(ctx, tab.accentIndex));
            }

            D2DWidgetUtils::drawTextEx(
                ctx,
                tab.label,
                FontRole::Small,
                r.inset({10.0f, 7.0f, 10.0f, 5.0f}),
                isActive ? ctx.brushes.text : ctx.brushes.muted,
                DWRITE_TEXT_ALIGNMENT_CENTER,
                DWRITE_PARAGRAPH_ALIGNMENT_CENTER
            );
        }
    }

    ID2D1Brush* D2DTabStrip::accentBrush(D2DRenderContext& ctx, int accentIndex) const
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

    UiRect D2DTabStrip::tabRect(std::size_t index) const
    {
        if (tabs_.empty())
        {
            return {};
        }

        const float gap = 6.0f;
        const float w = (rect_.width() - gap * (static_cast<float>(tabs_.size()) + 1.0f)) / static_cast<float>(tabs_.size());
        const float left = rect_.left + gap + static_cast<float>(index) * (w + gap);
        return {left, rect_.top + 5.0f, left + w, rect_.bottom - 5.0f};
    }
}
