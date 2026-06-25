#pragma once

#include "ArhqenCognitionEngine/AquariumRender/AceAquariumRenderPrimitive.h"

#include <vector>

namespace ace::aquarium_ui
{
    class AceAquariumRuntimeController;
}

namespace ace::aquarium_render
{
    class AceAquariumSceneAdapter
    {
    public:
        std::vector<AceAqRenderPrimitive> BuildPrimitives(
            const ace::aquarium_ui::AceAquariumRuntimeController& controller,
            bool debugTruthEnabled
        ) const;
    };
}
