#pragma once

#include "ArhqenCognitionEngine/Core/Assets/AceAssetRegistry.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace am::editor::content_browser
{
    enum class ItemKind : std::uint8_t
    {
        Folder,
        Asset
    };

    enum class ViewMode : std::uint8_t
    {
        Tiles,
        List
    };

    enum class SelectionMode : std::uint8_t
    {
        Replace,
        Add,
        Toggle,
        Range
    };

    struct Item final
    {
        ItemKind kind = ItemKind::Folder;
        am::core::Guid assetId{};
        am::core::assets::AssetPath path{};
        am::core::assets::AssetType assetType = am::core::assets::AssetType::Material;
        std::string displayName;
        std::uint64_t fileSize = 0;
        std::uint32_t descendantAssetCount = 0;
    };

    struct Breadcrumb final
    {
        std::string label;
        am::core::assets::AssetPath path{};
    };

    struct FolderTreeEntry final
    {
        am::core::assets::AssetPath path{};
        std::size_t depth = 0;
        std::uint32_t directAssetCount = 0;
        std::uint32_t descendantAssetCount = 0;
    };

    class ContentBrowserModel final
    {
    public:
        ContentBrowserModel();

        void synchronize(const am::core::assets::AssetRegistrySnapshot& snapshot);
        [[nodiscard]] std::uint64_t synchronizedGeneration() const noexcept { return synchronizedGeneration_; }

        [[nodiscard]] const am::core::assets::AssetPath& currentFolder() const noexcept { return currentFolder_; }
        [[nodiscard]] bool navigateTo(const am::core::assets::AssetPath& folder, bool addToHistory = true);
        [[nodiscard]] bool back();
        [[nodiscard]] bool forward();
        [[nodiscard]] bool up();
        [[nodiscard]] bool canBack() const noexcept { return historyCursor_ > 0; }
        [[nodiscard]] bool canForward() const noexcept { return historyCursor_ + 1 < history_.size(); }
        [[nodiscard]] bool canGoUp() const noexcept { return !currentFolder_.isRoot(); }

        [[nodiscard]] const std::vector<Breadcrumb>& breadcrumbs() const noexcept { return breadcrumbs_; }
        [[nodiscard]] const std::vector<FolderTreeEntry>& folderTree() const noexcept { return folderTree_; }
        [[nodiscard]] const std::vector<Item>& visibleItems() const noexcept { return visibleItems_; }
        [[nodiscard]] std::size_t unfilteredItemCount() const noexcept { return unfilteredItemCount_; }

        void setSearchText(std::string text);
        [[nodiscard]] const std::string& searchText() const noexcept { return searchText_; }
        void setTypeVisible(am::core::assets::AssetType type, bool visible);
        [[nodiscard]] bool typeVisible(am::core::assets::AssetType type) const noexcept;
        void showAllTypes();

        void setViewMode(ViewMode mode) noexcept { viewMode_ = mode; }
        [[nodiscard]] ViewMode viewMode() const noexcept { return viewMode_; }

        [[nodiscard]] bool selectVisible(std::size_t index, SelectionMode mode);
        [[nodiscard]] bool selectAsset(const am::core::Guid& id, SelectionMode mode = SelectionMode::Replace);
        [[nodiscard]] bool selectFolder(const am::core::assets::AssetPath& path,
                                        SelectionMode mode = SelectionMode::Replace);
        void clearSelection() noexcept;
        [[nodiscard]] bool isSelected(const Item& item) const;
        [[nodiscard]] std::vector<Item> selectedItems() const;
        [[nodiscard]] std::size_t selectionCount() const noexcept { return selectionKeys_.size(); }

        [[nodiscard]] bool beginRenameSelected();
        [[nodiscard]] bool beginRename(const Item& item);
        void cancelRename() noexcept { renameKey_.reset(); }
        [[nodiscard]] bool renameActive() const noexcept { return renameKey_.has_value(); }
        [[nodiscard]] const Item* renameItem() const;

    private:
        static constexpr std::size_t assetTypeCount_ = 8;

        [[nodiscard]] static std::size_t typeIndex(am::core::assets::AssetType type) noexcept;
        [[nodiscard]] static std::string itemKey(const Item& item);
        [[nodiscard]] static std::string folderKey(const am::core::assets::AssetPath& path);
        [[nodiscard]] static std::string assetKey(const am::core::Guid& id);
        [[nodiscard]] bool folderExists(const am::core::assets::AssetPath& path) const;
        [[nodiscard]] const Item* findItemByKey(std::string_view key) const;
        [[nodiscard]] std::optional<std::size_t> visibleIndexForKey(std::string_view key) const;
        [[nodiscard]] bool selectKey(std::string key, SelectionMode mode,
                                     std::optional<std::size_t> visibleIndex);
        void rebuildFolderTree(const am::core::assets::AssetRegistrySnapshot& snapshot);
        void rebuildCurrentView();
        void rebuildBreadcrumbs();
        void repairStateAfterSynchronize();
        void navigateHistoryTo(std::size_t cursor);

        am::core::assets::AssetPath currentFolder_{};
        std::uint64_t synchronizedGeneration_ = 0;
        bool synchronized_ = false;
        ViewMode viewMode_ = ViewMode::Tiles;
        std::array<bool, assetTypeCount_> visibleTypes_{};
        std::string searchText_;
        std::string foldedSearchText_;

        std::vector<am::core::assets::AssetPath> history_;
        std::size_t historyCursor_ = 0;
        std::vector<Breadcrumb> breadcrumbs_;
        std::vector<FolderTreeEntry> folderTree_;
        std::vector<Item> allItems_;
        std::vector<Item> visibleItems_;
        std::size_t unfilteredItemCount_ = 0;

        std::unordered_set<std::string> folderKeys_;
        std::unordered_map<std::string, std::vector<std::size_t>> childrenByFolder_;
        std::unordered_map<std::string, std::size_t> allItemByKey_;
        std::unordered_set<std::string> selectionKeys_;
        std::optional<std::string> selectionAnchorKey_;
        std::optional<std::string> renameKey_;
    };
}
