#include "ArhqenCognitionEngine/Editor/Scene/AceTransformGizmo.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace am::editor::scene
{
    namespace
    {
        constexpr double pi = 3.1415926535897932384626433832795;
        constexpr double minimumScaleMagnitude = 1.0e-6;

        double clampScale(double value) noexcept
        {
            if (std::abs(value) >= minimumScaleMagnitude) return value;
            return std::signbit(value) ? -minimumScaleMagnitude : minimumScaleMagnitude;
        }
    }

    bool TransformGizmo::initialize(am::core::scene::SceneWorld& world, SceneEditController& edits,
                                    std::string* error)
    {
        if (error) error->clear();
        if (!edits.initialized() || !world.validate(error)) return false;
        world_ = &world; edits_ = &edits; return true;
    }

    SceneEditResult TransformGizmo::failure(std::string message) const
    {
        return {false, std::move(message)};
    }

    bool TransformGizmo::axisContains(GizmoAxis axis, GizmoAxis component) noexcept
    {
        return (static_cast<unsigned>(axis) & static_cast<unsigned>(component)) != 0;
    }

    am::core::scene::Vec3d TransformGizmo::filter(am::core::scene::Vec3d value) const noexcept
    {
        if (!axisContains(activeAxis_, GizmoAxis::X)) value.x = 0.0;
        if (!axisContains(activeAxis_, GizmoAxis::Y)) value.y = 0.0;
        if (!axisContains(activeAxis_, GizmoAxis::Z)) value.z = 0.0;
        return value;
    }

    double TransformGizmo::snapValue(double value, double step) noexcept
    {
        if (!std::isfinite(value) || !std::isfinite(step) || step <= 0.0) return value;
        return std::round(value / step) * step;
    }

    am::core::scene::Vec3d TransformGizmo::rotateLocalToWorld(
        const am::core::scene::Vec3d& value, const am::core::scene::Vec3d& eulerDegrees) noexcept
    {
        const double x = eulerDegrees.x * pi / 180.0;
        const double y = eulerDegrees.y * pi / 180.0;
        const double z = eulerDegrees.z * pi / 180.0;
        const double cx = std::cos(x), sx = std::sin(x);
        const double cy = std::cos(y), sy = std::sin(y);
        const double cz = std::cos(z), sz = std::sin(z);
        // Rz * Ry * Rx, matching the editor's XYZ fields and a right-handed world.
        return {
            (cz * cy) * value.x + (cz * sy * sx - sz * cx) * value.y + (cz * sy * cx + sz * sx) * value.z,
            (sz * cy) * value.x + (sz * sy * sx + cz * cx) * value.y + (sz * sy * cx - cz * sx) * value.z,
            (-sy) * value.x + (cy * sx) * value.y + (cy * cx) * value.z};
    }

    SceneEditResult TransformGizmo::begin(GizmoAxis axis, const std::vector<am::core::Guid>& targets)
    {
        if (!world_ || !edits_ || active_ || axis == GizmoAxis::None || targets.empty())
            return failure("Transform gizmo cannot begin with this state or axis");
        const auto result = edits_->beginInteractiveTransform(targets);
        if (!result) return result;
        targets_ = edits_->transformTargets();
        startTransforms_.clear(); startTransforms_.reserve(targets_.size());
        for (const auto& id : targets_)
        {
            const auto* entity = world_->find(id);
            if (!entity)
            {
                edits_->cancelInteractiveTransform(); targets_.clear(); startTransforms_.clear();
                return failure("Transform target disappeared during begin");
            }
            startTransforms_.push_back(entity->transform);
        }
        activeAxis_ = axis; active_ = true; return {true, {}};
    }

    SceneEditResult TransformGizmo::update(const GizmoDelta& accumulatedDelta)
    {
        if (!active_ || !edits_) return failure("Transform gizmo is not active");
        std::vector<am::core::scene::Transform> values = startTransforms_;
        for (std::size_t index = 0; index < values.size(); ++index)
        {
            if (mode_ == GizmoMode::Translate)
            {
                auto delta = filter(accumulatedDelta.translation);
                if (snap_.translationEnabled)
                {
                    delta.x = snapValue(delta.x, snap_.translationStep);
                    delta.y = snapValue(delta.y, snap_.translationStep);
                    delta.z = snapValue(delta.z, snap_.translationStep);
                }
                if (space_ == GizmoSpace::Local) delta = rotateLocalToWorld(delta, startTransforms_[index].rotationDegrees);
                values[index].location.x += delta.x; values[index].location.y += delta.y; values[index].location.z += delta.z;
            }
            else if (mode_ == GizmoMode::Rotate)
            {
                auto delta = filter(accumulatedDelta.rotationDegrees);
                if (snap_.rotationEnabled)
                {
                    delta.x = snapValue(delta.x, snap_.rotationStepDegrees);
                    delta.y = snapValue(delta.y, snap_.rotationStepDegrees);
                    delta.z = snapValue(delta.z, snap_.rotationStepDegrees);
                }
                values[index].rotationDegrees.x += delta.x; values[index].rotationDegrees.y += delta.y;
                values[index].rotationDegrees.z += delta.z;
            }
            else
            {
                auto delta = filter(accumulatedDelta.scaleFraction);
                if (activeAxis_ == GizmoAxis::XYZ)
                {
                    const double uniform = accumulatedDelta.scaleFraction.x;
                    delta = {uniform, uniform, uniform};
                }
                if (snap_.scaleEnabled)
                {
                    delta.x = snapValue(delta.x, snap_.scaleStep);
                    delta.y = snapValue(delta.y, snap_.scaleStep);
                    delta.z = snapValue(delta.z, snap_.scaleStep);
                }
                values[index].scale.x = clampScale(startTransforms_[index].scale.x * (1.0 + delta.x));
                values[index].scale.y = clampScale(startTransforms_[index].scale.y * (1.0 + delta.y));
                values[index].scale.z = clampScale(startTransforms_[index].scale.z * (1.0 + delta.z));
            }
        }
        return edits_->updateInteractiveTransform(values);
    }

    SceneEditResult TransformGizmo::commit()
    {
        if (!active_ || !edits_) return failure("Transform gizmo is not active");
        const auto result = edits_->commitInteractiveTransform();
        if (result) { active_ = false; activeAxis_ = GizmoAxis::None; targets_.clear(); startTransforms_.clear(); }
        return result;
    }

    SceneEditResult TransformGizmo::cancel()
    {
        if (!active_ || !edits_) return failure("Transform gizmo is not active");
        const auto result = edits_->cancelInteractiveTransform();
        if (result) { active_ = false; activeAxis_ = GizmoAxis::None; targets_.clear(); startTransforms_.clear(); }
        return result;
    }
}
