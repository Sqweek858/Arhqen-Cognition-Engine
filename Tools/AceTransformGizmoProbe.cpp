#include "ArhqenCognitionEngine/Editor/Scene/AceTransformGizmo.h"

#include <cmath>
#include <iostream>
#include <string_view>

namespace
{
    int checks = 0, failures = 0;
    template <typename T>
    void check(T&& condition, std::string_view name)
    {
        const bool value = static_cast<bool>(condition);
        ++checks; std::cout << (value ? "PASS|" : "FAIL|") << name << '\n'; if (!value) ++failures;
    }
    bool near(double a, double b) { return std::abs(a - b) < 1.0e-9; }
}

int main()
{
    using namespace am::core::scene;
    using namespace am::editor::scene;
    am::editor::transactions::TransactionManager transactions;
    SceneWorld world;
    SceneSelection selection;
    SceneEditController edits;
    TransformGizmo gizmo;
    std::string error;
    const auto a = world.createEntity(EntityKind::StaticMesh, "A", world.rootId(), {}, &error);
    const auto b = world.createEntity(EntityKind::StaticMesh, "B", world.rootId(), {}, &error);
    check(edits.initialize(world, selection, transactions, &error) && gizmo.initialize(world, edits, &error),
          "gizmo_initializes_over_scene_edit_lifecycle");
    check(near(TransformGizmo::snapValue(1.24, 0.5), 1.0) &&
          near(TransformGizmo::snapValue(-1.26, 0.5), -1.5), "snap_is_symmetric_around_zero");

    GizmoSnapSettings snap;
    snap.translationEnabled = true; snap.translationStep = 0.5;
    gizmo.setSnapSettings(snap); gizmo.setMode(GizmoMode::Translate); gizmo.setSpace(GizmoSpace::World);
    check(gizmo.begin(GizmoAxis::X, {*a, *b}) && gizmo.active(), "world_translate_drag_begins");
    GizmoDelta delta; delta.translation = {1.24, 99.0, -4.0};
    check(gizmo.update(delta) && near(world.find(*a)->transform.location.x, 1.0) &&
          near(world.find(*a)->transform.location.y, 0.0), "axis_filter_and_translation_snap_apply");
    check(gizmo.commit() && transactions.undoCount() == 1, "gizmo_commit_creates_one_transaction");
    check(transactions.undo() && world.find(*a)->transform == Transform{} && world.find(*b)->transform == Transform{},
          "gizmo_multi_target_undo_is_exact");
    check(transactions.redo() && near(world.find(*b)->transform.location.x, 1.0), "gizmo_redo_is_exact");

    Transform rotated = world.find(*a)->transform;
    rotated.rotationDegrees.z = 90.0;
    check(edits.setTransform(*a, rotated), "local_translation_fixture_rotates_entity");
    snap.translationEnabled = false; gizmo.setSnapSettings(snap); gizmo.setSpace(GizmoSpace::Local);
    check(gizmo.begin(GizmoAxis::X, {*a}), "local_axis_drag_begins");
    delta = {}; delta.translation.x = 2.0;
    check(gizmo.update(delta) && near(world.find(*a)->transform.location.x, 1.0) &&
          near(world.find(*a)->transform.location.y, 2.0), "local_x_uses_entity_rotation_basis");
    check(gizmo.cancel() && world.find(*a)->transform == rotated, "gizmo_escape_restores_start_transform");

    gizmo.setMode(GizmoMode::Rotate); gizmo.setSpace(GizmoSpace::World);
    snap.rotationEnabled = true; snap.rotationStepDegrees = 15.0; gizmo.setSnapSettings(snap);
    check(gizmo.begin(GizmoAxis::Z, {*a}), "rotation_drag_begins");
    delta = {}; delta.rotationDegrees = {80.0, 70.0, 22.0};
    check(gizmo.update(delta) && near(world.find(*a)->transform.rotationDegrees.z, 105.0) &&
          near(world.find(*a)->transform.rotationDegrees.x, 0.0), "rotation_axis_and_snap_apply");
    check(gizmo.commit(), "rotation_drag_commits");

    gizmo.setMode(GizmoMode::Scale);
    snap.scaleEnabled = true; snap.scaleStep = 0.25; gizmo.setSnapSettings(snap);
    check(gizmo.begin(GizmoAxis::XYZ, {*b}), "uniform_scale_drag_begins");
    delta = {}; delta.scaleFraction.x = 0.38;
    check(gizmo.update(delta) && world.find(*b)->transform.scale == Vec3d{1.5, 1.5, 1.5},
          "uniform_scale_uses_snapped_fraction");
    check(gizmo.commit(), "uniform_scale_commits");
    check(gizmo.begin(GizmoAxis::X, {*b}), "negative_scale_guard_drag_begins");
    delta = {}; delta.scaleFraction.x = -1.0;
    check(gizmo.update(delta) && near(world.find(*b)->transform.scale.x, 1.0e-6) &&
          near(world.find(*b)->transform.scale.y, 1.5), "scale_never_collapses_to_singular_zero");
    check(gizmo.cancel(), "scale_guard_drag_cancels");
    check(!gizmo.begin(GizmoAxis::None, {*a}) && !gizmo.update({}) && !gizmo.commit() && !gizmo.cancel(),
          "inactive_and_axisless_gizmo_calls_are_rejected");
    check(world.validate(&error), "gizmo_math_preserves_scene_invariants");
    std::cout << "SUMMARY|checks=" << checks << "|failures=" << failures << '\n';
    return failures == 0 ? 0 : 1;
}
