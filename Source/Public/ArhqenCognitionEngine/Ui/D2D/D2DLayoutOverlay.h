#pragma once

#include "ArhqenCognitionEngine/Ui/D2D/D2DLayoutProfile.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DWidgetUtils.h"

namespace am::ui
{
    class D2DLayoutOverlay
    {
    public:
        void setRect(UiRect rect);
        void setProfile(D2DDockLayoutProfile profile);
        void setVisible(bool visible);
        bool visible() const;
        void toggle();

        bool onKeyDown(WPARAM key);
        bool onMouseDown(float x, float y);

        void render(D2DRenderContext& ctx);

    private:
        void renderRow(D2DRenderContext& ctx, UiRect row, const std::wstring& name, const std::wstring& value, int accent);

        UiRect rect_{};
        D2DDockLayoutProfile profile_{};
        bool visible_ = false;
    };
}
