#include "ArhqenCognitionEngine/Editor/Workspace/AceEditorWorkspaceGeometry.h"

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

    bool inside(const am::editor::WorkspaceRect& inner, const am::editor::WorkspaceRect& outer)
    {
        return inner.left >= outer.left - 0.000001 && inner.top >= outer.top - 0.000001 &&
            inner.right <= outer.right + 0.000001 && inner.bottom <= outer.bottom + 0.000001 &&
            inner.width() >= 0.0 && inner.height() >= 0.0;
    }
}

int main()
{
    using namespace am::editor;

    const WorkspaceRect desktop{0.0, 0.0, 1200.0, 800.0};
    std::string error;
    auto layout = EditorWorkspaceLayout::createDefault();
    auto solved = EditorWorkspaceGeometrySolver::solve(layout, desktop, {}, &error);
    check(solved.has_value(), "default_workspace_geometry_solves");
    if (!solved) return 1;

    check(solved->nodes().size() == 6 && solved->tabs().size() == 3 && solved->splitters().size() == 2,
        "hidden_content_drawer_collapses_geometry");
    const auto* root = solved->findNode("split.root");
    const auto* main = solved->findNode("split.main");
    const auto* viewport = solved->findNode("stack.viewport");
    const auto* right = solved->findNode("split.right");
    const auto* outliner = solved->findNode("stack.outliner");
    const auto* details = solved->findNode("stack.details");
    check(root && main && viewport && right && outliner && details, "all_visible_default_nodes_have_geometry");
    check(root && root->bounds.left == desktop.left && root->bounds.right == desktop.right &&
        main && main->bounds.bottom == desktop.bottom, "single_visible_root_child_fills_workspace");
    check(viewport && right && viewport->bounds.right < right->bounds.left, "main_horizontal_split_has_non_overlapping_panes");
    check(outliner && details && outliner->bounds.bottom < details->bounds.top, "right_vertical_split_has_non_overlapping_panes");
    check(viewport && viewport->tabBar.height() == 28.0 && viewport->content.top == viewport->tabBar.bottom,
        "stack_reserves_stable_tab_bar");

    bool splitterShapesValid = true;
    for (const auto& splitter : solved->splitters())
    {
        const double visual = splitter.orientation == DockOrientation::Horizontal
            ? splitter.visualBounds.width() : splitter.visualBounds.height();
        const double hit = splitter.orientation == DockOrientation::Horizontal
            ? splitter.hitBounds.width() : splitter.hitBounds.height();
        splitterShapesValid = splitterShapesValid && std::abs(visual - 1.0) < 0.000001 && hit >= 8.999;
        const double hitX = (splitter.hitBounds.left + splitter.hitBounds.right) * 0.5;
        const double hitY = (splitter.hitBounds.top + splitter.hitBounds.bottom) * 0.5;
        splitterShapesValid = splitterShapesValid && solved->hitTestSplitter(hitX, hitY) != nullptr;
    }
    check(splitterShapesValid, "splitters_have_thin_visuals_and_generous_hit_targets");

    const auto& viewportTab = solved->tabs().front();
    check(solved->hitTestTab((viewportTab.bounds.left + viewportTab.bounds.right) * 0.5,
        (viewportTab.bounds.top + viewportTab.bounds.bottom) * 0.5) != nullptr,
        "tab_hit_testing_uses_solved_geometry");
    check(solved->hitTestSplitter(-10.0, -10.0) == nullptr && solved->hitTestTab(-10.0, -10.0) == nullptr,
        "geometry_hit_testing_rejects_outside_points");

    check(layout.setTabVisible("tab.content", true, &error), "content_drawer_open_for_geometry");
    auto withDrawer = EditorWorkspaceGeometrySolver::solve(layout, desktop, {}, &error);
    check(withDrawer && withDrawer->nodes().size() == 7 && withDrawer->tabs().size() == 4 && withDrawer->splitters().size() == 3,
        "visible_content_drawer_adds_root_splitter");
    const auto* content = withDrawer ? withDrawer->findNode("stack.content") : nullptr;
    const auto* drawerMain = withDrawer ? withDrawer->findNode("split.main") : nullptr;
    check(content && drawerMain && drawerMain->bounds.bottom < content->bounds.top && content->bounds.bottom == desktop.bottom,
        "content_drawer_is_allocated_below_main_editor");

    check(layout.setChildPairRatio("split.main", 0, 1, 0.70, &error), "adjacent_splitter_pair_ratio_updates");
    auto resized = EditorWorkspaceGeometrySolver::solve(layout, desktop, {}, &error);
    const auto* resizedViewport = resized ? resized->findNode("stack.viewport") : nullptr;
    const auto* resizedRight = resized ? resized->findNode("split.right") : nullptr;
    const double pairWidth = resizedViewport && resizedRight ? resizedViewport->bounds.width() + resizedRight->bounds.width() : 0.0;
    check(pairWidth > 0.0 && std::abs((resizedViewport->bounds.width() / pairWidth) - 0.70) < 0.001,
        "pair_ratio_maps_to_expected_pixel_geometry");
    check(!layout.setChildPairRatio("split.main", 0, 0, 0.5, &error) &&
        !layout.setChildPairRatio("split.main", 0, 8, 0.5, &error),
        "invalid_pair_resize_requests_are_rejected");

    check(layout.setTabVisible("tab.details", false, &error), "details_panel_can_hide");
    auto noDetails = EditorWorkspaceGeometrySolver::solve(layout, desktop, {}, &error);
    const auto* expandedOutliner = noDetails ? noDetails->findNode("stack.outliner") : nullptr;
    const auto* rightContainer = noDetails ? noDetails->findNode("split.right") : nullptr;
    bool rightSplitterAbsent = true;
    if (noDetails)
        for (const auto& splitter : noDetails->splitters()) if (splitter.splitterId == "split.right") rightSplitterAbsent = false;
    check(expandedOutliner && rightContainer && expandedOutliner->bounds.width() == rightContainer->bounds.width() &&
        expandedOutliner->bounds.height() == rightContainer->bounds.height() && rightSplitterAbsent,
        "single_visible_split_child_expands_without_dead_separator");

    auto tabsLayout = EditorWorkspaceLayout::createDefault();
    auto* detailsStack = tabsLayout.findNode("stack.details");
    if (detailsStack)
    {
        detailsStack->tabs.push_back({"tab.properties", "Properties", true, true});
        detailsStack->activeTab = "tab.properties";
    }
    check(tabsLayout.normalizeAndValidate(&error), "multi_tab_stack_validates");
    auto multiTab = EditorWorkspaceGeometrySolver::solve(tabsLayout, desktop, {}, &error);
    std::size_t detailTabs = 0;
    std::size_t activeTabs = 0;
    if (multiTab)
        for (const auto& tab : multiTab->tabs()) if (tab.nodeId == "stack.details") { ++detailTabs; if (tab.active) ++activeTabs; }
    check(detailTabs == 2 && activeTabs == 1, "multi_tab_geometry_has_one_active_tab");

    auto tiny = EditorWorkspaceGeometrySolver::solve(tabsLayout, {5.0, 7.0, 55.0, 47.0}, {}, &error);
    bool tinyInside = tiny.has_value();
    if (tiny)
        for (const auto& node : tiny->nodes()) tinyInside = tinyInside && inside(node.bounds, {5.0, 7.0, 55.0, 47.0});
    check(tinyInside, "tiny_workspace_never_emits_negative_or_outside_rectangles");
    auto microscopic = EditorWorkspaceGeometrySolver::solve(tabsLayout, {2.0, 3.0, 2.5, 3.5}, {}, &error);
    bool microscopicInside = microscopic.has_value();
    if (microscopic)
        for (const auto& node : microscopic->nodes()) microscopicInside = microscopicInside && inside(node.bounds, {2.0, 3.0, 2.5, 3.5});
    check(microscopicInside, "subpixel_workspace_adapts_splitter_thickness_without_inversion");

    WorkspaceGeometryOptions invalidOptions;
    invalidOptions.splitterHitThickness = 0.5;
    check(!EditorWorkspaceGeometrySolver::solve(tabsLayout, desktop, invalidOptions, &error), "invalid_geometry_options_rejected");
    check(!EditorWorkspaceGeometrySolver::solve(tabsLayout,
        {0.0, 0.0, std::numeric_limits<double>::quiet_NaN(), 20.0}, {}, &error),
        "non_finite_workspace_bounds_rejected");
    auto invalidLayout = tabsLayout;
    invalidLayout.root().id = "invalid/id";
    check(!EditorWorkspaceGeometrySolver::solve(invalidLayout, desktop, {}, &error), "invalid_layout_rejected_before_geometry");

    std::cout << (failures == 0 ? "PASS|" : "FAIL|") << "ace_editor_workspace_geometry_probe\n";
    return failures == 0 ? 0 : 1;
}
