#pragma once

#include "ArhqenCognitionEngine/Ui/D2D/D2DWidgetUtils.h"

#include <optional>

namespace am::ui
{
    struct D2DToolbarItem
    {
        std::wstring id;
        std::wstring label;
        std::wstring hint;
        int accentIndex = 0;
        bool enabled = true;
        bool toggled = false;
    };

    class D2DToolbar
    {
    public:
        void setRect(UiRect rect);
        void setItems(std::vector<D2DToolbarItem> items);
        void setToggled(const std::wstring& id, bool toggled);
        void setEnabled(const std::wstring& id, bool enabled);

        bool onMouseMove(float x, float y);
        bool onMouseDown(float x, float y);
        bool onMouseUp(float x, float y);
        std::optional<std::wstring> takeActivated();

        void render(D2DRenderContext& ctx);

    private:
        ID2D1Brush* accentBrush(D2DRenderContext& ctx, int accentIndex) const;
        UiRect itemRect(std::size_t index) const;
        std::size_t hitIndex(float x, float y) const;

        UiRect rect_{};
        std::vector<D2DToolbarItem> items_;
        std::size_t hovered_ = static_cast<std::size_t>(-1);
        std::size_t pressed_ = static_cast<std::size_t>(-1);
        std::optional<std::wstring> activated_;
    };
}
