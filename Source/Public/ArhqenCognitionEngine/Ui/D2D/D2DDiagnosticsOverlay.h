#pragma once

#include "ArhqenCognitionEngine/Ui/D2D/D2DWidgetUtils.h"

namespace am::ui
{
    struct D2DDiagnosticsSnapshot
    {
        std::wstring title = L"Diagnostics";
        std::vector<std::pair<std::wstring, std::wstring>> rows;
        std::vector<std::wstring> notes;
        bool visible = false;
    };

    class D2DDiagnosticsOverlay
    {
    public:
        void setRect(UiRect rect);
        void setSnapshot(D2DDiagnosticsSnapshot snapshot);
        void setVisible(bool visible);
        bool visible() const;
        void toggle();

        bool onKeyDown(WPARAM key);
        bool onMouseDown(float x, float y);
        void render(D2DRenderContext& ctx);

    private:
        UiRect rect_{};
        D2DDiagnosticsSnapshot snapshot_{};
        bool visible_ = false;
    };
}
