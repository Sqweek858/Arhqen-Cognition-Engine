#include "ArhqenCognitionEngine/AquariumRender/AceAquariumViewport.h"

#include <algorithm>

namespace ace::aquarium_render
{
    AceAqViewportPoint AceAquariumViewport::Project(
        const AceAqRenderPrimitive& primitive,
        const AceAquariumCamera& camera,
        float viewportWidth,
        float viewportHeight
    ) const
    {
        const float isoX = (primitive.X - primitive.Y) * camera.Zoom;
        const float isoY = (primitive.X + primitive.Y) * camera.Zoom * camera.Tilt - primitive.Z * camera.Zoom * camera.HeightScale;

        return {
            viewportWidth * 0.50f + isoX + camera.OriginX,
            viewportHeight * 0.12f + isoY + camera.OriginY
        };
    }

    std::vector<AceAqViewportPrimitive> AceAquariumViewport::BuildViewportModel(
        const std::vector<AceAqRenderPrimitive>& primitives,
        const AceAquariumCamera& camera,
        float viewportWidth,
        float viewportHeight
    ) const
    {
        std::vector<AceAqViewportPrimitive> model;
        model.reserve(primitives.size());

        for (const auto& primitive : primitives)
        {
            AceAqViewportPrimitive out{};
            out.Kind = primitive.Kind;
            out.R = primitive.R;
            out.G = primitive.G;
            out.Bc = primitive.B;
            out.Aalpha = primitive.A;
            out.Label = primitive.Label;
            out.Depth = primitive.X + primitive.Y + primitive.Z;

            AceAqRenderPrimitive a = primitive;
            AceAqRenderPrimitive b = primitive;
            AceAqRenderPrimitive c = primitive;
            AceAqRenderPrimitive d = primitive;

            switch (primitive.Kind)
            {
            case AceAqRenderPrimitiveKind::GridLine:
                b.X = primitive.X + primitive.SizeX;
                b.Y = primitive.Y + primitive.SizeY;
                out.A = Project(a, camera, viewportWidth, viewportHeight);
                out.B = Project(b, camera, viewportWidth, viewportHeight);
                out.C = out.B;
                out.D = out.A;
                break;
            case AceAqRenderPrimitiveKind::DirectionArrow:
                b.X = primitive.X + primitive.SizeX;
                b.Y = primitive.Y + primitive.SizeY;
                b.Z = primitive.Z + primitive.SizeZ;
                out.A = Project(a, camera, viewportWidth, viewportHeight);
                out.B = Project(b, camera, viewportWidth, viewportHeight);
                out.C = out.B;
                out.D = out.A;
                break;
            default:
                b.X = primitive.X + primitive.SizeX;
                c.X = primitive.X + primitive.SizeX;
                c.Y = primitive.Y + primitive.SizeY;
                d.Y = primitive.Y + primitive.SizeY;

                out.A = Project(a, camera, viewportWidth, viewportHeight);
                out.B = Project(b, camera, viewportWidth, viewportHeight);
                out.C = Project(c, camera, viewportWidth, viewportHeight);
                out.D = Project(d, camera, viewportWidth, viewportHeight);
                break;
            }

            model.push_back(out);
        }

        std::stable_sort(model.begin(), model.end(), [](const AceAqViewportPrimitive& lhs, const AceAqViewportPrimitive& rhs)
        {
            return lhs.Depth < rhs.Depth;
        });

        return model;
    }
}
