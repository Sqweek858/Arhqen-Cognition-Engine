#pragma once

#include <string>

namespace ace::aquarium_render
{
    enum class AceAqRenderPrimitiveKind
    {
        GridLine,
        Tile,
        Block,
        Agent,
        DirectionArrow,
        Highlight,
        DebugLabel
    };

    struct AceAqRenderPrimitive
    {
        AceAqRenderPrimitiveKind Kind = AceAqRenderPrimitiveKind::Tile;
        float X = 0.0f;
        float Y = 0.0f;
        float Z = 0.0f;
        float SizeX = 1.0f;
        float SizeY = 1.0f;
        float SizeZ = 1.0f;
        float R = 1.0f;
        float G = 1.0f;
        float B = 1.0f;
        float A = 1.0f;
        std::string Label;
    };

    const char* ToString(AceAqRenderPrimitiveKind kind);
    bool PrimitiveLabelContainsObjectKindTruth(const AceAqRenderPrimitive& primitive);
}
