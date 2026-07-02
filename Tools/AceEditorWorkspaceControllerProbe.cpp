#include "ArhqenCognitionEngine/Editor/Workspace/AceEditorWorkspaceController.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <string_view>
#include <utility>

namespace
{
    int failures = 0;
    void check(bool value, std::string_view name)
    {
        std::cout << (value ? "PASS|" : "FAIL|") << name << '\n';
        if (!value) ++failures;
    }

    std::pair<double, double> center(const am::editor::WorkspaceRect& rect)
    {
        return {(rect.left + rect.right) * 0.5, (rect.top + rect.bottom) * 0.5};
    }
}

int main()
{
    using namespace am::editor;

    std::string error;
    EditorWorkspaceController controller;
    check(controller.geometry() == nullptr && !controller.dirty() && !controller.draggingSplitter(),
        "controller_starts_unarranged_and_clean");
    check(controller.arrange({0.0, 0.0, 1200.0, 800.0}, {}, &error) && controller.geometry(),
        "controller_arranges_default_workspace");

    const auto* mainSplitter = controller.geometry()->hitTestSplitter(864.0, 300.0);
    if (!mainSplitter)
    {
        for (const auto& candidate : controller.geometry()->splitters())
            if (candidate.splitterId == "split.main") { mainSplitter = &candidate; break; }
    }
    check(mainSplitter != nullptr, "main_splitter_is_interactive");
    if (!mainSplitter) return 1;
    const auto [splitX, splitY] = center(mainSplitter->hitBounds);
    check(controller.cursorAt(splitX, splitY) == WorkspaceCursor::ResizeHorizontal,
        "hover_cursor_matches_splitter_orientation");
    check(controller.pointerDown(splitX, splitY, &error) && controller.draggingSplitter(),
        "pointer_down_captures_splitter");
    check(controller.cursorAt(-100.0, -100.0) == WorkspaceCursor::ResizeHorizontal,
        "captured_splitter_cursor_survives_pointer_leaving_bounds");
    check(controller.pointerUp(splitX, splitY, &error) && !controller.draggingSplitter() &&
        !controller.dirty() && !controller.takeCommitRequested(),
        "click_without_drag_does_not_dirty_or_commit_layout");

    const auto* mainBefore = controller.layout().findNode("split.main");
    const double ratioBefore = mainBefore ? mainBefore->children[0].sizeCoefficient : 0.0;
    check(controller.pointerDown(splitX, splitY, &error) && controller.pointerMove(splitX - 120.0, splitY, &error),
        "splitter_drag_updates_layout_live");
    const auto* mainMoved = controller.layout().findNode("split.main");
    check(mainMoved && mainMoved->children[0].sizeCoefficient < ratioBefore && controller.dirty() &&
        !controller.takeCommitRequested(), "live_drag_is_dirty_but_not_persistable_before_release");
    check(controller.cancelPointerInteraction(&error) && !controller.draggingSplitter(),
        "escape_style_cancel_releases_splitter_capture");
    const auto* mainCancelled = controller.layout().findNode("split.main");
    check(mainCancelled && std::abs(mainCancelled->children[0].sizeCoefficient - ratioBefore) < 0.000001 &&
        !controller.dirty() && !controller.takeCommitRequested(),
        "cancel_restores_exact_pre_drag_layout_and_flags");

    check(controller.pointerDown(splitX, splitY, &error) && controller.pointerMove(splitX + 80.0, splitY, &error) &&
        controller.pointerUp(splitX + 80.0, splitY, &error), "completed_splitter_drag_releases_capture");
    check(controller.dirty() && controller.takeCommitRequested() && !controller.takeCommitRequested(),
        "completed_drag_requests_one_persistence_commit");
    controller.markSaved();
    check(!controller.dirty(), "successful_persistence_can_mark_controller_clean");

    const auto* updatedSplitter = controller.geometry()->hitTestSplitter(splitX + 80.0, splitY);
    if (!updatedSplitter)
    {
        for (const auto& candidate : controller.geometry()->splitters())
            if (candidate.splitterId == "split.main") { updatedSplitter = &candidate; break; }
    }
    check(updatedSplitter != nullptr, "splitter_geometry_rebuilds_after_drag");
    if (updatedSplitter)
    {
        const auto [newX, newY] = center(updatedSplitter->hitBounds);
        const double coefficientBeforeResizeCancel = controller.layout().findNode("split.main")->children[0].sizeCoefficient;
        check(controller.pointerDown(newX, newY, &error) && controller.pointerMove(newX - 40.0, newY, &error),
            "second_drag_begins_before_window_resize");
        check(controller.arrange({0.0, 0.0, 1000.0, 700.0}, {}, &error) && !controller.draggingSplitter(),
            "window_rearrange_cancels_active_drag_safely");
        check(std::abs(controller.layout().findNode("split.main")->children[0].sizeCoefficient - coefficientBeforeResizeCancel) < 0.000001,
            "window_rearrange_restores_pre_drag_ratio");
    }

    check(controller.setTabVisible("tab.content", true, &error) && controller.layout().findTab("tab.content")->visible,
        "controller_opens_content_drawer");
    check(controller.geometry()->findNode("stack.content") != nullptr && controller.takeCommitRequested(),
        "drawer_toggle_rebuilds_geometry_and_requests_commit");
    check(controller.setTabVisible("tab.content", true, &error) && !controller.takeCommitRequested(),
        "no_op_visibility_change_avoids_commit");
    check(!controller.setTabVisible("tab.missing", true, &error) && !error.empty(),
        "controller_reports_unknown_tab");

    auto tabbedLayout = EditorWorkspaceLayout::createDefault();
    auto* detailStack = tabbedLayout.findNode("stack.details");
    if (detailStack) detailStack->tabs.push_back({"tab.properties", "Properties", true, true});
    tabbedLayout.normalizeAndValidate(&error);
    EditorWorkspaceController tabController(tabbedLayout);
    check(tabController.arrange({0.0, 0.0, 900.0, 600.0}, {}, &error), "tab_controller_arranges");
    const WorkspaceTabGeometry* propertiesTab = nullptr;
    for (const auto& tab : tabController.geometry()->tabs()) if (tab.tabId == "tab.properties") propertiesTab = &tab;
    check(propertiesTab != nullptr, "inactive_tab_has_click_geometry");
    if (propertiesTab)
    {
        const auto [tabX, tabY] = center(propertiesTab->bounds);
        check(tabController.pointerDown(tabX, tabY, &error) &&
            tabController.layout().findNode("stack.details")->activeTab == "tab.properties",
            "tab_click_activates_tab");
        check(tabController.dirty() && tabController.takeCommitRequested(),
            "tab_activation_requests_persistence");
    }

    check(!controller.pointerDown(-20.0, -20.0, &error) && !controller.pointerMove(0.0, 0.0, &error) &&
        !controller.pointerUp(0.0, 0.0, &error), "unhandled_pointer_events_do_not_capture");

    const auto* anySplitter = controller.geometry()->splitters().empty() ? nullptr : &controller.geometry()->splitters().front();
    if (anySplitter)
    {
        const auto [x, y] = center(anySplitter->hitBounds);
        check(controller.pointerDown(x, y, &error) &&
            !controller.pointerMove(std::numeric_limits<double>::quiet_NaN(), y, &error) &&
            controller.draggingSplitter(), "non_finite_drag_input_is_rejected_without_losing_capture");
        check(controller.cancelPointerInteraction(&error), "invalid_drag_can_still_cancel_cleanly");
    }

    auto invalidLayout = EditorWorkspaceLayout::createDefault();
    invalidLayout.root().id = "bad/id";
    EditorWorkspaceController recovered(std::move(invalidLayout));
    check(recovered.layout().validate(&error) && recovered.layout().findNode("split.root"),
        "invalid_constructor_layout_falls_back_to_default");
    check(controller.resetLayout(&error) && !controller.layout().findTab("tab.content")->visible &&
        controller.takeCommitRequested(), "reset_restores_default_and_requests_commit");

    std::cout << (failures == 0 ? "PASS|" : "FAIL|") << "ace_editor_workspace_controller_probe\n";
    return failures == 0 ? 0 : 1;
}
