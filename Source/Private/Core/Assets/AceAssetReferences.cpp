#include "ArhqenCognitionEngine/Core/Assets/AceAssetReferences.h"

#include <algorithm>

namespace am::core::assets
{
    namespace
    {
        constexpr std::size_t kMaximumReferencesPerAsset = 65536;

        void fail(std::string* error, std::string message)
        {
            if (error) *error = std::move(message);
        }

        std::vector<Guid> sorted(const std::unordered_set<Guid, GuidHash>& values)
        {
            std::vector<Guid> result(values.begin(), values.end());
            std::sort(result.begin(), result.end());
            return result;
        }
    }

    bool AssetReferenceIndex::setReferences(const Guid& referencer, std::span<const Guid> targets, std::string* error)
    {
        if (error) error->clear();
        if (!referencer.isValid() || targets.size() > kMaximumReferencesPerAsset)
        {
            fail(error, "Asset reference request is invalid or exceeds its bound");
            return false;
        }

        GuidSet next;
        next.reserve(targets.size());
        for (const Guid& target : targets)
        {
            if (!target.isValid() || target == referencer)
            {
                fail(error, "Asset references require valid distinct identities");
                return false;
            }
            next.insert(target);
        }

        removeReferencer(referencer);
        if (next.empty()) return true;
        for (const Guid& target : next) reverse_[target].insert(referencer);
        edgeCount_ += next.size();
        forward_.emplace(referencer, std::move(next));
        return true;
    }

    bool AssetReferenceIndex::removeReferencer(const Guid& referencer) noexcept
    {
        const auto found = forward_.find(referencer);
        if (found == forward_.end()) return false;
        for (const Guid& target : found->second)
        {
            const auto reverse = reverse_.find(target);
            if (reverse == reverse_.end()) continue;
            reverse->second.erase(referencer);
            if (reverse->second.empty()) reverse_.erase(reverse);
        }
        edgeCount_ -= found->second.size();
        forward_.erase(found);
        return true;
    }

    void AssetReferenceIndex::clear() noexcept
    {
        forward_.clear();
        reverse_.clear();
        edgeCount_ = 0;
    }

    std::vector<Guid> AssetReferenceIndex::referencesFrom(const Guid& referencer) const
    {
        const auto found = forward_.find(referencer);
        return found == forward_.end() ? std::vector<Guid>{} : sorted(found->second);
    }

    std::vector<Guid> AssetReferenceIndex::referencersTo(const Guid& target) const
    {
        const auto found = reverse_.find(target);
        return found == reverse_.end() ? std::vector<Guid>{} : sorted(found->second);
    }

    bool AssetReferenceIndex::hasReferencers(const Guid& target) const noexcept
    {
        const auto found = reverse_.find(target);
        return found != reverse_.end() && !found->second.empty();
    }
}
