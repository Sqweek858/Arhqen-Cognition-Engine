#include "ArhqenCognitionEngine/Core/Assets/AceAssetRegistry.h"
#include "ArhqenCognitionEngine/Core/Assets/AceAssetDirectoryWatcher.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>
#include <thread>

namespace
{
    int failures = 0;
    int checks = 0;

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
    using am::core::assets::AssetRegistry;
    using am::core::assets::AssetType;

    const auto base = std::filesystem::current_path() / "Build" / "ACE-ASSET-REGISTRY" / "runtime";
    const auto content = base / "Content";
    const auto state = base / "State" / "registry.acebin";
    std::error_code ec;
    std::filesystem::remove_all(base, ec);
    std::filesystem::create_directories(content / "Materials");
    std::filesystem::create_directories(content / "Meshes");
    std::filesystem::create_directories(content / "Empty");
    std::filesystem::create_directories(content / ".ace");

    write(content / "Materials" / "M_Rock.acemat", "material");
    write(content / "Meshes" / "SM_Rock.fbx", "fbx-data");
    write(content / "T_Albedo.PNG", "png-data");
    write(content / "Level.acescene", "scene-data");
    write(content / "notes.txt", "unsupported");
    write(content / ".ace" / "private.acemat", "hidden");
    write(base / "Source" / "Private.cpp", "outside");

    AssetRegistry registry;
    std::string error;
    check(registry.initialize(content, state, &error), "initialize_and_create_mount");
    check(error.empty() && registry.initialized(), "initialized_state_is_clean");
    check(std::filesystem::is_directory(content) && std::filesystem::is_regular_file(state),
        "content_and_external_state_exist");
    check(registry.snapshot().assets.size() == 4, "only_supported_assets_are_indexed");
    check(registry.lastDelta().fullRescan && registry.lastDelta().added.size() == 4,
        "initial_scan_publishes_full_added_delta");
    check(registry.snapshot().folders.size() == 4, "root_and_visible_folders_are_indexed");
    check(registry.snapshot().skippedUnsupportedFiles == 1, "unsupported_file_is_counted");
    check(registry.snapshot().skippedUnsafeEntries >= 1, "internal_folder_is_skipped");
    check(registry.findByPath("/Game/materials/m_rock.ACEMAT") != nullptr,
        "path_lookup_is_case_stable");
    check(registry.findByPath("/Game/notes.txt") == nullptr &&
        registry.findByPath("/Game/.ace/private.acemat") == nullptr,
        "unsupported_and_internal_files_are_invisible");
    const auto* material = registry.findByPath("/Game/Materials/M_Rock.acemat");
    check(material && material->type == AssetType::Material && material->fileSize == 8,
        "material_metadata_is_real");
    check(material && registry.findById(material->id) == material, "stable_id_lookup_round_trip");
    const auto materialId = material ? material->id : am::core::Guid{};

    const auto& folders = registry.snapshot().folders;
    check(!folders.empty() && folders.front().path.string() == "/Game", "folders_are_sorted_from_root");
    check(!folders.empty() && folders.front().descendantAssetCount == 4, "root_descendant_count_is_exact");

    AssetRegistry reload;
    check(reload.initialize(content, state, &error), "reload_persisted_registry");
    const auto* reloadedMaterial = reload.findByPath("/Game/Materials/M_Rock.acemat");
    check(reloadedMaterial && reloadedMaterial->id == materialId, "asset_id_is_stable_across_restart");

    const auto generation = reload.snapshot().generation;
    write(content / "Materials" / "M_Water.acematerial", "water");
    check(reload.rescan(&error), "incremental_rescan_succeeds");
    check(reload.snapshot().generation == generation + 1 && reload.snapshot().assets.size() == 5,
        "rescan_advances_generation_and_discovers_asset");
    check(!reload.lastDelta().fullRescan && reload.lastDelta().added.size() == 1 &&
        reload.lastDelta().modified.empty() && reload.lastDelta().removed.empty(),
        "rescan_publishes_incremental_add_delta");
    check(reload.findByPath("/Game/Materials/M_Rock.acemat") &&
        reload.findByPath("/Game/Materials/M_Rock.acemat")->id == materialId,
        "existing_id_survives_rescan");

