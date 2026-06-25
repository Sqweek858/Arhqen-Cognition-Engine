#pragma once

#include "ArhqenCognitionEngine/Ui/D2D/D2DWidgetUtils.h"

#include <optional>

namespace am::ui
{
    struct D2DCommandPaletteItem
    {
        std::wstring id;
        std::wstring title;
        std::wstring subtitle;
        std::wstring shortcut;
    };

    class D2DCommandPalette
    {
    public:
        void setRect(UiRect rect);
        void setItems(std::vector<D2DCommandPaletteItem> items);

        void open();
        void close();
        void toggle();
        bool active() const;

        bool onChar(WPARAM wParam);
        bool onKeyDown(WPARAM wParam);
        bool onMouseMove(float x, float y);
        bool onMouseDown(float x, float y);
        bool onMouseUp(float x, float y);

        std::optional<std::wstring> takePendingCommand();
        void render(D2DRenderContext& ctx);

    private:
        std::vector<std::size_t> filteredIndexes() const;
        bool matches(const D2DCommandPaletteItem& item) const;
        void moveSelection(int delta);
        void acceptSelection();

        UiRect rect_{};
        UiRect inputRect_{};
        UiRect listRect_{};

        std::vector<D2DCommandPaletteItem> items_;
        std::wstring query_;
        std::size_t selected_ = 0;
        bool active_ = false;
        bool mousePressed_ = false;
        std::optional<std::wstring> pendingCommand_;
    };
}
