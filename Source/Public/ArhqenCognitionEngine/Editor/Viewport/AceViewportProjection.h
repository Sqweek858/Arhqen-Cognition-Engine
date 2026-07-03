#pragma once

#include "ArhqenCognitionEngine/Editor/Scene/AceScenePicking.h"

#include <optional>

namespace am::editor::viewport
{
    struct ViewportArea final
    {
        double left = 0.0;
        double top = 0.0;
        double width = 0.0;
        double height = 0.0;
    };

    struct ViewportCamera final
    {
        am::core::scene::Vec3d position{};
        am::core::scene::Vec3d forward{0.0, 0.0, 1.0};
        am::core::scene::Vec3d right{1.0, 0.0, 0.0};
        am::core::scene::Vec3d up{0.0, 1.0, 0.0};
        double verticalFieldOfViewRadians = 1.0471975511965976;
        double nearPlane = 0.05;
        double farPlane = 250.0;
    };

    struct ProjectedPoint final
    {
        double x = 0.0;
        double y = 0.0;
        double viewDepth = 0.0;
        bool insideViewport = false;
    };

    class ViewportProjection final
    {
    public:
        [[nodiscard]] static bool valid(const ViewportCamera& camera, const ViewportArea& area) noexcept;
        [[nodiscard]] static std::optional<am::editor::scene::PickRay> screenRay(
            const ViewportCamera& camera, const ViewportArea& area, double screenX, double screenY) noexcept;
        [[nodiscard]] static std::optional<ProjectedPoint> project(
            const ViewportCamera& camera, const ViewportArea& area,
            const am::core::scene::Vec3d& worldPosition) noexcept;
        [[nodiscard]] static std::optional<double> worldUnitsPerPixel(
            const ViewportCamera& camera, const ViewportArea& area, double viewDepth) noexcept;
    };
}
