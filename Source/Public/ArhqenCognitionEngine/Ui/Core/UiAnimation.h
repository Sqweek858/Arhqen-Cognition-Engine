#pragma once

namespace am::ui
{
    enum class UiEasing
    {
        Linear,
        EaseOutCubic,
        EaseInOutCubic,
        SmoothStep
    };

    float uiApplyEasing(float t, UiEasing easing);

    class UiAnimatedFloat
    {
    public:
        UiAnimatedFloat() = default;
        explicit UiAnimatedFloat(float value);

        void setImmediate(float value);
        void animateTo(float target, float durationSeconds, UiEasing easing = UiEasing::EaseOutCubic);
        void update(float deltaSeconds);

        float value() const;
        float target() const;
        bool animating() const;

    private:
        float start_ = 0.0f;
        float value_ = 0.0f;
        float target_ = 0.0f;
        float elapsed_ = 0.0f;
        float duration_ = 0.0f;
        UiEasing easing_ = UiEasing::EaseOutCubic;
        bool animating_ = false;
    };

    class UiAnimatedBool
    {
    public:
        void set(bool enabled, float durationSeconds = 0.16f);
        void update(float deltaSeconds);

        bool enabled() const;
        float alpha() const;

    private:
        bool enabled_ = false;
        UiAnimatedFloat alpha_ {};
    };

    struct UiHoverState
    {
        UiAnimatedFloat alpha {0.0f};

        void setHovered(bool hovered, float durationSeconds = 0.14f);
        void update(float deltaSeconds);
        float value() const;
    };
}
