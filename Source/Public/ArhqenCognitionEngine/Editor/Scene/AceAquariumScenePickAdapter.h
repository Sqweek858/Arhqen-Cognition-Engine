#pragma once

#include "ArhqenCognitionEngine/AquariumRender/AceAquariumRenderPrimitive.h"
#include "ArhqenCognitionEngine/Editor/Scene/AceScenePicking.h"

#include <cstddef>
#include <vector>

namespace am::editor::scene
{
    struct ScenePickSyncResult final
    {
        std::size_t renderedGeometryPrimitives = 0;
        std::size_t renderedGridPrimitives = 0;
        std::size_t publishedProxies = 0;
    };

    class AquariumScenePickAdapter final
    {
    public:
        [[nodiscard]] static ScenePickSyncResult synchronize(
            const std::vector<ace::aquarium_render::AceAqRenderPrimitive>& primitives,
            const am::core::scene::SceneWorld& world,
            const am::core::Guid& previewGeometryId,
            const am::core::Guid& referenceGridId,
            ScenePicker& picker);
    };
}
