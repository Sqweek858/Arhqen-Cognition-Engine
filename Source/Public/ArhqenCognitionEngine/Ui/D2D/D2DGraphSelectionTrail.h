#pragma once

#include "ArhqenCognitionEngine/Core/AceUiModel.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DWidgetUtils.h"

#include <deque>

namespace am::ui
{
    struct D2DGraphSelectionTrailEntry
    {
        std::uint64_t id = 0;
        std::wstring label;
        std::wstring source;
    };

    class D2DGraphSelectionTrail
    {
    public:
        void setRect(UiRect rect);
        void push(am::core::AceUiSelection selection);
        void clear();

        std::size_t size() const;
        bool empty() const;

        void render(D2DRenderContext& ctx);

    private:
        UiRect rect_{};
        std::deque<D2DGraphSelectionTrailEntry> entries_;
        std::size_t maxEntries_ = 5;
    };
}
