#include "ArhqenCognitionEngine/Editor/Scene/AceScenePicking.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace am::editor::scene
{
    namespace
    {
        using am::core::scene::Vec3d;
        constexpr double pi = 3.1415926535897932384626433832795;
        constexpr double parallelEpsilon = 1.0e-12;

        bool finite(const Vec3d& value) noexcept
        { return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z); }

        Vec3d rotate(const Vec3d& value, const Vec3d& degrees) noexcept
        {
            const double x = degrees.x * pi / 180.0, y = degrees.y * pi / 180.0, z = degrees.z * pi / 180.0;
            const double cx = std::cos(x), sx = std::sin(x), cy = std::cos(y), sy = std::sin(y);
            const double cz = std::cos(z), sz = std::sin(z);
            return {
                (cz * cy) * value.x + (cz * sy * sx - sz * cx) * value.y + (cz * sy * cx + sz * sx) * value.z,
                (sz * cy) * value.x + (sz * sy * sx + cz * cx) * value.y + (sz * sy * cx - cz * sx) * value.z,
                (-sy) * value.x + (cy * sx) * value.y + (cy * cx) * value.z};
        }

        std::optional<double> intersect(const PickRay& ray, const AxisAlignedBounds& bounds,
                                        double maximumDistance) noexcept
        {
            double nearDistance = 0.0;
            double farDistance = maximumDistance;
            const double origins[3] = {ray.origin.x, ray.origin.y, ray.origin.z};
            const double directions[3] = {ray.direction.x, ray.direction.y, ray.direction.z};
            const double minimums[3] = {bounds.minimum.x, bounds.minimum.y, bounds.minimum.z};
            const double maximums[3] = {bounds.maximum.x, bounds.maximum.y, bounds.maximum.z};
            for (std::size_t axis = 0; axis < 3; ++axis)
            {
                if (std::abs(directions[axis]) <= parallelEpsilon)
                {
                    if (origins[axis] < minimums[axis] || origins[axis] > maximums[axis]) return std::nullopt;
                    continue;
                }
                const double inverse = 1.0 / directions[axis];
                double first = (minimums[axis] - origins[axis]) * inverse;
                double second = (maximums[axis] - origins[axis]) * inverse;
                if (first > second) std::swap(first, second);
                nearDistance = std::max(nearDistance, first);
                farDistance = std::min(farDistance, second);
                if (nearDistance > farDistance) return std::nullopt;
            }
            return nearDistance <= maximumDistance && farDistance >= 0.0 ? std::optional<double>(nearDistance) : std::nullopt;
        }
    }

    bool ScenePicker::validBounds(const AxisAlignedBounds& bounds) noexcept
    {
        return finite(bounds.minimum) && finite(bounds.maximum) &&
            bounds.minimum.x <= bounds.maximum.x && bounds.minimum.y <= bounds.maximum.y &&
            bounds.minimum.z <= bounds.maximum.z;
    }

    bool ScenePicker::upsert(ScenePickProxy proxy)
    {
        if (!proxy.entityId.isValid() || !validBounds(proxy.worldBounds)) return false;
        proxies_.insert_or_assign(proxy.entityId, std::move(proxy));
        return true;
    }

    bool ScenePicker::remove(const am::core::Guid& entityId) noexcept { return proxies_.erase(entityId) != 0; }

    std::optional<ScenePickHit> ScenePicker::raycast(const PickRay& ray, double maximumDistance) const noexcept
    {
        if (!finite(ray.origin) || !finite(ray.direction) || !std::isfinite(maximumDistance) || maximumDistance < 0.0)
            return std::nullopt;
        const double length = std::sqrt(ray.direction.x * ray.direction.x + ray.direction.y * ray.direction.y +
                                        ray.direction.z * ray.direction.z);
        if (!std::isfinite(length) || length <= parallelEpsilon) return std::nullopt;
        PickRay normalized = ray;
        normalized.direction = {ray.direction.x / length, ray.direction.y / length, ray.direction.z / length};
        std::optional<ScenePickHit> best;
        for (const auto& [id, proxy] : proxies_)
        {
            (void)id;
            if (!proxy.visible || !proxy.selectable) continue;
            const auto distance = intersect(normalized, proxy.worldBounds, maximumDistance);
            if (!distance) continue;
            const bool closer = !best || *distance < best->distance - parallelEpsilon;
            const bool tiedHigherPriority = best && std::abs(*distance - best->distance) <= parallelEpsilon &&
                proxy.priority > best->priority;
            const bool tiedStableIdentity = best && std::abs(*distance - best->distance) <= parallelEpsilon &&
                proxy.priority == best->priority && proxy.entityId < best->entityId;
            if (!closer && !tiedHigherPriority && !tiedStableIdentity) continue;
            best = ScenePickHit{proxy.entityId, *distance,
                {normalized.origin.x + normalized.direction.x * *distance,
                 normalized.origin.y + normalized.direction.y * *distance,
                 normalized.origin.z + normalized.direction.z * *distance}, proxy.priority};
        }
        return best;
    }

    AxisAlignedBounds ScenePicker::transformBounds(const AxisAlignedBounds& localBounds,
                                                    const am::core::scene::Transform& transform) noexcept
    {
        if (!validBounds(localBounds)) return {};
        const double infinity = std::numeric_limits<double>::infinity();
        AxisAlignedBounds result{{infinity, infinity, infinity}, {-infinity, -infinity, -infinity}};
        for (unsigned corner = 0; corner < 8; ++corner)
        {
            Vec3d value{
                (corner & 1u) ? localBounds.maximum.x : localBounds.minimum.x,
                (corner & 2u) ? localBounds.maximum.y : localBounds.minimum.y,
                (corner & 4u) ? localBounds.maximum.z : localBounds.minimum.z};
            value = {value.x * transform.scale.x, value.y * transform.scale.y, value.z * transform.scale.z};
            value = rotate(value, transform.rotationDegrees);
            value = {value.x + transform.location.x, value.y + transform.location.y, value.z + transform.location.z};
            result.minimum.x = std::min(result.minimum.x, value.x); result.maximum.x = std::max(result.maximum.x, value.x);
            result.minimum.y = std::min(result.minimum.y, value.y); result.maximum.y = std::max(result.maximum.y, value.y);
            result.minimum.z = std::min(result.minimum.z, value.z); result.maximum.z = std::max(result.maximum.z, value.z);
        }
        return result;
    }
}
