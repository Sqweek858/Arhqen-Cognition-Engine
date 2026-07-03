#include "ArhqenCognitionEngine/Editor/Viewport/AceViewportProjection.h"

#include <algorithm>
#include <cmath>

namespace am::editor::viewport
{
    namespace
    {
        using am::core::scene::Vec3d;
        constexpr double epsilon = 1.0e-10;
        constexpr double pi = 3.1415926535897932384626433832795;

        bool finite(const Vec3d& value) noexcept
        { return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z); }

        double dot(const Vec3d& left, const Vec3d& right) noexcept
        { return left.x * right.x + left.y * right.y + left.z * right.z; }

        Vec3d subtract(const Vec3d& left, const Vec3d& right) noexcept
        { return {left.x - right.x, left.y - right.y, left.z - right.z}; }

        Vec3d multiply(const Vec3d& value, double scalar) noexcept
        { return {value.x * scalar, value.y * scalar, value.z * scalar}; }

        Vec3d cross(const Vec3d& left, const Vec3d& right) noexcept
        {
            return {left.y * right.z - left.z * right.y,
                    left.z * right.x - left.x * right.z,
                    left.x * right.y - left.y * right.x};
        }

        std::optional<Vec3d> normalized(const Vec3d& value) noexcept
        {
            if (!finite(value)) return std::nullopt;
            const double lengthSquared = dot(value, value);
            if (!std::isfinite(lengthSquared) || lengthSquared <= epsilon * epsilon) return std::nullopt;
            const double inverseLength = 1.0 / std::sqrt(lengthSquared);
            return multiply(value, inverseLength);
        }

        struct Basis final { Vec3d forward{}, right{}, up{}; };

        std::optional<Basis> basis(const ViewportCamera& camera) noexcept
        {
            const auto forward = normalized(camera.forward);
            if (!forward) return std::nullopt;
            const auto right = normalized(subtract(camera.right, multiply(*forward, dot(camera.right, *forward))));
            if (!right) return std::nullopt;
            const auto up = normalized(cross(*forward, *right));
            const auto requestedUp = normalized(camera.up);
            if (!up || !requestedUp || dot(*up, *requestedUp) <= 0.0) return std::nullopt;
            return Basis{*forward, *right, *up};
        }
    }

    bool ViewportProjection::valid(const ViewportCamera& camera, const ViewportArea& area) noexcept
    {
        return finite(camera.position) && basis(camera).has_value() &&
            std::isfinite(area.left) && std::isfinite(area.top) &&
            std::isfinite(area.width) && std::isfinite(area.height) &&
            area.width > epsilon && area.height > epsilon &&
            std::isfinite(camera.verticalFieldOfViewRadians) &&
            camera.verticalFieldOfViewRadians > epsilon && camera.verticalFieldOfViewRadians < pi - epsilon &&
            std::isfinite(camera.nearPlane) && std::isfinite(camera.farPlane) &&
            camera.nearPlane > epsilon && camera.farPlane > camera.nearPlane;
    }

    std::optional<am::editor::scene::PickRay> ViewportProjection::screenRay(
        const ViewportCamera& camera, const ViewportArea& area, double screenX, double screenY) noexcept
    {
        if (!valid(camera, area) || !std::isfinite(screenX) || !std::isfinite(screenY) ||
            screenX < area.left || screenX > area.left + area.width ||
            screenY < area.top || screenY > area.top + area.height) return std::nullopt;
        const auto axes = basis(camera);
        const double ndcX = ((screenX - area.left) / area.width) * 2.0 - 1.0;
        const double ndcY = 1.0 - ((screenY - area.top) / area.height) * 2.0;
        const double tangent = std::tan(camera.verticalFieldOfViewRadians * 0.5);
        const double aspect = area.width / area.height;
        Vec3d direction{
            axes->forward.x + axes->right.x * ndcX * tangent * aspect + axes->up.x * ndcY * tangent,
            axes->forward.y + axes->right.y * ndcX * tangent * aspect + axes->up.y * ndcY * tangent,
            axes->forward.z + axes->right.z * ndcX * tangent * aspect + axes->up.z * ndcY * tangent};
        const auto normalizedDirection = normalized(direction);
        if (!normalizedDirection) return std::nullopt;
        const Vec3d nearPlaneOrigin{
            camera.position.x + direction.x * camera.nearPlane,
            camera.position.y + direction.y * camera.nearPlane,
            camera.position.z + direction.z * camera.nearPlane};
        return am::editor::scene::PickRay{nearPlaneOrigin, *normalizedDirection};
    }

    std::optional<ProjectedPoint> ViewportProjection::project(
        const ViewportCamera& camera, const ViewportArea& area, const Vec3d& worldPosition) noexcept
    {
        if (!valid(camera, area) || !finite(worldPosition)) return std::nullopt;
        const auto axes = basis(camera);
        const Vec3d relative = subtract(worldPosition, camera.position);
        const double depth = dot(relative, axes->forward);
        if (!std::isfinite(depth) || depth < camera.nearPlane || depth > camera.farPlane) return std::nullopt;
        const double tangent = std::tan(camera.verticalFieldOfViewRadians * 0.5);
        const double ndcX = dot(relative, axes->right) / (depth * tangent * (area.width / area.height));
        const double ndcY = dot(relative, axes->up) / (depth * tangent);
        ProjectedPoint result;
        result.x = area.left + (ndcX * 0.5 + 0.5) * area.width;
        result.y = area.top + (0.5 - ndcY * 0.5) * area.height;
        result.viewDepth = depth;
        result.insideViewport = ndcX >= -1.0 && ndcX <= 1.0 && ndcY >= -1.0 && ndcY <= 1.0;
        return result;
    }

    std::optional<double> ViewportProjection::worldUnitsPerPixel(
        const ViewportCamera& camera, const ViewportArea& area, double viewDepth) noexcept
    {
        if (!valid(camera, area) || !std::isfinite(viewDepth) || viewDepth < camera.nearPlane ||
            viewDepth > camera.farPlane) return std::nullopt;
        return 2.0 * viewDepth * std::tan(camera.verticalFieldOfViewRadians * 0.5) / area.height;
    }
}
