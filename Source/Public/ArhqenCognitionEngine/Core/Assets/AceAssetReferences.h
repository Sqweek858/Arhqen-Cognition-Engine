#pragma once

#include "ArhqenCognitionEngine/Core/Identity/AceGuid.h"

#include <span>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace am::core::assets
{
    class AssetReferenceIndex final
    {
    public:
        bool setReferences(const Guid& referencer, std::span<const Guid> targets, std::string* error = nullptr);
        bool removeReferencer(const Guid& referencer) noexcept;
        void clear() noexcept;

        [[nodiscard]] std::vector<Guid> referencesFrom(const Guid& referencer) const;
        [[nodiscard]] std::vector<Guid> referencersTo(const Guid& target) const;
        [[nodiscard]] bool hasReferencers(const Guid& target) const noexcept;
        [[nodiscard]] bool canDelete(const Guid& target) const noexcept { return target.isValid() && !hasReferencers(target); }
        [[nodiscard]] std::size_t referencerCount() const noexcept { return forward_.size(); }
        [[nodiscard]] std::size_t edgeCount() const noexcept { return edgeCount_; }

    private:
        using GuidSet = std::unordered_set<Guid, GuidHash>;
        std::unordered_map<Guid, GuidSet, GuidHash> forward_;
        std::unordered_map<Guid, GuidSet, GuidHash> reverse_;
        std::size_t edgeCount_ = 0;
    };
}
