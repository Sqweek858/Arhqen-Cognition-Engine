#include "ArhqenCognitionEngine/Editor/ContentBrowser/AceContentBrowserModel.h"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>

namespace
{
    using am::core::Guid;
    using am::core::assets::AssetFolderRecord;
    using am::core::assets::AssetPath;
    using am::core::assets::AssetRecord;
    using am::core::assets::AssetRegistrySnapshot;
    using am::core::assets::AssetType;

    int checks = 0;
    int failures = 0;

    void check(bool condition, std::string_view name)
    {
        ++checks;
        std::cout << (condition ? "PASS|" : "FAIL|") << name << '\n';
        if (!condition) ++failures;
    }

    AssetPath path(std::string_view value)
    {
        return *AssetPath::parse(value);
    }

    Guid guid(std::uint32_t value)
    {
        Guid::Bytes bytes{};
        bytes[0] = static_cast<std::uint8_t>(value & 0xffu);
        bytes[1] = static_cast<std::uint8_t>((value >> 8u) & 0xffu);
        bytes[2] = static_cast<std::uint8_t>((value >> 16u) & 0xffu);
        bytes[3] = static_cast<std::uint8_t>((value >> 24u) & 0xffu);
        bytes[15] = 0xA5u;
        return Guid::fromBytes(bytes);
    }

    AssetRecord asset(std::uint32_t id, std::string_view value, AssetType type, std::uint64_t size = 1)
    {
        AssetRecord result;
        result.id = guid(id);
        result.path = path(value);
        result.type = type;
        result.fileSize = size;
        return result;
    }

    AssetFolderRecord folder(std::string_view value, std::uint32_t direct, std::uint32_t descendants)
    {
        return AssetFolderRecord{path(value), direct, descendants};
    }

    AssetRegistrySnapshot fixture()
    {
        AssetRegistrySnapshot snapshot;
        snapshot.generation = 1;
        snapshot.folders = {
            folder("/Game", 1, 6),
            folder("/Game/Materials", 3, 4),
            folder("/Game/Materials/Layered", 1, 1),
            folder("/Game/Meshes", 1, 1),
            folder("/Game/Empty", 0, 0)};
        snapshot.assets = {
            asset(1, "/Game/Materials/M_Rock.acemat", AssetType::Material, 12),
            asset(2, "/Game/Materials/T_Rock.png", AssetType::Texture, 24),
            asset(3, "/Game/Materials/Zebra.acemat", AssetType::Material, 4),
            asset(4, "/Game/Materials/Layered/M_Moss.acemat", AssetType::Material, 8),
            asset(5, "/Game/Meshes/SM_Rock.fbx", AssetType::MeshSource, 64),
            asset(6, "/Game/Main.acescene", AssetType::Scene, 100)};
        return snapshot;
    }
}

