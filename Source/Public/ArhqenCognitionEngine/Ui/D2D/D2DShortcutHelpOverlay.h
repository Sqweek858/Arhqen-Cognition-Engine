#pragma once

#include "ArhqenCognitionEngine/Ui/D2D/D2DWidgetUtils.h"

namespace am::ui
{
    struct D2DShortcutHelpItem
    {
        std::wstring key;
        std::wstring action;
        std::wstring group;
    };

    class D2DShortcutHelpOverlay
    {
    public:
        void setRect(UiRect rect);
        void setItems(std::vector<D2DShortcutHelpItem> items);
        void setVisible(bool visible);
        bool visible() const;
        void toggle();

        bool onKeyDown(WPARAM key);
        bool onMouseDown(float x, float y);
        void render(D2DRenderContext& ctx);

    private:
        void renderGroup(D2DRenderContext& ctx, UiRect rect, const std::wstring& group, const std::vector<D2DShortcutHelpItem>& items);
        std::vector<std::wstring> groups() const;

        UiRect rect_{};
        std::vector<D2DShortcutHelpItem> items_;
        bool visible_ = false;
    };
}
