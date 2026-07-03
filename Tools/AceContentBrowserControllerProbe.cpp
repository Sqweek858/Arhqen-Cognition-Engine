#include "ArhqenCognitionEngine/Editor/ContentBrowser/AceContentBrowserController.h"

#include <filesystem>
#include <fstream>
#include <iostream>
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

    void write(const std::filesystem::path& path, std::string_view bytes)
    {
        std::filesystem::create_directories(path.parent_path());
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    }
}

int main()
{
    using am::core::assets::AssetPath;
    using am::core::assets::AssetReferenceIndex;
    using am::core::assets::AssetRegistry;
    using am::editor::assets::AssetOperationService;
    using am::editor::content_browser::ContentBrowserController;
    using am::editor::content_browser::ContentBrowserModel;
    using am::editor::content_browser::SelectionMode;
    using am::editor::transactions::TransactionManager;

    const auto base = std::filesystem::current_path() / "Build" / "ACE-CONTENT-BROWSER-CONTROLLER" / "runtime";
    const auto content = base / "Content";
    const auto state = base / "State" / "registry.acebin";
    const auto undo = base / "State" / "Undo";
    std::error_code ec;
    std::filesystem::remove_all(base, ec);
    std::filesystem::create_directories(content / "Materials");
    write(content / "Materials" / "M_Rock.acemat", "rock-material");

    AssetRegistry registry;
    AssetReferenceIndex references;
    TransactionManager transactions;
    AssetOperationService operations;
    ContentBrowserModel model;
    ContentBrowserController controller;
    std::string error;

    check(registry.initialize(content, state, &error), "registry_initialized");
    check(operations.initialize(registry, references, transactions, undo, &error), "operations_initialized");
    check(controller.initialize(registry, operations, model, &error), "controller_initialized");
    check(model.currentFolder().isRoot() && model.visibleItems().size() == 1, "controller_publishes_initial_snapshot");

    const auto created = controller.createFolder("NewFolder");
    check(created && created.path.string() == "/Game/NewFolder", "create_folder_uses_current_folder");
    check(std::filesystem::is_directory(content / "NewFolder") && model.selectionCount() == 1,
          "created_folder_is_published_and_selected");
    check(!controller.createFolder("Bad.Name") && !std::filesystem::exists(content / "Bad.Name"),
          "invalid_folder_name_does_not_mutate_disk");

    const auto materials = AssetPath::parse("/Game/Materials");
    check(materials && model.navigateTo(*materials), "navigate_to_materials");
    check(model.visibleItems().size() == 1 && model.selectVisible(0, SelectionMode::Replace), "select_asset_for_action");
    const auto originalId = model.selectedItems().front().assetId;
    const auto renamed = controller.renameSelection("M_Stone");
    check(renamed && renamed.id == originalId && renamed.path.string() == "/Game/Materials/M_Stone.acemat",
          "rename_preserves_guid_and_extension");
    check(std::filesystem::is_regular_file(content / "Materials" / "M_Stone.acemat") &&
          model.selectedItems().front().assetId == originalId, "renamed_asset_is_published_and_reselected");

    const auto duplicate = controller.duplicateSelection("M_Stone_Copy");
    check(duplicate && duplicate.id.isValid() && duplicate.id != originalId,
          "duplicate_gets_distinct_stable_guid");
    check(std::filesystem::file_size(content / "Materials" / "M_Stone_Copy.acemat") == 13 &&
          model.selectedItems().front().assetId == duplicate.id, "duplicate_is_byte_exact_and_selected");
    check(controller.deleteSelection() && !std::filesystem::exists(content / "Materials" / "M_Stone_Copy.acemat"),
          "delete_selected_asset_is_transactional");
    check(model.selectionCount() == 0, "deleted_selection_is_pruned");
    check(controller.undo() && std::filesystem::is_regular_file(content / "Materials" / "M_Stone_Copy.acemat"),
          "controller_undo_restores_deleted_asset");
    check(controller.redo() && !std::filesystem::exists(content / "Materials" / "M_Stone_Copy.acemat"),
          "controller_redo_reapplies_delete");

    model.clearSelection();
    check(!controller.renameSelection("Nothing") && !controller.duplicateSelection("Nothing") &&
          !controller.deleteSelection(), "actions_require_one_selection");
    check(model.navigateTo(*AssetPath::parse("/Game")), "return_to_root");
    bool selectedNewFolder = false;
    for (std::size_t index = 0; index < model.visibleItems().size(); ++index)
    {
        if (model.visibleItems()[index].path.string() == "/Game/NewFolder")
            selectedNewFolder = model.selectVisible(index, SelectionMode::Replace);
    }
    check(selectedNewFolder && !controller.duplicateSelection("Copy"), "folders_cannot_use_asset_duplicate");
    const auto renamedFolder = controller.renameSelection("RenamedFolder");
    check(renamedFolder && std::filesystem::is_directory(content / "RenamedFolder") &&
          !std::filesystem::exists(content / "NewFolder"), "folder_rename_uses_same_operation_boundary");
    check(controller.deleteSelection() && !std::filesystem::exists(content / "RenamedFolder"),
          "empty_selected_folder_can_be_deleted");

    ContentBrowserController unavailable;
    check(!unavailable.createFolder("Nope") && !unavailable.undo() && !unavailable.redo(),
          "uninitialized_controller_fails_without_mutation");
    check(registry.snapshot().assets.size() == 1 && registry.findById(originalId) != nullptr,
          "unrelated_original_asset_remains_registered");

    std::filesystem::remove_all(base, ec);
    std::cout << "SUMMARY|checks=" << checks << "|failures=" << failures << '\n';
    return failures == 0 ? 0 : 1;
}