int main()
{
    using namespace am::editor::content_browser;

    ContentBrowserModel model;
    auto snapshot = fixture();
    model.synchronize(snapshot);
    check(model.synchronizedGeneration() == 1, "generation_is_synchronized");
    check(model.currentFolder().string() == "/Game", "initial_folder_is_game_root");
    check(model.folderTree().size() == 5 && model.folderTree().front().depth == 0,
          "folder_tree_contains_root_with_zero_depth");
    check(model.visibleItems().size() == 4 && model.unfilteredItemCount() == 4,
          "root_lists_only_direct_children");
    check(model.visibleItems()[0].kind == ItemKind::Folder && model.visibleItems()[1].kind == ItemKind::Folder &&
          model.visibleItems()[2].kind == ItemKind::Folder && model.visibleItems()[3].kind == ItemKind::Asset,
          "folders_sort_before_assets");
    check(model.visibleItems()[3].displayName == "Main", "asset_display_name_hides_extension");
    check(!model.navigateTo(path("/Game/Missing")), "invalid_navigation_is_rejected");

    check(model.navigateTo(path("/Game/Materials")), "navigate_to_folder");
    check(model.breadcrumbs().size() == 2 && model.breadcrumbs()[0].label == "Content" &&
          model.breadcrumbs()[1].label == "Materials", "breadcrumbs_are_semantic_and_complete");
    check(model.visibleItems().size() == 4 && model.visibleItems()[0].kind == ItemKind::Folder,
          "folder_view_is_direct_and_sorted");
    check(model.canBack() && !model.canForward(), "history_state_after_navigation");
    check(model.back() && model.currentFolder().isRoot(), "history_back_navigates");
    check(model.forward() && model.currentFolder().string() == "/Game/Materials", "history_forward_navigates");
    check(model.up() && model.currentFolder().isRoot(), "up_navigates_to_parent");
    check(model.back() && model.currentFolder().string() == "/Game/Materials", "up_adds_history_entry");
    check(model.navigateTo(path("/Game/Meshes")) && !model.canForward(), "new_navigation_truncates_forward_history");

    check(model.navigateTo(path("/Game/Materials")), "return_to_materials");
    model.setSearchText("rock");
    check(model.visibleItems().size() == 2 && model.visibleItems()[0].displayName == "M_Rock" &&
          model.visibleItems()[1].displayName == "T_Rock", "search_is_case_insensitive_and_filters_names");
    model.setTypeVisible(AssetType::Texture, false);
    check(model.visibleItems().size() == 1 && model.visibleItems()[0].assetType == AssetType::Material,
          "asset_type_filter_combines_with_search");
    model.showAllTypes();
    check(model.visibleItems().size() == 2, "show_all_types_restores_results");
    model.setSearchText("");
    check(model.visibleItems().size() == 4, "empty_search_restores_unfiltered_view");
    model.setViewMode(ViewMode::List);
    check(model.viewMode() == ViewMode::List, "view_mode_is_retained");

    check(model.selectVisible(0, SelectionMode::Replace) && model.selectionCount() == 1,
          "replace_selection_selects_one_item");
    check(model.selectVisible(2, SelectionMode::Add) && model.selectionCount() == 2,
          "add_selection_preserves_existing_item");
    check(model.selectVisible(2, SelectionMode::Toggle) && model.selectionCount() == 1,
          "toggle_removes_selected_item");
    check(model.selectVisible(0, SelectionMode::Replace) && model.selectVisible(2, SelectionMode::Range) &&
          model.selectionCount() == 3, "range_selection_is_inclusive");
    check(model.selectedItems().size() == 3 && model.selectedItems().front().kind == ItemKind::Folder,
          "selected_items_are_stably_sorted");
    model.clearSelection();
    check(model.selectionCount() == 0, "clear_selection_resets_all_items");

    check(model.selectAsset(guid(1)) && model.beginRenameSelected(), "f2_model_begins_single_item_rename");
    check(model.renameActive() && model.renameItem() && model.renameItem()->assetId == guid(1),
          "rename_target_uses_stable_asset_identity");
    model.cancelRename();
    check(!model.renameActive() && model.renameItem() == nullptr, "rename_can_be_cancelled");
    check(model.selectAsset(guid(1)) && model.selectAsset(guid(2), SelectionMode::Add) &&
          !model.beginRenameSelected(), "multi_selection_cannot_begin_inline_rename");

    model.clearSelection();
    check(model.selectAsset(guid(1)), "stable_asset_selected_before_move");
    snapshot.generation = 2;
    snapshot.assets[0].path = path("/Game/Materials/M_Stone.acemat");
    model.synchronize(snapshot);
    const auto selectedAfterMove = model.selectedItems();
    check(selectedAfterMove.size() == 1 && selectedAfterMove.front().path.string() ==
          "/Game/Materials/M_Stone.acemat", "selection_survives_asset_move_by_guid");
    check(model.canBack(), "valid_navigation_history_survives_generation_change");

    snapshot.generation = 3;
    snapshot.assets.erase(snapshot.assets.begin());
    model.synchronize(snapshot);
    check(model.selectionCount() == 0, "deleted_asset_is_pruned_from_selection");

    check(model.navigateTo(path("/Game/Materials/Layered")), "navigate_to_deep_folder");
    snapshot.generation = 4;
    snapshot.folders.erase(snapshot.folders.begin() + 2);
    snapshot.assets.erase(snapshot.assets.begin() + 2);
    model.synchronize(snapshot);
    check(model.currentFolder().string() == "/Game/Materials", "removed_current_folder_repairs_to_existing_parent");
    check(model.breadcrumbs().back().label == "Materials", "breadcrumbs_repair_with_current_folder");

    auto ignored = snapshot;
    ignored.assets.push_back(asset(90, "/Game/Materials/Ignored.acemat", AssetType::Material));
    model.synchronize(ignored);
    bool ignoredFound = false;
    for (const auto& item : model.visibleItems()) ignoredFound = ignoredFound || item.displayName == "Ignored";
    check(!ignoredFound, "same_generation_snapshot_is_not_rebuilt");

    AssetRegistrySnapshot large;
    large.generation = 100;
    large.folders.push_back(folder("/Game", 100000, 100000));
    large.assets.reserve(100000);
    for (std::uint32_t index = 0; index < 100000; ++index)
    {
        large.assets.push_back(asset(index + 1000, "/Game/A_" + std::to_string(index) + ".acemat",
                                     AssetType::Material, index));
    }
    const auto start = std::chrono::steady_clock::now();
    model.synchronize(large);
    const auto elapsed = std::chrono::steady_clock::now() - start;
    check(model.currentFolder().isRoot() && model.visibleItems().size() == 100000,
          "large_registry_repairs_folder_and_lists_all_direct_assets");
    check(elapsed < std::chrono::seconds(5), "hundred_thousand_item_sync_is_bounded");
    model.setSearchText("A_99999");
    check(model.visibleItems().size() == 1 && model.visibleItems().front().displayName == "A_99999",
          "large_registry_search_is_exactly_filtered");

    std::cout << "SUMMARY|checks=" << checks << "|failures=" << failures << '\n';
    return failures == 0 ? 0 : 1;
}
