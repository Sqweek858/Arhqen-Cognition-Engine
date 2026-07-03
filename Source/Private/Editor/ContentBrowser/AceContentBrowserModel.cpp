#include "ArhqenCognitionEngine/Editor/ContentBrowser/AceContentBrowserModel.h"

#include <algorithm>
#include <cctype>
#include <limits>

namespace am::editor::content_browser
{
    namespace
    {
        using am::core::assets::AssetPath;

        std::string foldAscii(std::string_view text)
        {
            std::string result(text);
            std::transform(result.begin(), result.end(), result.begin(), [](unsigned char value)
            {
                return static_cast<char>(std::tolower(value));
            });
            return result;
        }

        std::size_t pathDepth(std::string_view path) noexcept
        {
            return static_cast<std::size_t>(std::count(path.begin(), path.end(), '/')) - 1;
        }

        bool itemLess(const Item& left, const Item& right)
        {
            if (left.kind != right.kind)
            {
                return left.kind == ItemKind::Folder;
            }
            const std::string leftName = foldAscii(left.displayName);
            const std::string rightName = foldAscii(right.displayName);
            if (leftName != rightName)
            {
                return leftName < rightName;
            }
            return left.path.string() < right.path.string();
        }

        std::string assetDisplayName(const AssetPath& path)
        {
            std::string result(path.leafName());
            const std::size_t dot = result.find_last_of('.');
            if (dot != std::string::npos && dot != 0)
            {
                result.resize(dot);
            }
            return result;
        }
    }

    ContentBrowserModel::ContentBrowserModel()
    {
        visibleTypes_.fill(true);
        currentFolder_ = *AssetPath::parse("/Game");
        history_.push_back(currentFolder_);
        rebuildBreadcrumbs();
    }

    std::size_t ContentBrowserModel::typeIndex(am::core::assets::AssetType type) noexcept
    {
        const auto index = static_cast<std::size_t>(type);
        return index < assetTypeCount_ ? index : 0;
    }

    std::string ContentBrowserModel::folderKey(const AssetPath& path)
    {
        return "F:" + path.comparisonKey();
    }

    std::string ContentBrowserModel::assetKey(const am::core::Guid& id)
    {
        return "A:" + id.toString();
    }

    std::string ContentBrowserModel::itemKey(const Item& item)
    {
        return item.kind == ItemKind::Folder ? folderKey(item.path) : assetKey(item.assetId);
    }

    bool ContentBrowserModel::folderExists(const AssetPath& path) const
    {
        return folderKeys_.contains(folderKey(path));
    }

    void ContentBrowserModel::synchronize(const am::core::assets::AssetRegistrySnapshot& snapshot)
    {
        if (synchronized_ && synchronizedGeneration_ == snapshot.generation)
        {
            return;
        }

        synchronized_ = true;
        synchronizedGeneration_ = snapshot.generation;
        allItems_.clear();
        allItemByKey_.clear();
        childrenByFolder_.clear();
        folderKeys_.clear();
        allItems_.reserve(snapshot.assets.size() + snapshot.folders.size());

        for (const auto& folder : snapshot.folders)
        {
            folderKeys_.insert(folderKey(folder.path));
        }
        if (const auto root = AssetPath::parse("/Game"))
        {
            folderKeys_.insert(folderKey(*root));
        }

        for (const auto& folder : snapshot.folders)
        {
            if (folder.path.isRoot())
            {
                continue;
            }
            Item item;
            item.kind = ItemKind::Folder;
            item.path = folder.path;
            item.displayName = std::string(folder.path.leafName());
            item.descendantAssetCount = folder.descendantAssetCount;
            const std::size_t index = allItems_.size();
            allItems_.push_back(std::move(item));
            childrenByFolder_[folderKey(folder.path.parent())].push_back(index);
        }

        for (const auto& asset : snapshot.assets)
        {
            Item item;
            item.kind = ItemKind::Asset;
            item.assetId = asset.id;
            item.path = asset.path;
            item.assetType = asset.type;
            item.displayName = assetDisplayName(asset.path);
            item.fileSize = asset.fileSize;
            const std::size_t index = allItems_.size();
            allItems_.push_back(std::move(item));
            childrenByFolder_[folderKey(asset.path.parent())].push_back(index);
        }

        for (std::size_t index = 0; index < allItems_.size(); ++index)
        {
            allItemByKey_.emplace(itemKey(allItems_[index]), index);
        }
        for (auto& [key, indices] : childrenByFolder_)
        {
            (void)key;
            std::sort(indices.begin(), indices.end(), [this](std::size_t left, std::size_t right)
            {
                return itemLess(allItems_[left], allItems_[right]);
            });
        }

        rebuildFolderTree(snapshot);
        repairStateAfterSynchronize();
        rebuildCurrentView();
    }

