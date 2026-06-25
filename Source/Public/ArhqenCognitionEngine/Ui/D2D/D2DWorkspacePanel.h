#pragma once

#include "ArhqenCognitionEngine/Core/AceUiModel.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DGraphPreview.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DGraphCanvas.h"

#include <optional>

namespace am::ui
{
    class D2DWorkspacePanel
    {
    public:
        void setRect(UiRect rect);
        void setSnapshot(am::core::AceUiSnapshot snapshot);
        std::optional<am::core::AceUiSelection> hitTest(float x, float y) const;

        bool onMouseDown(float x, float y);
        bool onMouseMove(float x, float y);
        bool onMouseUp(float x, float y);
        bool onMouseWheel(float x, float y, int wheelDelta);
        bool onKeyDown(WPARAM key);
        std::optional<am::core::AceUiSelection> takeGraphSelection();
        am::core::AceUiGraphStats graphStats() const;

        void render(D2DRenderContext& ctx);

    private:
        void renderMetrics(D2DRenderContext& ctx, UiRect rect);
        void renderList(D2DRenderContext& ctx, UiRect rect, const std::wstring& title, const std::vector<am::core::AceUiListItem>& items, std::size_t maxItems);
        std::optional<am::core::AceUiSelection> hitTestList(UiRect rect, const std::vector<am::core::AceUiListItem>& items, std::size_t maxItems, float x, float y) const;
        ID2D1Brush* accentBrush(D2DRenderContext& ctx, int accentIndex) const;

        UiRect rect_{};
        UiRect metricsRect_{};
        UiRect conceptsRect_{};
        UiRect questionsRect_{};
        UiRect graphRect_{};
        am::core::AceUiSnapshot snapshot_;
        D2DGraphPreview graphPreview_;
        D2DGraphCanvas graphCanvas_;
    };
}
