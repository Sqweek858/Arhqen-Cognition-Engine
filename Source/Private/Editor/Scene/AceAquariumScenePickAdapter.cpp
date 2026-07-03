#include "ArhqenCognitionEngine/Editor/Scene/AceAquariumScenePickAdapter.h"

#include <algorithm>
#include <limits>

namespace am::editor::scene
{
    ScenePickSyncResult AquariumScenePickAdapter::synchronize(
        const std::vector<ace::aquarium_render::AceAqRenderPrimitive>& primitives,
        const am::core::scene::SceneWorld& world,
        const am::core::Guid& previewGeometryId,
        const am::core::Guid& referenceGridId,
        ScenePicker& picker)
    {
        using ace::aquarium_render::AceAqRenderPrimitiveKind;
        ScenePickSyncResult result;
        picker.clear();
        if (!previewGeometryId.isValid() || !referenceGridId.isValid()) return result;

        const double infinity = std::numeric_limits<double>::infinity();
        AxisAlignedBounds geometry{{infinity, infinity, infinity}, {-infinity, -infinity, -infinity}};
        AxisAlignedBounds grid{{infinity, infinity, infinity}, {-infinity, -infinity, -infinity}};
        auto include = [](AxisAlignedBounds& bounds, double x0, double y0, double z0,
                          double x1, double y1, double z1)
        {
            bounds.minimum.x = std::min(bounds.minimum.x, std::min(x0, x1));
            bounds.minimum.y = std::min(bounds.minimum.y, std::min(y0, y1));
            bounds.minimum.z = std::min(bounds.minimum.z, std::min(z0, z1));
            bounds.maximum.x = std::max(bounds.maximum.x, std::max(x0, x1));
            bounds.maximum.y = std::max(bounds.maximum.y, std::max(y0, y1));
            bounds.maximum.z = std::max(bounds.maximum.z, std::max(z0, z1));
        };

        for (const auto& primitive : primitives)
        {
            if (primitive.Kind == AceAqRenderPrimitiveKind::DebugLabel) continue;
            const double x = static_cast<double>(primitive.X) + 4.0;
            const double z = static_cast<double>(primitive.Y);
            if (primitive.Kind == AceAqRenderPrimitiveKind::GridLine)
            {
                include(grid, x - 0.02, 0.0, z - 0.02,
                    x + static_cast<double>(primitive.SizeX) + 0.02, 0.03,
                    z + static_cast<double>(primitive.SizeY) + 0.02);
                ++result.renderedGridPrimitives;
                continue;
            }

            const double width = std::max(0.05, static_cast<double>(primitive.SizeX));
            const double depth = std::max(0.05, static_cast<double>(primitive.SizeY));
            double padding = 0.0, height = 0.12;
            if (primitive.Kind == AceAqRenderPrimitiveKind::Block)
                height = std::max(0.84, static_cast<double>(primitive.SizeZ) + 0.12);
            else if (primitive.Kind == AceAqRenderPrimitiveKind::Agent)
            { padding = 0.12; height = 1.30; }
            else if (primitive.Kind == AceAqRenderPrimitiveKind::DirectionArrow)
            { padding = 0.10; height = 0.30; }
            include(geometry, x - padding, 0.0, z - padding,
                x + width + padding, height, z + depth + padding);
            ++result.renderedGeometryPrimitives;
        }

        if (result.renderedGridPrimitives != 0)
        {
            const auto* entity = world.find(referenceGridId);
            if (entity && picker.upsert({entity->id, grid, entity->visible, !entity->locked, -10}))
                ++result.publishedProxies;
        }
        if (result.renderedGeometryPrimitives != 0)
        {
            const auto* entity = world.find(previewGeometryId);
            if (entity && picker.upsert({entity->id, geometry, entity->visible, !entity->locked, 10}))
                ++result.publishedProxies;
        }
        return result;
    }
}
