#pragma once

#include "ArhqenCognitionEngine/Core/AceUiModel.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DWidgetUtils.h"

#include <optional>

namespace am::ui
{
    class D2DAutocompletePopup
    {
    public:
        void setRect(UiRect rect);
        void setSuggestions(std::vector<am::core::AceSuggestion> suggestions);

        void open();
        void close();
        bool active() const;

        bool onKeyDown(WPARAM key);
        bool onMouseMove(float x, float y);
        bool onMouseDown(float x, float y);
        bool onMouseUp(float x, float y);

        std::optional<std::wstring> takeAcceptedText();
        void render(D2DRenderContext& ctx);

    private:
        ID2D1Brush* accentBrush(D2DRenderContext& ctx, int accentIndex) const;

        UiRect rect_{};
        std::vector<am::core::AceSuggestion> suggestions_;
        std::size_t selected_ = 0;
        bool active_ = false;
        bool mousePressed_ = false;
        std::optional<std::wstring> acceptedText_;
    };
}
