#pragma once

#include "ArhqenCognitionEngine/Core/AceUiModel.h"

namespace am::ui
{
    struct D2DGraphForceLayoutOptions
    {
        int iterations = 18;
        float repulsion = 0.024f;
        float attraction = 0.045f;
        float centerPull = 0.035f;
        float damping = 0.72f;
    };

    class D2DGraphForceLayout
    {
    public:
        static void apply(
            std::vector<am::core::AceUiGraphNode>& nodes,
            const std::vector<am::core::AceUiGraphEdge>& edges,
            D2DGraphForceLayoutOptions options = {}
        );

    private:
        static am::core::AceUiGraphNode* findNode(std::vector<am::core::AceUiGraphNode>& nodes, std::uint64_t id);
    };
}