    write(content / "Materials" / "M_Rock.acemat", "material-modified");
    check(reload.rescan(&error) && reload.lastDelta().modified.size() == 1 &&
        reload.lastDelta().modified.front() == materialId,
        "metadata_change_publishes_modified_delta");
    const auto* water = reload.findByPath("/Game/Materials/M_Water.acematerial");
    const auto waterId = water ? water->id : am::core::Guid{};
    std::filesystem::remove(content / "Materials" / "M_Water.acematerial", ec);
    check(reload.rescan(&error) && reload.lastDelta().removed.size() == 1 &&
        reload.lastDelta().removed.front() == waterId,
        "delete_publishes_removed_delta");

    AssetRegistry invalidStateLocation;
    check(!invalidStateLocation.initialize(content, content / ".ace" / "registry.acebin", &error) && !error.empty(),
        "state_file_inside_content_is_rejected");

    write(state, "corrupt-state");
    AssetRegistry recovered;
    check(recovered.initialize(content, state, &error), "corrupt_state_recovers_without_losing_content");
    check(!recovered.lastWarning().empty() && recovered.snapshot().assets.size() == 4,
        "corrupt_state_emits_warning_and_rebuilds");

    check(AssetRegistry::typeFromExtension(".FBX") == AssetType::MeshSource &&
        AssetRegistry::typeFromExtension(".cpp") == std::nullopt,
        "extension_classification_is_strict_and_case_insensitive");
    check(AssetRegistry::typeName(AssetType::PhysicsMaterial) == "Physics Material",
        "asset_type_names_are_stable");

    AssetRegistry createsMissingRoot;
    const auto missingContent = base / "CreatedAtRuntime" / "Content";
    check(createsMissingRoot.initialize(missingContent, base / "State2" / "registry.acebin", &error) &&
        std::filesystem::is_directory(missingContent) && createsMissingRoot.snapshot().folders.size() == 1,
        "missing_empty_content_root_is_created_at_runtime");

    const auto watchRoot = base / "WatchContent";
    std::filesystem::create_directories(watchRoot);
    am::core::assets::AssetDirectoryWatcher watcher;
    check(watcher.start(watchRoot, &error) && watcher.running(), "directory_watcher_starts_on_content_root");
    write(watchRoot / "Live.acemat", "live");
    std::vector<am::core::assets::AssetFileChange> liveChanges;
    for (int attempt = 0; attempt < 100 && liveChanges.empty(); ++attempt)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        liveChanges = watcher.drainChanges();
    }
    bool sawLivePath = false;
    for (const auto& change : liveChanges)
        sawLivePath = sawLivePath || change.relativePath.generic_wstring() == L"Live.acemat";
    check(sawLivePath, "directory_watcher_reports_real_file_change");

    std::filesystem::rename(watchRoot / "Live.acemat", watchRoot / "Renamed.acemat", ec);
    std::vector<am::core::assets::AssetFileChange> renameChanges;
    for (int attempt = 0; attempt < 100 && renameChanges.size() < 2; ++attempt)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        auto batch = watcher.drainChanges();
        renameChanges.insert(renameChanges.end(), batch.begin(), batch.end());
    }
    bool sawOld = false;
    bool sawNew = false;
    for (const auto& change : renameChanges)
    {
        sawOld = sawOld || change.action == am::core::assets::AssetFileChangeAction::RenamedOld;
        sawNew = sawNew || change.action == am::core::assets::AssetFileChangeAction::RenamedNew;
    }
    check(sawOld && sawNew, "directory_watcher_preserves_rename_pair");
    const auto watchStats = watcher.stats();
    check(watchStats.nativeBatches >= 1 && watchStats.queuedEvents >= liveChanges.size(),
        "directory_watcher_stats_are_observable");
    watcher.stop();
    check(!watcher.running(), "directory_watcher_stops_cleanly");
    check(!watcher.start(base / "MissingWatchRoot", &error) && !error.empty(),
        "directory_watcher_rejects_missing_root");

    std::cout << (failures == 0 ? "PASS|" : "FAIL|")
        << "ace_asset_registry_probe|checks=" << checks << "|failures=" << failures << '\n';
    return failures == 0 ? 0 : 1;
}
