#pragma once

#include "ArhqenCognitionEngine/Core/Scene/AceSceneWorld.h"

#include <cstddef>
#include <optional>
#include <unordered_map>

namespace am::editor::scene
{
    struct PickRay final
    {
        am::core::scene::Vec3d origin{};
        am::core::scene::Vec3d direction{0.0, 0.0, 1.0};
    };

    struct AxisAlignedBounds final
    {
        am::core::scene::Vec3d minimum{};
        am::core::scene::Vec3d maximum{};
    };

    struct ScenePickProxy final
    {
        am::core::Guid entityId{};
        AxisAlignedBounds worldBounds{};
        bool visible = true;
        bool selectable = true;
        std::int32_t priority = 0;
    };

    struct ScenePickHit final
    {
        am::core::Guid entityId{};
        double distance = 0.0;
        am::core::scene::Vec3d position{};
        std::int32_t priority = 0;
    };

    class ScenePicker final
    {
    public:
        bool upsert(ScenePickProxy proxy);
        bool remove(const am::core::Guid& entityId) noexcept;
        void clear() noexcept { proxies_.clear(); }
        [[nodiscard]] std::size_t size() const noexcept { return proxies_.size(); }
        [[nodiscard]] std::optional<ScenePickHit> raycast(const PickRay& ray,
                                                          double maximumDistance = 1.0e15) const noexcept;

        [[nodiscard]] static bool validBounds(const AxisAlignedBounds& bounds) noexcept;
        [[nodiscard]] static AxisAlignedBounds transformBounds(
            const AxisAlignedBounds& localBounds, const am::core::scene::Transform& transform) noexcept;

    private:
        std::unordered_map<am::core::Guid, ScenePickProxy, am::core::GuidHash> proxies_;
    };
}
