#include "ArhqenCognitionEngine/AquariumRender/AceAquariumSceneAdapter.h"

#include "ArhqenCognitionEngine/Aquarium/AceAqActions.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqEnvironment.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqGrid.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqObjects.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqObservation.h"
#include "ArhqenCognitionEngine/AquariumUI/AceAquariumRuntimeController.h"

#include <algorithm>
#include <string>

namespace ace::aquarium_render
{
    namespace
    {
        float channel(int value)
        {
            return std::clamp(static_cast<float>(value) / 255.0f, 0.0f, 1.0f);
        }

        AceAqRenderPrimitive primitive(AceAqRenderPrimitiveKind kind, float x, float y, float z, float sx, float sy, float sz, float r, float g, float b, float a)
        {
            AceAqRenderPrimitive p{};
            p.Kind = kind;
            p.X = x;
            p.Y = y;
            p.Z = z;
            p.SizeX = sx;
            p.SizeY = sy;
            p.SizeZ = sz;
            p.R = r;
            p.G = g;
            p.B = b;
            p.A = a;
            return p;
        }

        bool isHazardLike(const ace::aquarium::AceAqWorldObject& object)
        {
            return object.touchEffect.integrity < 0.0 || object.consumeEffect.integrity < 0.0 || object.temperatureSignal > 0.75;
        }

        AceAqRenderPrimitive objectPrimitiveFor(
            int x,
            int y,
            const ace::aquarium::AceAqWorldObject& object
        )
        {
            const auto obs = ace::aquarium::ObserveCell(object);
            const float r = channel(obs.colorRgb.r);
            const float g = channel(obs.colorRgb.g);
            const float b = channel(obs.colorRgb.b);

            if (obs.solidHint)
            {
                return primitive(AceAqRenderPrimitiveKind::Block, static_cast<float>(x), static_cast<float>(y), 0.02f, 0.86f, 0.86f, 0.85f, std::max(0.18f, r), std::max(0.22f, g), std::max(0.26f, b), 0.86f);
            }

            if (obs.liquidLikeHint)
            {
                return primitive(AceAqRenderPrimitiveKind::Tile, static_cast<float>(x), static_cast<float>(y), 0.03f, 0.92f, 0.92f, 0.04f, std::max(0.08f, r), std::max(0.12f, g), std::max(0.18f, b), 0.72f);
            }

            if (isHazardLike(object))
            {
                return primitive(AceAqRenderPrimitiveKind::Block, static_cast<float>(x) + 0.18f, static_cast<float>(y) + 0.18f, 0.08f, 0.60f, 0.60f, 0.52f, 1.00f, 0.24f, 0.22f, 0.82f);
            }

            if (object.consumable || object.smellSignal > 0.10)
            {
                return primitive(AceAqRenderPrimitiveKind::Block, static_cast<float>(x) + 0.25f, static_cast<float>(y) + 0.25f, 0.08f, 0.50f, 0.50f, 0.38f, 0.65f, 0.96f, 0.48f, 0.82f);
            }

            return primitive(AceAqRenderPrimitiveKind::Block, static_cast<float>(x) + 0.22f, static_cast<float>(y) + 0.22f, 0.05f, 0.56f, 0.56f, 0.34f, std::max(0.22f, r), std::max(0.28f, g), std::max(0.32f, b), 0.78f);
        }
    }

    std::vector<AceAqRenderPrimitive> AceAquariumSceneAdapter::BuildPrimitives(
        const ace::aquarium_ui::AceAquariumRuntimeController& controller,
        bool debugTruthEnabled
    ) const
    {
        std::vector<AceAqRenderPrimitive> primitives;
        const auto& environment = controller.Environment();
        const auto& world = environment.World();

        const int width = world.Width();
        const int height = world.Height();

        primitives.reserve(static_cast<std::size_t>(width * height + width + height + 8));

        for (int x = 0; x <= width; ++x)
        {
            primitives.push_back(primitive(AceAqRenderPrimitiveKind::GridLine, static_cast<float>(x), 0.0f, 0.0f, 0.0f, static_cast<float>(height), 0.0f, 0.16f, 0.78f, 1.0f, 0.22f));
        }

        for (int y = 0; y <= height; ++y)
        {
            primitives.push_back(primitive(AceAqRenderPrimitiveKind::GridLine, 0.0f, static_cast<float>(y), 0.0f, static_cast<float>(width), 0.0f, 0.0f, 0.16f, 0.78f, 1.0f, 0.22f));
        }

        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                const ace::aquarium::AceAqPoint point{x, y};
                const auto object = world.GetCell(point);
                const auto obs = ace::aquarium::ObserveCell(object);

                primitives.push_back(primitive(
                    AceAqRenderPrimitiveKind::Tile,
                    static_cast<float>(x),
                    static_cast<float>(y),
                    -0.01f,
                    0.96f,
                    0.96f,
                    0.01f,
                    obs.liquidLikeHint ? 0.08f : 0.035f,
                    obs.liquidLikeHint ? 0.22f : 0.10f,
                    obs.liquidLikeHint ? 0.30f : 0.16f,
                    obs.liquidLikeHint ? 0.30f : 0.18f
                ));

                if (object.kind == ace::aquarium::AceAqObjectKind::Empty)
                {
                    continue;
                }

                auto visual = objectPrimitiveFor(x, y, object);
                primitives.push_back(visual);

                if (debugTruthEnabled)
                {
                    AceAqRenderPrimitive label = primitive(
                        AceAqRenderPrimitiveKind::DebugLabel,
                        static_cast<float>(x) + 0.50f,
                        static_cast<float>(y) + 0.50f,
                        visual.Z + visual.SizeZ + 0.20f,
                        0.0f,
                        0.0f,
                        0.0f,
                        1.0f,
                        0.74f,
                        0.24f,
                        1.0f
                    );
                    label.Label = ace::aquarium::ToString(object.kind);
                    primitives.push_back(label);
                }
            }
        }

        const auto agent = world.AgentPosition();
        primitives.push_back(primitive(
            AceAqRenderPrimitiveKind::Agent,
            static_cast<float>(agent.x) + 0.18f,
            static_cast<float>(agent.y) + 0.18f,
            0.14f,
            0.64f,
            0.64f,
            0.70f,
            0.15f,
            0.98f,
            1.0f,
            0.95f
        ));

        const auto front = world.FrontPosition();
        primitives.push_back(primitive(
            AceAqRenderPrimitiveKind::Highlight,
            static_cast<float>(front.x),
            static_cast<float>(front.y),
            0.12f,
            0.96f,
            0.96f,
            0.06f,
            1.0f,
            0.86f,
            0.20f,
            0.72f
        ));

        const auto delta = ace::aquarium::DirectionDelta(world.AgentDirection());
        primitives.push_back(primitive(
            AceAqRenderPrimitiveKind::DirectionArrow,
            static_cast<float>(agent.x) + 0.50f,
            static_cast<float>(agent.y) + 0.50f,
            0.72f,
            static_cast<float>(delta.x) * 0.82f,
            static_cast<float>(delta.y) * 0.82f,
            0.02f,
            1.0f,
            0.95f,
            0.35f,
            0.95f
        ));

        if (debugTruthEnabled)
        {
            AceAqRenderPrimitive warning = primitive(AceAqRenderPrimitiveKind::DebugLabel, 0.0f, -1.20f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.62f, 0.18f, 1.0f);
            warning.Label = "DEBUG TRUTH - NOT AGENT INPUT";
            primitives.push_back(warning);
        }

        return primitives;
    }
}
