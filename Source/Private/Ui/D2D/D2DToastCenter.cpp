#include "ArhqenCognitionEngine/Ui/D2D/D2DToastCenter.h"

#include <algorithm>
#include <utility>

namespace am::ui
{
    void D2DToastCenter::setRect(UiRect rect)
    {
        rect_ = rect;
    }

    void D2DToastCenter::push(std::wstring title, std::wstring body, D2DToastKind kind, float ttlSeconds)
    {
        D2DToast toast;
        toast.title = std::move(title);
        toast.body = std::move(body);
        toast.kind = kind;
        toast.ttlSeconds = std::max(0.8f, ttlSeconds);
        toast.id = nextId_++;

        toasts_.push_back(std::move(toast));

        if (toasts_.size() > 5)
        {
            toasts_.erase(toasts_.begin());
        }
    }

    void D2DToastCenter::clear()
    {
        toasts_.clear();
    }

    void D2DToastCenter::update(float dtSeconds)
    {
        for (auto& toast : toasts_)
        {
            toast.ageSeconds += std::max(0.0f, dtSeconds);
        }

        toasts_.erase(
            std::remove_if(
                toasts_.begin(),
                toasts_.end(),
                [](const D2DToast& toast)
                {
                    return toast.ageSeconds >= toast.ttlSeconds;
                }),
            toasts_.end()
        );
    }

    bool D2DToastCenter::hasActiveToasts() const
    {
        return !toasts_.empty();
    }

    std::size_t D2DToastCenter::size() const
    {
        return toasts_.size();
    }

    bool D2DToastCenter::onMouseDown(float x, float y)
    {
        if (!rect_.contains(x, y) || toasts_.empty())
        {
            return false;
        }

        for (std::size_t i = 0; i < toasts_.size(); ++i)
        {
            if (toastRect(i).contains(x, y))
            {
                toasts_.erase(toasts_.begin() + static_cast<std::ptrdiff_t>(i));
                return true;
            }
        }

        return false;
    }

    void D2DToastCenter::render(D2DRenderContext& ctx)
    {
        if (toasts_.empty())
        {
            return;
        }

        const std::size_t count = std::min<std::size_t>(toasts_.size(), 4);

        for (std::size_t i = 0; i < count; ++i)
        {
            const auto& toast = toasts_[toasts_.size() - 1 - i];
            const auto r = toastRect(i);

            D2DWidgetUtils::fillRounded(ctx, r, 16.0f, ctx.brushes.panelElevated, accentBrush(ctx, toast.kind), 1.2f);

            UiRect stripe{r.left + 8.0f, r.top + 9.0f, r.left + 12.0f, r.bottom - 9.0f};
            D2DWidgetUtils::fillRounded(ctx, stripe, 2.0f, accentBrush(ctx, toast.kind));

            D2DWidgetUtils::drawTextEx(
                ctx,
                toast.title,
                FontRole::Small,
                r.inset({20.0f, 8.0f, 12.0f, 38.0f}),
                ctx.brushes.text
            );

            D2DWidgetUtils::drawTextEx(
                ctx,
                toast.body,
                FontRole::Small,
                r.inset({20.0f, 27.0f, 12.0f, 7.0f}),
                ctx.brushes.muted
            );
        }
    }

    ID2D1Brush* D2DToastCenter::accentBrush(D2DRenderContext& ctx, D2DToastKind kind) const
    {
        switch (kind)
        {
        case D2DToastKind::Success:
            return ctx.brushes.accent;
        case D2DToastKind::Warning:
            return ctx.brushes.accentWarm;
        case D2DToastKind::Error:
            return ctx.brushes.danger;
        case D2DToastKind::Info:
        default:
            return ctx.brushes.accentBlue;
        }
    }

    UiRect D2DToastCenter::toastRect(std::size_t visualIndex) const
    {
        const float width = std::min(370.0f, rect_.width());
        const float height = 70.0f;
        const float gap = 10.0f;
        const float right = rect_.right;
        const float bottom = rect_.bottom - static_cast<float>(visualIndex) * (height + gap);

        return makeUiRect(
            right - width,
            bottom - height,
            right,
            bottom
        );
    }

    float D2DToastCenter::opacityFor(const D2DToast& toast) const
    {
        const float fadeIn = std::min(1.0f, toast.ageSeconds / 0.18f);
        const float remaining = toast.ttlSeconds - toast.ageSeconds;
        const float fadeOut = remaining < 0.35f ? std::max(0.0f, remaining / 0.35f) : 1.0f;
        return std::min(fadeIn, fadeOut);
    }
}
