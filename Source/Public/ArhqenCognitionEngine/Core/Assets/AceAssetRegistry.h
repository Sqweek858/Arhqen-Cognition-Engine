#pragma once

#include "ArhqenCognitionEngine/Core/Assets/AceAssetPath.h"
#include "ArhqenCognitionEngine/Core/Identity/AceGuid.h"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace am::core::assets
{
    enum class AssetType : std::uint8_t
    {
        Material,
        MaterialFunction,
        StaticMesh,
        MeshSource,
        Texture,
        Scene,
        Landscape,
        PhysicsMaterial
    };

    struct AssetRecord
    {
        Guid id{};
        AssetPath path{};
        AssetType type = AssetType::Material;
        std::uint64_t fileSize = 0;
        std::int64_t lastWriteStamp = 0;
        std::string extension;
    };

    struct AssetFolderRecord
    {
        AssetPath path{};
        std::uint32_t directAssetCount = 0;
        std::uint32_t descendantAssetCount = 0;
    };

    struct AssetRegistrySnapshot
    {
        std::uint64_t generation = 0;
        std::vector<AssetRecord> assets;
        std::vector<AssetFolderRecord> folders;
        std::uint64_t skippedUnsupportedFiles = 0;
        std::uint64_t skippedUnsafeEntries = 0;
    };

    struct AssetRegistryDelta
    {
        struct Move
        {
            Guid id{};
            AssetPath oldPath{};
            AssetPath newPath{};
        };

        std::uint64_t generation = 0;
        bool fullRescan = false;
        std::vector<Guid> added;
        std::vector<Guid> modified;
        std::vector<Guid> removed;
        std::vector<Move> moved;

        [[nodiscard]] bool empty() const noexcept
        {
            return added.empty() && modified.empty() && removed.empty() && moved.empty();
        }
    };

    class AssetRegistry final
    {
    public:
        bool initialize(std::filesystem::path contentRoot,
                        std::filesystem::path stateFile,
                        std::string* error = nullptr);
        bool rescan(std::string* error = nullptr, bool fullRescan = false);
        bool remapPath(const AssetPath& oldPath, const AssetPath& newPath, std::string* error = nullptr);

        [[nodiscard]] bool initialized() const noexcept { return initialized_; }
        [[nodiscard]] const std::filesystem::path& contentRoot() const noexcept { return contentRoot_; }
        [[nodiscard]] const std::filesystem::path& stateFile() const noexcept { return stateFile_; }
        [[nodiscard]] const AssetRegistrySnapshot& snapshot() const noexcept { return snapshot_; }
        [[nodiscard]] const AssetRegistryDelta& lastDelta() const noexcept { return lastDelta_; }
        [[nodiscard]] const std::string& lastWarning() const noexcept { return lastWarning_; }
        [[nodiscard]] const AssetRecord* findByPath(std::string_view virtualPath) const;
        [[nodiscard]] const AssetRecord* findById(const Guid& id) const;

        [[nodiscard]] static std::optional<AssetType> typeFromExtension(std::string_view extension) noexcept;
        [[nodiscard]] static std::string_view typeName(AssetType type) noexcept;

    private:
        bool loadStableIds(std::string* error);
        bool saveStableIds(std::string* error) const;
        bool validateRoots(std::string* error);

        std::filesystem::path contentRoot_;
        std::filesystem::path stateFile_;
        AssetRegistrySnapshot snapshot_{};
        AssetRegistryDelta lastDelta_{};
        std::unordered_map<std::string, Guid> stableIdsByPathKey_;
        std::unordered_map<std::string, std::size_t> assetIndexByPathKey_;
        std::unordered_map<Guid, std::size_t, GuidHash> assetIndexById_;
        std::string lastWarning_;
        bool initialized_ = false;
    };
}
