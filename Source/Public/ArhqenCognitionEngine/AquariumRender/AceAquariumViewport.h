#pragma once

#include "ArhqenCognitionEngine/AquariumRender/AceAquariumCamera.h"
#include "ArhqenCognitionEngine/AquariumRender/AceAquariumRenderPrimitive.h"

#include <vector>

namespace ace::aquarium_render
{
    struct AceAqViewportPoint
    {
        float X = 0.0f;
        float Y = 0.0f;
    };

    struct AceAqViewportPrimitive
    {
        AceAqRenderPrimitiveKind Kind = AceAqRenderPrimitiveKind::Tile;
        AceAqViewportPoint A{};
        AceAqViewportPoint B{};
        AceAqViewportPoint C{};
        AceAqViewportPoint D{};
        float Depth = 0.0f;
        float R = 1.0f;
        float G = 1.0f;
        float Bc = 1.0f;
        float Aalpha = 1.0f;
        std::string Label;
    };

    class AceAquariumViewport
    {
    public:
        std::vector<AceAqViewportPrimitive> BuildViewportModel(
            const std::vector<AceAqRenderPrimitive>& primitives,
            const AceAquariumCamera& camera,
            float viewportWidth,
            float viewportHeight
        ) const;

        AceAqViewportPoint Project(
            const AceAqRenderPrimitive& primitive,
            const AceAquariumCamera& camera,
            float viewportWidth,
            float viewportHeight
        ) const;
    };
}
