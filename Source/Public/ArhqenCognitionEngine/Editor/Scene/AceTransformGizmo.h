#pragma once

#include "ArhqenCognitionEngine/Editor/Scene/AceSceneEditController.h"

#include <cstdint>
#include <string>
#include <vector>

namespace am::editor::scene
{
    enum class GizmoMode : std::uint8_t { Translate, Rotate, Scale };
    enum class GizmoSpace : std::uint8_t { World, Local };
    enum class GizmoAxis : std::uint8_t
    {
        None = 0,
        X = 1,
        Y = 2,
        Z = 4,
        XY = 3,
        XZ = 5,
        YZ = 6,
        XYZ = 7
    };

    struct GizmoSnapSettings final
    {
        bool translationEnabled = false;
        bool rotationEnabled = false;
        bool scaleEnabled = false;
        double translationStep = 1.0;
        double rotationStepDegrees = 10.0;
        double scaleStep = 0.1;
    };

    struct GizmoDelta final
    {
        am::core::scene::Vec3d translation{};
        am::core::scene::Vec3d rotationDegrees{};
        am::core::scene::Vec3d scaleFraction{};
    };

    class TransformGizmo final
    {
    public:
        bool initialize(am::core::scene::SceneWorld& world, SceneEditController& edits,
                        std::string* error = nullptr);
        void setMode(GizmoMode mode) noexcept { mode_ = mode; }
        void setSpace(GizmoSpace space) noexcept { space_ = space; }
        void setSnapSettings(GizmoSnapSettings settings) noexcept { snap_ = settings; }
        [[nodiscard]] GizmoMode mode() const noexcept { return mode_; }
        [[nodiscard]] GizmoSpace space() const noexcept { return space_; }
        [[nodiscard]] const GizmoSnapSettings& snapSettings() const noexcept { return snap_; }

        SceneEditResult begin(GizmoAxis axis, const std::vector<am::core::Guid>& targets);
        SceneEditResult update(const GizmoDelta& accumulatedDelta);
        SceneEditResult commit();
        SceneEditResult cancel();
        [[nodiscard]] bool active() const noexcept { return active_; }
        [[nodiscard]] GizmoAxis activeAxis() const noexcept { return activeAxis_; }

        [[nodiscard]] static double snapValue(double value, double step) noexcept;

    private:
        [[nodiscard]] static bool axisContains(GizmoAxis axis, GizmoAxis component) noexcept;
        [[nodiscard]] static am::core::scene::Vec3d rotateLocalToWorld(
            const am::core::scene::Vec3d& value, const am::core::scene::Vec3d& eulerDegrees) noexcept;
        [[nodiscard]] am::core::scene::Vec3d filter(am::core::scene::Vec3d value) const noexcept;
        [[nodiscard]] SceneEditResult failure(std::string message) const;

        am::core::scene::SceneWorld* world_ = nullptr;
        SceneEditController* edits_ = nullptr;
        GizmoMode mode_ = GizmoMode::Translate;
        GizmoSpace space_ = GizmoSpace::World;
        GizmoSnapSettings snap_{};
        bool active_ = false;
        GizmoAxis activeAxis_ = GizmoAxis::None;
        std::vector<am::core::Guid> targets_;
        std::vector<am::core::scene::Transform> startTransforms_;
    };
}
