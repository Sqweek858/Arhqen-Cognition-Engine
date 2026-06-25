#include "ArhqenCognitionEngine/Ui/Core/UiAnimation.h"

#include <algorithm>
#include <cmath>

namespace am::ui
{
    float uiApplyEasing(float t, UiEasing easing)
    {
        t = std::clamp(t, 0.0f, 1.0f);

        switch (easing)
        {
        case UiEasing::Linear:
            return t;
        case UiEasing::EaseOutCubic:
        {
            const float inv = 1.0f - t;
            return 1.0f - inv * inv * inv;
        }
        case UiEasing::EaseInOutCubic:
            return t < 0.5f ? 4.0f * t * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) * 0.5f;
        case UiEasing::SmoothStep:
            return t * t * (3.0f - 2.0f * t);
        default:
            return t;
        }
    }

    UiAnimatedFloat::UiAnimatedFloat(float value)
        : start_(value)
        , value_(value)
        , target_(value)
    {
    }

    void UiAnimatedFloat::setImmediate(float value)
    {
        start_ = value;
        value_ = value;
        target_ = value;
        elapsed_ = 0.0f;
        duration_ = 0.0f;
        animating_ = false;
    }

    void UiAnimatedFloat::animateTo(float target, float durationSeconds, UiEasing easing)
    {
        start_ = value_;
        target_ = target;
        elapsed_ = 0.0f;
        duration_ = std::max(0.0f, durationSeconds);
        easing_ = easing;
        animating_ = duration_ > 0.0f && start_ != target_;

        if (!animating_)
        {
            value_ = target_;
        }
    }

    void UiAnimatedFloat::update(float deltaSeconds)
    {
        if (!animating_)
        {
            return;
        }

        elapsed_ += std::max(0.0f, deltaSeconds);
        const float t = duration_ <= 0.0f ? 1.0f : std::clamp(elapsed_ / duration_, 0.0f, 1.0f);
        const float eased = uiApplyEasing(t, easing_);

        value_ = start_ + (target_ - start_) * eased;

        if (t >= 1.0f)
        {
            value_ = target_;
            animating_ = false;
        }
    }

    float UiAnimatedFloat::value() const
    {
        return value_;
    }

    float UiAnimatedFloat::target() const
    {
        return target_;
    }

    bool UiAnimatedFloat::animating() const
    {
        return animating_;
    }

    void UiAnimatedBool::set(bool enabled, float durationSeconds)
    {
        enabled_ = enabled;
        alpha_.animateTo(enabled ? 1.0f : 0.0f, durationSeconds, UiEasing::EaseOutCubic);
    }

    void UiAnimatedBool::update(float deltaSeconds)
    {
        alpha_.update(deltaSeconds);
    }

    bool UiAnimatedBool::enabled() const
    {
        return enabled_;
    }

    float UiAnimatedBool::alpha() const
    {
        return alpha_.value();
    }

    void UiHoverState::setHovered(bool hovered, float durationSeconds)
    {
        alpha.animateTo(hovered ? 1.0f : 0.0f, durationSeconds, UiEasing::EaseOutCubic);
    }

    void UiHoverState::update(float deltaSeconds)
    {
        alpha.update(deltaSeconds);
    }

    float UiHoverState::value() const
    {
        return alpha.value();
    }
}