    void ContentBrowserModel::rebuildFolderTree(const am::core::assets::AssetRegistrySnapshot& snapshot)
    {
        folderTree_.clear();
        folderTree_.reserve(snapshot.folders.size());
        for (const auto& folder : snapshot.folders)
        {
            folderTree_.push_back(FolderTreeEntry{
                folder.path,
                pathDepth(folder.path.string()),
                folder.directAssetCount,
                folder.descendantAssetCount});
        }
        std::sort(folderTree_.begin(), folderTree_.end(), [](const FolderTreeEntry& left, const FolderTreeEntry& right)
        {
            const std::string leftKey = left.path.comparisonKey();
            const std::string rightKey = right.path.comparisonKey();
            return leftKey == rightKey ? left.path.string() < right.path.string() : leftKey < rightKey;
        });
    }

    void ContentBrowserModel::repairStateAfterSynchronize()
    {
        while (!folderExists(currentFolder_) && !currentFolder_.isRoot())
        {
            currentFolder_ = currentFolder_.parent();
        }
        if (!folderExists(currentFolder_))
        {
            currentFolder_ = *AssetPath::parse("/Game");
        }

        std::vector<AssetPath> repairedHistory;
        repairedHistory.reserve(history_.size() + 1);
        std::size_t repairedCursor = 0;
        for (std::size_t historyIndex = 0; historyIndex < history_.size(); ++historyIndex)
        {
            auto entry = history_[historyIndex];
            while (!folderExists(entry) && !entry.isRoot())
            {
                entry = entry.parent();
            }
            if (!folderExists(entry))
            {
                continue;
            }
            if (repairedHistory.empty() || repairedHistory.back().comparisonKey() != entry.comparisonKey())
            {
                repairedHistory.push_back(std::move(entry));
            }
            if (historyIndex <= historyCursor_ && !repairedHistory.empty())
            {
                repairedCursor = repairedHistory.size() - 1;
            }
        }
        if (repairedHistory.empty())
        {
            repairedHistory.push_back(currentFolder_);
            repairedCursor = 0;
        }
        else if (repairedHistory[repairedCursor].comparisonKey() != currentFolder_.comparisonKey())
        {
            repairedHistory.erase(repairedHistory.begin() + static_cast<std::ptrdiff_t>(repairedCursor + 1),
                                  repairedHistory.end());
            repairedHistory.push_back(currentFolder_);
            repairedCursor = repairedHistory.size() - 1;
        }
        history_ = std::move(repairedHistory);
        historyCursor_ = repairedCursor;

        for (auto iterator = selectionKeys_.begin(); iterator != selectionKeys_.end();)
        {
            if (!allItemByKey_.contains(*iterator)) iterator = selectionKeys_.erase(iterator);
            else ++iterator;
        }
        if (selectionAnchorKey_ && !allItemByKey_.contains(*selectionAnchorKey_)) selectionAnchorKey_.reset();
        if (renameKey_ && !allItemByKey_.contains(*renameKey_)) renameKey_.reset();
        rebuildBreadcrumbs();
    }

