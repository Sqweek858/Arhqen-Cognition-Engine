#pragma once

#include "ArhqenCognitionEngine/Core/AceUiModel.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DWidgetUtils.h"

#include <optional>

namespace am::ui
{
    class D2DGraphPreview
    {
    public:
        void setRect(UiRect rect);
        void setGraph(std::vector<am::core::AceUiGraphNode> nodes, std::vector<am::core::AceUiGraphEdge> edges);
        std::optional<am::core::AceUiSelection> hitTest(float x, float y) const;
        void render(D2DRenderContext& ctx);

    private:
        ID2D1Brush* accentBrush(D2DRenderContext& ctx, int accentIndex) const;
        UiRect nodeRect(UiRect graphRect, const am::core::AceUiGraphNode& node) const;
        const am::core::AceUiGraphNode* findNode(std::uint64_t id) const;

        UiRect rect_{};
        std::vector<am::core::AceUiGraphNode> nodes_;
        std::vector<am::core::AceUiGraphEdge> edges_;
    };
}
