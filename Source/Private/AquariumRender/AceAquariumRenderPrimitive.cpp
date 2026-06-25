#include "ArhqenCognitionEngine/AquariumRender/AceAquariumRenderPrimitive.h"

#include "ArhqenCognitionEngine/AquariumUI/AceAquariumUiSnapshot.h"

namespace ace::aquarium_render
{
    const char* ToString(AceAqRenderPrimitiveKind kind)
    {
        switch (kind)
        {
        case AceAqRenderPrimitiveKind::GridLine: return "GridLine";
        case AceAqRenderPrimitiveKind::Tile: return "Tile";
        case AceAqRenderPrimitiveKind::Block: return "Block";
        case AceAqRenderPrimitiveKind::Agent: return "Agent";
        case AceAqRenderPrimitiveKind::DirectionArrow: return "DirectionArrow";
        case AceAqRenderPrimitiveKind::Highlight: return "Highlight";
        case AceAqRenderPrimitiveKind::DebugLabel: return "DebugLabel";
        default: return "Unknown";
        }
    }

    bool PrimitiveLabelContainsObjectKindTruth(const AceAqRenderPrimitive& primitive)
    {
        return ace::aquarium_ui::ContainsAquariumTruthLabel(primitive.Label);
    }
}