    bool ContentBrowserModel::navigateTo(const AssetPath& folder, bool addToHistory)
    {
        if (!folderExists(folder))
        {
            return false;
        }
        if (currentFolder_.comparisonKey() == folder.comparisonKey())
        {
            return true;
        }
        currentFolder_ = folder;
        clearSelection();
        cancelRename();
        if (addToHistory)
        {
            if (historyCursor_ + 1 < history_.size())
            {
                history_.erase(history_.begin() + static_cast<std::ptrdiff_t>(historyCursor_ + 1), history_.end());
            }
            history_.push_back(folder);
            historyCursor_ = history_.size() - 1;
        }
        rebuildBreadcrumbs();
        rebuildCurrentView();
        return true;
    }

    void ContentBrowserModel::navigateHistoryTo(std::size_t cursor)
    {
        historyCursor_ = cursor;
        currentFolder_ = history_[cursor];
        clearSelection();
        cancelRename();
        rebuildBreadcrumbs();
        rebuildCurrentView();
    }

    bool ContentBrowserModel::back()
    {
        if (!canBack()) return false;
        navigateHistoryTo(historyCursor_ - 1);
        return true;
    }

    bool ContentBrowserModel::forward()
    {
        if (!canForward()) return false;
        navigateHistoryTo(historyCursor_ + 1);
        return true;
    }

    bool ContentBrowserModel::up()
    {
        return canGoUp() && navigateTo(currentFolder_.parent());
    }

    void ContentBrowserModel::rebuildBreadcrumbs()
    {
        breadcrumbs_.clear();
        const auto root = *AssetPath::parse("/Game");
        breadcrumbs_.push_back(Breadcrumb{"Content", root});
        if (currentFolder_.isRoot()) return;

        std::string accumulated = "/Game";
        std::string_view remainder(currentFolder_.string());
        remainder.remove_prefix(6);
        while (!remainder.empty())
        {
            const std::size_t slash = remainder.find('/');
            const std::string_view segment = remainder.substr(0, slash);
            accumulated += "/";
            accumulated.append(segment);
            if (const auto path = AssetPath::parse(accumulated))
            {
                breadcrumbs_.push_back(Breadcrumb{std::string(segment), *path});
            }
            if (slash == std::string_view::npos) break;
            remainder.remove_prefix(slash + 1);
        }
    }

    void ContentBrowserModel::setSearchText(std::string text)
    {
        if (text == searchText_) return;
        searchText_ = std::move(text);
        foldedSearchText_ = foldAscii(searchText_);
        rebuildCurrentView();
    }

    void ContentBrowserModel::setTypeVisible(am::core::assets::AssetType type, bool visible)
    {
        const std::size_t index = typeIndex(type);
        if (visibleTypes_[index] == visible) return;
        visibleTypes_[index] = visible;
        rebuildCurrentView();
    }

    bool ContentBrowserModel::typeVisible(am::core::assets::AssetType type) const noexcept
    {
        return visibleTypes_[typeIndex(type)];
    }

    void ContentBrowserModel::showAllTypes()
    {
        visibleTypes_.fill(true);
        rebuildCurrentView();
    }

    void ContentBrowserModel::rebuildCurrentView()
    {
        visibleItems_.clear();
        const auto found = childrenByFolder_.find(folderKey(currentFolder_));
        if (found == childrenByFolder_.end())
        {
            unfilteredItemCount_ = 0;
            return;
        }

        unfilteredItemCount_ = found->second.size();
        visibleItems_.reserve(unfilteredItemCount_);
        for (const std::size_t index : found->second)
        {
            const Item& item = allItems_[index];
            if (item.kind == ItemKind::Asset && !typeVisible(item.assetType)) continue;
            if (!foldedSearchText_.empty() && foldAscii(item.displayName).find(foldedSearchText_) == std::string::npos)
            {
                continue;
            }
            visibleItems_.push_back(item);
        }
    }

    const Item* ContentBrowserModel::findItemByKey(std::string_view key) const
    {
        const auto found = allItemByKey_.find(std::string(key));
        return found == allItemByKey_.end() ? nullptr : &allItems_[found->second];
    }

