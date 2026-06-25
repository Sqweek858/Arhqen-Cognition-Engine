#pragma once

#include "ArhqenCognitionEngine/Ui/D2D/D2DWidgetUtils.h"

#include <optional>

namespace am::ui
{
    struct D2DTabItem
    {
        std::wstring id;
        std::wstring label;
        int accentIndex = 0;
    };

    class D2DTabStrip
    {
    public:
        void setRect(UiRect rect);
        void setTabs(std::vector<D2DTabItem> tabs);
        void setActive(std::wstring id);

        const std::wstring& active() const;
        bool onMouseMove(float x, float y);
        bool onMouseDown(float x, float y);
        bool onMouseUp(float x, float y);
        std::optional<std::wstring> takeActivated();

        void render(D2DRenderContext& ctx);

    private:
        ID2D1Brush* accentBrush(D2DRenderContext& ctx, int accentIndex) const;
        UiRect tabRect(std::size_t index) const;

        UiRect rect_{};
        std::vector<D2DTabItem> tabs_;
        std::wstring active_;
        std::size_t hovered_ = static_cast<std::size_t>(-1);
        std::size_t pressed_ = static_cast<std::size_t>(-1);
        std::optional<std::wstring> activated_;
    };
}
