#include "ArhqenCognitionEngine/Editor/Scene/AceSceneEditController.h"

#include <iostream>
#include <limits>
#include <string_view>

namespace
{
    int checks = 0;
    int failures = 0;
    void check(bool condition, std::string_view name)
    {
        ++checks;
        std::cout << (condition ? "PASS|" : "FAIL|") << name << '\n';
        if (!condition) ++failures;
    }
}

int main()
{
    using namespace am::core::scene;
    using am::editor::scene::SceneEditController;
    using am::editor::transactions::TransactionManager;

    SceneWorld world;
    SceneSelection selection;
    TransactionManager transactions;
    SceneEditController controller;
    std::string error;
    const auto first = world.createEntity(EntityKind::StaticMesh, "First", world.rootId(), {}, &error);
    const auto second = world.createEntity(EntityKind::StaticMesh, "Second", world.rootId(), {}, &error);
    const auto locked = world.createEntity(EntityKind::StaticMesh, "Locked", world.rootId(), {}, &error);
    world.setLocked(*locked, true, nullptr);

    check(controller.initialize(world, selection, transactions, &error), "controller_initializes_with_valid_world");
    check(controller.rename(*first, "Renamed") && world.find(*first)->label == "Renamed" &&
          transactions.undoCount() == 1, "rename_records_one_transaction");
    check(transactions.undo() && world.find(*first)->label == "First", "rename_undo_restores_label");
    check(transactions.redo() && world.find(*first)->label == "Renamed", "rename_redo_restores_new_label");
    check(!controller.rename(world.rootId(), "Nope") && !controller.rename(*locked, "Nope"),
          "root_and_locked_entities_cannot_be_renamed");

    Transform direct;
    direct.location = {1.0, 2.0, 3.0};
    check(controller.setTransform(*first, direct) && world.find(*first)->transform == direct,
          "direct_transform_applies");
    check(transactions.undo() && world.find(*first)->transform == Transform{}, "direct_transform_undo_is_exact");
    check(transactions.redo() && world.find(*first)->transform == direct, "direct_transform_redo_is_exact");
    Transform invalid = direct;
    invalid.location.x = std::numeric_limits<double>::infinity();
    const auto historyBeforeInvalid = transactions.undoCount();
    check(!controller.setTransform(*first, invalid) && transactions.undoCount() == historyBeforeInvalid &&
          world.find(*first)->transform == direct, "invalid_direct_transform_does_not_touch_history_or_world");

    check(controller.beginInteractiveTransform({*first, *second, *first}) && controller.transforming() &&
          controller.transformTargets().size() == 2, "interactive_transform_deduplicates_targets");
    Transform firstLive = direct;
    firstLive.location.x = 50.0;
    Transform secondLive;
    secondLive.location.y = 75.0;
    const auto historyBeforeDrag = transactions.undoCount();
    check(controller.updateInteractiveTransform({firstLive, secondLive}) &&
          world.find(*first)->transform == firstLive && world.find(*second)->transform == secondLive,
          "interactive_update_is_live");
    check(transactions.undoCount() == historyBeforeDrag, "interactive_updates_do_not_spam_history");
    check(controller.commitInteractiveTransform() && !controller.transforming() &&
          transactions.undoCount() == historyBeforeDrag + 1, "mouse_up_commits_exactly_one_history_entry");
    check(transactions.undo() && world.find(*first)->transform == direct &&
          world.find(*second)->transform == Transform{}, "multi_transform_undo_restores_every_target");
    check(transactions.redo() && world.find(*first)->transform == firstLive &&
          world.find(*second)->transform == secondLive, "multi_transform_redo_restores_every_target");

    check(controller.beginInteractiveTransform({*first}) &&
          controller.updateInteractiveTransform({direct}), "second_interactive_drag_begins");
    check(controller.cancelInteractiveTransform() && world.find(*first)->transform == firstLive &&
          !controller.transforming(), "escape_cancel_restores_pre_drag_transform");
    check(!transactions.isActive(), "cancel_releases_central_transaction");

    const auto countBeforeNoop = transactions.undoCount();
    check(controller.beginInteractiveTransform({*first}) &&
          controller.updateInteractiveTransform({firstLive}) && controller.commitInteractiveTransform() &&
          transactions.undoCount() == countBeforeNoop, "no_op_drag_creates_no_history_entry");
    check(!controller.beginInteractiveTransform({world.rootId()}) &&
          !controller.beginInteractiveTransform({*locked}), "invalid_interactive_targets_are_rejected");
    check(!controller.updateInteractiveTransform({direct}) && !controller.commitInteractiveTransform() &&
          !controller.cancelInteractiveTransform(), "interactive_calls_require_active_tracking");

    check(static_cast<bool>(controller.beginInteractiveTransform({*first, *second})), "atomic_failure_drag_begins");
    const Transform beforeFirstFailure = world.find(*first)->transform;
    const Transform beforeSecondFailure = world.find(*second)->transform;
    check(!controller.updateInteractiveTransform({direct, invalid}) &&
          world.find(*first)->transform == beforeFirstFailure && world.find(*second)->transform == beforeSecondFailure,
          "partial_invalid_update_rolls_back_all_applied_targets");
    check(static_cast<bool>(controller.cancelInteractiveTransform()), "failed_drag_can_still_cancel_cleanly");

    SceneEditController unavailable;
    check(!unavailable.rename(*first, "No") && !unavailable.setTransform(*first, direct) &&
          !unavailable.beginInteractiveTransform({*first}), "uninitialized_controller_is_inert");
    check(world.validate(&error), "scene_remains_valid_after_all_edit_lifecycles");

    std::cout << "SUMMARY|checks=" << checks << "|failures=" << failures << '\n';
    return failures == 0 ? 0 : 1;
}