    std::optional<std::size_t> ContentBrowserModel::visibleIndexForKey(std::string_view key) const
    {
        for (std::size_t index = 0; index < visibleItems_.size(); ++index)
        {
            if (itemKey(visibleItems_[index]) == key) return index;
        }
        return std::nullopt;
    }

    bool ContentBrowserModel::selectKey(std::string key, SelectionMode mode,
                                        std::optional<std::size_t> visibleIndex)
    {
        if (!allItemByKey_.contains(key)) return false;
        if (mode == SelectionMode::Range)
        {
            if (!visibleIndex || !selectionAnchorKey_)
            {
                mode = SelectionMode::Replace;
            }
            else if (const auto anchor = visibleIndexForKey(*selectionAnchorKey_))
            {
                selectionKeys_.clear();
                const std::size_t first = std::min(*anchor, *visibleIndex);
                const std::size_t last = std::max(*anchor, *visibleIndex);
                for (std::size_t index = first; index <= last; ++index)
                {
                    selectionKeys_.insert(itemKey(visibleItems_[index]));
                }
                cancelRename();
                return true;
            }
            else
            {
                mode = SelectionMode::Replace;
            }
        }

        if (mode == SelectionMode::Replace)
        {
            selectionKeys_.clear();
            selectionKeys_.insert(key);
            selectionAnchorKey_ = key;
        }
        else if (mode == SelectionMode::Add)
        {
            selectionKeys_.insert(key);
            selectionAnchorKey_ = key;
        }
        else if (mode == SelectionMode::Toggle)
        {
            if (selectionKeys_.erase(key) == 0)
            {
                selectionKeys_.insert(key);
                selectionAnchorKey_ = key;
            }
            else if (selectionAnchorKey_ == key)
            {
                selectionAnchorKey_.reset();
            }
        }
        cancelRename();
        return true;
    }

    bool ContentBrowserModel::selectVisible(std::size_t index, SelectionMode mode)
    {
        return index < visibleItems_.size() && selectKey(itemKey(visibleItems_[index]), mode, index);
    }

    bool ContentBrowserModel::selectAsset(const am::core::Guid& id, SelectionMode mode)
    {
        const std::string key = assetKey(id);
        return selectKey(key, mode, visibleIndexForKey(key));
    }

    bool ContentBrowserModel::selectFolder(const AssetPath& path, SelectionMode mode)
    {
        const std::string key = folderKey(path);
        return selectKey(key, mode, visibleIndexForKey(key));
    }

    void ContentBrowserModel::clearSelection() noexcept
    {
        selectionKeys_.clear();
        selectionAnchorKey_.reset();
        cancelRename();
    }

    bool ContentBrowserModel::isSelected(const Item& item) const
    {
        return selectionKeys_.contains(itemKey(item));
    }

    std::vector<Item> ContentBrowserModel::selectedItems() const
    {
        std::vector<Item> result;
        result.reserve(selectionKeys_.size());
        for (const Item& item : allItems_)
        {
            if (isSelected(item)) result.push_back(item);
        }
        std::sort(result.begin(), result.end(), itemLess);
        return result;
    }

    bool ContentBrowserModel::beginRenameSelected()
    {
        if (selectionKeys_.size() != 1) return false;
        const auto& key = *selectionKeys_.begin();
        if (!allItemByKey_.contains(key)) return false;
        renameKey_ = key;
        return true;
    }

    bool ContentBrowserModel::beginRename(const Item& item)
    {
        const std::string key = itemKey(item);
        if (!allItemByKey_.contains(key)) return false;
        selectionKeys_.clear();
        selectionKeys_.insert(key);
        selectionAnchorKey_ = key;
        renameKey_ = key;
        return true;
    }

    const Item* ContentBrowserModel::renameItem() const
    {
        return renameKey_ ? findItemByKey(*renameKey_) : nullptr;
    }
}
