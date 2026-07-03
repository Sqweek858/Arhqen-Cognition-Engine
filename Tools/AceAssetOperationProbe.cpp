#include "ArhqenCognitionEngine/Editor/Assets/AceAssetOperationService.h"

#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>

namespace
{
    int failures = 0;
    int checks = 0;
    void check(bool value, std::string_view name)
    {
        ++checks;
        std::cout << (value ? "PASS|" : "FAIL|") << name << '\n';
        if (!value) ++failures;
    }
    void write(const std::filesystem::path& path, std::string_view bytes)
    {
        std::filesystem::create_directories(path.parent_path());
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    }
    bool folderExists(const am::core::assets::AssetRegistry& registry, std::string_view path)
    {
        const auto parsed = am::core::assets::AssetPath::parse(path);
        if (!parsed) return false;
        const std::string key = parsed->comparisonKey();
        for (const auto& folder : registry.snapshot().folders)
            if (folder.path.comparisonKey() == key) return true;
        return false;
    }
}

int main()
{
    using am::core::Guid;
    using am::core::assets::AssetPath;
    using am::core::assets::AssetReferenceIndex;
    using am::core::assets::AssetRegistry;
    using am::editor::assets::AssetOperationService;
    using am::editor::transactions::TransactionManager;

    const auto base = std::filesystem::current_path() / "Build" / "ACE-ASSET-OPERATIONS" / "runtime";
    const auto content = base / "Content";
    const auto state = base / "State" / "registry.acebin";
    const auto undoRoot = base / "Undo";
    std::error_code ec;
    std::filesystem::remove_all(base, ec);
    std::filesystem::create_directories(content / "Source");
    std::filesystem::create_directories(content / "Dest");
    write(content / "Source" / "M_Rock.acemat", "rock-material-v1");
    write(undoRoot / "stale.deleted", "stale");
    write(undoRoot / "keep.txt", "keep");

    AssetRegistry registry;
    AssetReferenceIndex references;
    TransactionManager transactions(128, 16ull * 1024ull * 1024ull);
    AssetOperationService operations;
    std::string error;
    check(registry.initialize(content, state, &error), "registry_initialized");
    check(operations.initialize(registry, references, transactions, undoRoot, &error), "operation_service_initialized");
    check(!std::filesystem::exists(undoRoot / "stale.deleted") && std::filesystem::exists(undoRoot / "keep.txt"),
        "startup_cleans_only_owned_stale_undo_files");
    AssetOperationService unsafeOperations;
    check(!unsafeOperations.initialize(registry, references, transactions, content / "Undo", &error),
        "undo_storage_inside_content_rejected");
    check(!unsafeOperations.initialize(registry, references, transactions, base, &error),
        "undo_storage_ancestor_of_content_rejected");

    const auto game = AssetPath::parse("/Game");
    const auto sourceFolder = AssetPath::parse("/Game/Source");
    const auto destFolder = AssetPath::parse("/Game/Dest");
    const auto sourceAsset = AssetPath::parse("/Game/Source/M_Rock.acemat");
    check(game && sourceFolder && destFolder && sourceAsset, "test_paths_parse");

    const auto created = operations.createFolder(*game, "Created");
    check(created && folderExists(registry, "/Game/Created") && std::filesystem::is_directory(content / "Created"),
        "create_folder_updates_disk_and_registry");
    check(operations.undo(&error) && !folderExists(registry, "/Game/Created"), "create_folder_undo");
    check(operations.redo(&error) && folderExists(registry, "/Game/Created"), "create_folder_redo");
    const auto createdPath = AssetPath::parse("/Game/Created");
    const auto deletedFolder = operations.deleteEmptyFolder(*createdPath);
    check(deletedFolder && !folderExists(registry, "/Game/Created"), "delete_empty_folder");
    check(operations.undo(&error) && folderExists(registry, "/Game/Created"), "delete_folder_undo");
    check(operations.redo(&error) && !folderExists(registry, "/Game/Created"), "delete_folder_redo");

    const auto* originalRecord = registry.findByPath(sourceAsset->string());
    const Guid originalId = originalRecord ? originalRecord->id : Guid{};
    const auto renamed = operations.rename(*sourceAsset, "M_Stone");
    check(renamed && renamed.id == originalId && registry.findByPath("/Game/Source/M_Stone.acemat"),
        "rename_preserves_guid");
    check(!operations.rename(renamed.path, "Bad.Name") && std::filesystem::exists(content / "Source" / "M_Stone.acemat"),
        "rename_rejects_dotted_display_name");
    check(operations.undo(&error) && registry.findByPath(sourceAsset->string()) &&
        registry.findByPath(sourceAsset->string())->id == originalId,
        "rename_undo_restores_path_and_guid");
    check(operations.redo(&error) && registry.findByPath("/Game/Source/M_Stone.acemat"), "rename_redo");

    const auto stone = AssetPath::parse("/Game/Source/M_Stone.acemat");
    const auto moved = operations.move(*stone, *destFolder);
    check(moved && moved.id == originalId && registry.findByPath("/Game/Dest/M_Stone.acemat"),
        "move_preserves_guid");
    check(operations.undo(&error) && registry.findByPath(stone->string()), "move_undo");
    check(operations.redo(&error) && registry.findByPath(moved.path.string()), "move_redo");

    const auto duplicate = operations.duplicateAsset(moved.path, "M_Stone_Copy");
    check(duplicate && duplicate.id.isValid() && duplicate.id != originalId &&
        std::filesystem::file_size(duplicate.path.toFilesystemPath(content)) == std::filesystem::file_size(moved.path.toFilesystemPath(content)),
        "duplicate_creates_independent_identity_and_exact_bytes");
    const Guid duplicateId = duplicate.id;
    check(operations.undo(&error) && !registry.findByPath(duplicate.path.string()), "duplicate_undo_removes_asset");
    check(operations.redo(&error) && registry.findByPath(duplicate.path.string()) &&
        registry.findByPath(duplicate.path.string())->id == duplicateId,
        "duplicate_redo_restores_same_guid");

    const Guid blocker = Guid::create();
    const std::array incoming{originalId};
    check(references.setReferences(blocker, incoming, &error), "blocking_reference_registered");
    const auto blockedDelete = operations.deleteAsset(moved.path);
    check(!blockedDelete && blockedDelete.blockingReferencers.size() == 1 &&
        std::filesystem::exists(moved.path.toFilesystemPath(content)),
        "referenced_delete_is_refused_before_disk_mutation");
    references.removeReferencer(blocker);
    const Guid outgoingTarget = Guid::create();
    const std::array outgoing{outgoingTarget};
    check(references.setReferences(originalId, outgoing, &error), "outgoing_reference_registered");
    const auto deleted = operations.deleteAsset(moved.path);
    check(deleted && !registry.findByPath(moved.path.string()) && !references.hasReferencers(outgoingTarget),
        "delete_moves_file_to_undo_storage_and_removes_outgoing_edges");
    check(operations.undo(&error) && registry.findByPath(moved.path.string()) &&
        registry.findByPath(moved.path.string())->id == originalId && references.hasReferencers(outgoingTarget),
        "delete_undo_restores_file_guid_and_references");
    check(operations.redo(&error) && !registry.findByPath(moved.path.string()), "delete_redo");
    check(operations.undo(&error) && registry.findByPath(moved.path.string()), "delete_second_undo");

    write(content / "NonEmpty" / "child.txt", "child");
    check(registry.rescan(&error), "registry_sees_nonempty_folder");
    const auto nonEmpty = AssetPath::parse("/Game/NonEmpty");
    check(nonEmpty && !operations.deleteEmptyFolder(*nonEmpty) && std::filesystem::exists(content / "NonEmpty" / "child.txt"),
        "nonempty_folder_delete_is_refused");

    const auto folderMove = operations.move(*sourceFolder, *destFolder);
    check(folderMove && folderExists(registry, "/Game/Dest/Source"), "folder_subtree_move");
    check(operations.undo(&error) && folderExists(registry, "/Game/Source"), "folder_subtree_move_undo");

    check(transactions.canUndo() && transactions.nextUndo().operationCount == 1,
        "asset_operations_use_central_transaction_history");
    check(std::filesystem::is_directory(operations.undoStorageRoot()), "undo_storage_remains_external_and_available");

    std::cout << (failures == 0 ? "PASS|" : "FAIL|")
        << "ace_asset_operation_probe|checks=" << checks << "|failures=" << failures << '\n';
    return failures == 0 ? 0 : 1;
}
