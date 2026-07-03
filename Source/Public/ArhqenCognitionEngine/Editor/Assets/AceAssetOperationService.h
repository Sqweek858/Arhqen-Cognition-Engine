#pragma once

#include "ArhqenCognitionEngine/Core/Assets/AceAssetReferences.h"
#include "ArhqenCognitionEngine/Core/Assets/AceAssetRegistry.h"
#include "ArhqenCognitionEngine/Editor/Transactions/AceTransaction.h"

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace am::editor::assets
{
    struct AssetOperationResult
    {
        bool success = false;
        am::core::assets::AssetPath path{};
        am::core::Guid id{};
        std::string error;
        std::vector<am::core::Guid> blockingReferencers;

        explicit operator bool() const noexcept { return success; }
    };

    class AssetOperationService final
    {
    public:
        struct ErrorState;

        AssetOperationService();
        ~AssetOperationService();

        AssetOperationService(const AssetOperationService&) = delete;
        AssetOperationService& operator=(const AssetOperationService&) = delete;

        bool initialize(am::core::assets::AssetRegistry& registry,
                        am::core::assets::AssetReferenceIndex& references,
                        am::editor::transactions::TransactionManager& transactions,
                        std::filesystem::path undoStorageRoot,
                        std::string* error = nullptr);

        AssetOperationResult createFolder(const am::core::assets::AssetPath& parent, std::string_view name);
        AssetOperationResult rename(const am::core::assets::AssetPath& source, std::string_view newName);
        AssetOperationResult move(const am::core::assets::AssetPath& source,
                                  const am::core::assets::AssetPath& destinationFolder);
        AssetOperationResult duplicateAsset(const am::core::assets::AssetPath& source, std::string_view newName);
        AssetOperationResult deleteAsset(const am::core::assets::AssetPath& source, bool force = false);
        AssetOperationResult deleteEmptyFolder(const am::core::assets::AssetPath& source);

        bool undo(std::string* error = nullptr);
        bool redo(std::string* error = nullptr);
        [[nodiscard]] bool initialized() const noexcept { return registry_ != nullptr; }
        [[nodiscard]] const std::filesystem::path& undoStorageRoot() const noexcept { return undoStorageRoot_; }

    private:
        bool folderExists(const am::core::assets::AssetPath& path) const;
        bool validateName(std::string_view name, std::string* error) const;
        AssetOperationResult moveInternal(const am::core::assets::AssetPath& source,
                                          const am::core::assets::AssetPath& destination,
                                          std::string transactionName);

        am::core::assets::AssetRegistry* registry_ = nullptr;
        am::core::assets::AssetReferenceIndex* references_ = nullptr;
        am::editor::transactions::TransactionManager* transactions_ = nullptr;
        std::filesystem::path undoStorageRoot_;
        std::shared_ptr<ErrorState> errorState_;
    };
}
