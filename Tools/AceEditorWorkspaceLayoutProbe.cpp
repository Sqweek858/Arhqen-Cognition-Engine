#include "ArhqenCognitionEngine/Editor/Workspace/AceEditorWorkspaceLayout.h"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
    int failures = 0;
    void check(bool value, std::string_view name)
    {
        std::cout << (value ? "PASS|" : "FAIL|") << name << '\n';
        if (!value) ++failures;
    }

    double childTotal(const am::editor::EditorDockNode& node)
    {
        double total = 0.0;
        for (const auto& child : node.children) total += child.sizeCoefficient;
        return total;
    }
}

int main()
{
    using namespace am::editor;

    std::string error;
    auto layout = EditorWorkspaceLayout::createDefault();
    check(layout.validate(&error), "default_layout_valid");
    check(layout.root().kind == DockNodeKind::Split && layout.root().orientation == DockOrientation::Vertical,
        "default_root_is_vertical_split");
    check(layout.findNode("stack.viewport") != nullptr && layout.findNode("stack.outliner") != nullptr &&
        layout.findNode("stack.details") != nullptr && layout.findNode("stack.content") != nullptr,
        "default_editor_stacks_exist");
    check(layout.findTab("tab.viewport") && layout.findTab("tab.outliner") && layout.findTab("tab.details") &&
        layout.findTab("tab.content"), "default_editor_tabs_exist");
    check(!layout.isNodeVisible("stack.content"), "content_drawer_starts_hidden");

    check(layout.setTabVisible("tab.content", true, &error) && layout.isNodeVisible("stack.content"),
        "content_drawer_can_open");
    const auto* content = layout.findNode("stack.content");
    check(content && content->activeTab == "tab.content", "opened_drawer_becomes_active");
    check(layout.setTabVisible("tab.content", false, &error) && !layout.isNodeVisible("stack.content"),
        "content_drawer_can_close");
    check(!layout.activateTab("tab.missing", &error) && !error.empty(), "unknown_tab_rejected");

    const auto* mainBefore = layout.findNode("split.main");
    check(mainBefore && mainBefore->children.size() == 2, "main_split_has_expected_children");
    check(layout.setChildCoefficient("split.main", 1, 0.6, &error), "splitter_coefficient_can_change");
    const auto* mainAfter = layout.findNode("split.main");
    check(mainAfter && std::abs(childTotal(*mainAfter) - 1.0) < 0.000001 &&
        mainAfter->children[1].sizeCoefficient > mainAfter->children[0].sizeCoefficient,
        "splitter_coefficients_are_normalized");
    check(!layout.setChildCoefficient("stack.details", 0, 0.5, &error), "stack_cannot_be_resized_as_splitter");
    check(!layout.setChildCoefficient("split.main", 9, 0.5, &error), "invalid_split_child_rejected");
    check(!layout.setChildCoefficient("split.main", 0, -1.0, &error), "invalid_split_coefficient_rejected");

    auto threeWay = EditorWorkspaceLayout::createDefault();
    EditorDockNode extra;
    extra.id = "stack.extra";
    extra.kind = DockNodeKind::Stack;
    extra.sizeCoefficient = 0.1;
    extra.tabs.push_back({"tab.extra", "Extra", true, true});
    extra.activeTab = "tab.extra";
    threeWay.root().children.push_back(std::move(extra));
    check(threeWay.normalizeAndValidate(&error) && threeWay.setChildCoefficient("split.root", 0, 0.99, &error),
        "multi_child_split_resize_clamps_safely");
    check(threeWay.root().children[0].sizeCoefficient <= 0.960001 &&
        threeWay.root().children[1].sizeCoefficient >= 0.019999 &&
        threeWay.root().children[2].sizeCoefficient >= 0.019999 && threeWay.validate(&error),
        "multi_child_split_preserves_recoverable_minimums");

    auto repaired = layout;
    auto* details = repaired.findNode("stack.details");
    check(details != nullptr, "details_stack_found_for_repair_test");
    if (details)
    {
        details->activeTab = "tab.missing";
        details->sizeCoefficient = -4.0;
    }
    check(repaired.normalizeAndValidate(&error), "normalization_repairs_active_tab_and_coefficient");
    details = repaired.findNode("stack.details");
    check(details && details->activeTab == "tab.details" && details->sizeCoefficient > 0.0,
        "normalization_repair_is_deterministic");

    auto duplicate = layout;
    auto* outliner = duplicate.findNode("stack.outliner");
    if (outliner) outliner->id = "stack.details";
    check(!duplicate.validate(&error) && !error.empty(), "duplicate_node_id_rejected");
    auto invalidId = layout;
    invalidId.root().id = "bad/id";
    check(!invalidId.validate(&error), "unsafe_node_id_rejected");
    auto duplicateTab = layout;
    auto* duplicateTabNode = duplicateTab.findNode("stack.details");
    if (duplicateTabNode) duplicateTabNode->tabs[0].id = "tab.outliner";
    check(!duplicateTab.validate(&error), "duplicate_tab_id_rejected");

    auto unicode = layout;
    auto* unicodeTab = unicode.findNode("stack.details");
    if (unicodeTab) unicodeTab->tabs[0].label = "Detalii Èi proprietÄÈi";
    check(unicode.normalizeAndValidate(&error), "unicode_tab_label_validates");

    const auto directory = std::filesystem::current_path() / "Build" / "ACE-EDITOR-WORKSPACE";
    const auto file = directory / "editor-layout.acebin";
    check(unicode.save(file, &error), "workspace_layout_saves_atomically");
    const auto loaded = EditorWorkspaceLayout::load(file, &error);
    check(loaded.has_value() && loaded->validate(&error), "workspace_layout_loads_and_validates");
    const auto* loadedDetails = loaded ? loaded->findNode("stack.details") : nullptr;
    check(loadedDetails && loadedDetails->tabs[0].label == "Detalii Èi proprietÄÈi",
        "workspace_utf8_label_round_trips");
    const auto* loadedMain = loaded ? loaded->findNode("split.main") : nullptr;
    check(loadedMain && std::abs(loadedMain->children[1].sizeCoefficient - mainAfter->children[1].sizeCoefficient) < 0.000001,
        "workspace_split_ratio_round_trips");

    std::vector<char> bytes;
    {
        std::ifstream input(file, std::ios::binary);
        bytes.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
    }
    if (!bytes.empty()) bytes.back() ^= 0x5a;
    const auto corruptedFile = directory / "corrupted.acebin";
    {
        std::ofstream output(corruptedFile, std::ios::binary);
        output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    }
    check(!EditorWorkspaceLayout::load(corruptedFile, &error).has_value(), "corrupted_layout_rejected");
    check(!EditorWorkspaceLayout::load(directory / "missing.acebin", &error).has_value(), "missing_layout_rejected_cleanly");

    auto invalidUtf8 = layout;
    auto* badLabelNode = invalidUtf8.findNode("stack.details");
    if (badLabelNode) badLabelNode->tabs[0].label = std::string("bad") + static_cast<char>(0xc0);
    check(!invalidUtf8.validate(&error), "invalid_utf8_label_rejected_by_model");
    check(!invalidUtf8.save(directory / "bad-utf8.acebin", &error), "invalid_utf8_label_not_persisted");

    layout.resetToDefault();
    check(!layout.isNodeVisible("stack.content") && layout.validate(&error), "reset_restores_known_good_default");

    std::size_t temporaryCount = 0;
    if (std::filesystem::exists(directory))
    {
        for (const auto& entry : std::filesystem::directory_iterator(directory))
            if (entry.path().filename().wstring().find(L".tmp.") != std::wstring::npos) ++temporaryCount;
    }
    check(temporaryCount == 0, "workspace_save_leaves_no_temporary_files");
    std::error_code ec;
    std::filesystem::remove_all(directory, ec);

    std::cout << (failures == 0 ? "PASS|" : "FAIL|") << "ace_editor_workspace_layout_probe\n";
    return failures == 0 ? 0 : 1;
}
