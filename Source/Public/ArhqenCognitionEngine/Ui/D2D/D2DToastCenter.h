#pragma once

#include "ArhqenCognitionEngine/Ui/D2D/D2DWidgetUtils.h"

namespace am::ui
{
    enum class D2DToastKind
    {
        Info,
        Success,
        Warning,
        Error
    };

    struct D2DToast
    {
        std::wstring title;
        std::wstring body;
        D2DToastKind kind = D2DToastKind::Info;
        float ageSeconds = 0.0f;
        float ttlSeconds = 4.0f;
        std::uint64_t id = 0;
    };

    class D2DToastCenter
    {
    public:
        void setRect(UiRect rect);
        void push(std::wstring title, std::wstring body, D2DToastKind kind = D2DToastKind::Info, float ttlSeconds = 4.0f);
        void clear();

        void update(float dtSeconds);
        bool hasActiveToasts() const;
        std::size_t size() const;

        bool onMouseDown(float x, float y);
        void render(D2DRenderContext& ctx);

    private:
        ID2D1Brush* accentBrush(D2DRenderContext& ctx, D2DToastKind kind) const;
        UiRect toastRect(std::size_t visualIndex) const;
        float opacityFor(const D2DToast& toast) const;

        UiRect rect_{};
        std::vector<D2DToast> toasts_;
        std::uint64_t nextId_ = 1;
    };
}
