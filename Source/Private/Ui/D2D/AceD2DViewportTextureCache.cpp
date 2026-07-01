#include "ArhqenCognitionEngine/Ui/D2D/AceD2DViewportTextureCache.h"

#include <algorithm>
#include <sstream>

namespace am::ui
{
    bool AceD2DViewportTextureKey::operator==(const AceD2DViewportTextureKey& rhs) const
    {
        return nativeResource == rhs.nativeResource &&
            width == rhs.width &&
            height == rhs.height &&
            format == rhs.format &&
            resourceEpoch == rhs.resourceEpoch &&
            resizeEpoch == rhs.resizeEpoch;
    }

    AceD2DViewportTextureCache::AceD2DViewportTextureCache(std::size_t capacity)
        : capacity_(std::max<std::size_t>(2, capacity))
    {
        entries_.reserve(capacity_);
    }

    void AceD2DViewportTextureCache::Reset()
    {
        entries_.clear();
        stats_ = {};
    }

    AceD2DViewportTextureCacheEntry* AceD2DViewportTextureCache::FindMutable(const AceD2DViewportTextureKey& key, std::uint64_t frameNumber)
    {
        ++stats_.lookups;
        const std::size_t index = FindIndex(key);
        if (index == static_cast<std::size_t>(-1))
        {
            ++stats_.misses;
            return nullptr;
        }
        auto& entry = entries_[index];
        entry.lastTouchedFrame = frameNumber;
        ++entry.hitCount;
        ++stats_.hits;
        return &entry;
    }

    const AceD2DViewportTextureCacheEntry* AceD2DViewportTextureCache::Find(const AceD2DViewportTextureKey& key) const
    {
        const std::size_t index = FindIndex(key);
        return index == static_cast<std::size_t>(-1) ? nullptr : &entries_[index];
    }

    AceD2DViewportTextureCacheEntry& AceD2DViewportTextureCache::InsertOrAssign(const AceD2DViewportTextureCacheEntry& entry, std::uint64_t frameNumber)
    {
        const std::size_t index = FindIndex(entry.key);
        if (index != static_cast<std::size_t>(-1))
        {
            entries_[index] = entry;
            entries_[index].lastTouchedFrame = frameNumber;
            stats_.liveEntries = entries_.size();
            return entries_[index];
        }
        if (entries_.size() >= capacity_)
        {
            EvictOne();
        }
        entries_.push_back(entry);
        entries_.back().firstFrame = frameNumber;
        entries_.back().lastTouchedFrame = frameNumber;
        ++stats_.inserts;
        stats_.liveEntries = entries_.size();
        return entries_.back();
    }

    void AceD2DViewportTextureCache::RecordDirectSurfaceRejected(const AceD2DViewportTextureKey& key, const std::string& reason)
    {
        AceD2DViewportTextureCacheEntry entry{};
        if (const auto* existing = Find(key))
        {
            entry = *existing;
        }
        else
        {
            entry.key = key;
        }
        entry.directSurfaceRejected = true;
        entry.diagnostics = reason;
        InsertOrAssign(entry, entry.lastTouchedFrame);
        ++stats_.directSurfaceRejects;
        stats_.lastReason = reason;
    }

    void AceD2DViewportTextureCache::RecordSharedTextureAccepted(const AceD2DViewportTextureKey& key, const std::string& reason)
    {
        AceD2DViewportTextureCacheEntry entry{};
        if (const auto* existing = Find(key))
        {
            entry = *existing;
        }
        else
        {
            entry.key = key;
        }
        entry.sharedTextureValid = true;
        entry.d2dBitmapValid = true;
        entry.directSurfaceRejected = true;
        entry.stale = false;
        entry.diagnostics = reason;
        InsertOrAssign(entry, entry.lastTouchedFrame);
        ++stats_.sharedTextureAccepts;
        stats_.lastReason = reason;
    }

    void AceD2DViewportTextureCache::InvalidateResizeEpoch(std::uint64_t resizeEpoch)
    {
        for (auto& entry : entries_)
        {
            if (entry.key.resizeEpoch != resizeEpoch)
            {
                entry.stale = true;
            }
        }
        const auto before = entries_.size();
        entries_.erase(std::remove_if(entries_.begin(), entries_.end(), [](const AceD2DViewportTextureCacheEntry& entry)
        {
            return entry.stale;
        }), entries_.end());
        if (entries_.size() != before)
        {
            ++stats_.invalidations;
            ++stats_.resizeInvalidations;
            stats_.staleEvictions += before - entries_.size();
            stats_.lastReason = "resize_epoch";
        }
        stats_.liveEntries = entries_.size();
    }

    void AceD2DViewportTextureCache::InvalidateResource(const void* nativeResource)
    {
        const auto before = entries_.size();
        entries_.erase(std::remove_if(entries_.begin(), entries_.end(), [&](const AceD2DViewportTextureCacheEntry& entry)
        {
            return entry.key.nativeResource == nativeResource;
        }), entries_.end());
        if (entries_.size() != before)
        {
            ++stats_.invalidations;
            ++stats_.resourceInvalidations;
            stats_.staleEvictions += before - entries_.size();
            stats_.lastReason = "resource";
        }
        stats_.liveEntries = entries_.size();
    }

    void AceD2DViewportTextureCache::RetireOlderThan(std::uint64_t frameNumber, std::uint64_t maxAgeFrames)
    {
        const auto before = entries_.size();
        entries_.erase(std::remove_if(entries_.begin(), entries_.end(), [&](const AceD2DViewportTextureCacheEntry& entry)
        {
            return frameNumber > entry.lastTouchedFrame && frameNumber - entry.lastTouchedFrame > maxAgeFrames;
        }), entries_.end());
        if (entries_.size() != before)
        {
            stats_.staleEvictions += before - entries_.size();
            stats_.lastReason = "retire_old";
        }
        stats_.liveEntries = entries_.size();
    }

    std::string AceD2DViewportTextureCache::Diagnostics() const
    {
        std::ostringstream oss;
        oss << "lookups=" << stats_.lookups
            << ";hits=" << stats_.hits
            << ";misses=" << stats_.misses
            << ";inserts=" << stats_.inserts
            << ";invalidations=" << stats_.invalidations
            << ";resize_invalidations=" << stats_.resizeInvalidations
            << ";resource_invalidations=" << stats_.resourceInvalidations
            << ";direct_rejects=" << stats_.directSurfaceRejects
            << ";shared_accepts=" << stats_.sharedTextureAccepts
            << ";stale_evictions=" << stats_.staleEvictions
            << ";live=" << stats_.liveEntries
            << ";reason=" << stats_.lastReason;
        return oss.str();
    }

    std::wstring AceD2DViewportTextureCache::WideDiagnostics() const
    {
        const std::string text = Diagnostics();
        return std::wstring(text.begin(), text.end());
    }

    std::size_t AceD2DViewportTextureCache::FindIndex(const AceD2DViewportTextureKey& key) const
    {
        for (std::size_t i = 0; i < entries_.size(); ++i)
        {
            if (entries_[i].key == key)
            {
                return i;
            }
        }
        return static_cast<std::size_t>(-1);
    }

    void AceD2DViewportTextureCache::EvictOne()
    {
        if (entries_.empty())
        {
            return;
        }
        const auto it = std::min_element(entries_.begin(), entries_.end(), [](const AceD2DViewportTextureCacheEntry& a, const AceD2DViewportTextureCacheEntry& b)
        {
            if (a.lastTouchedFrame != b.lastTouchedFrame)
            {
                return a.lastTouchedFrame < b.lastTouchedFrame;
            }
            return a.hitCount < b.hitCount;
        });
        if (it != entries_.end())
        {
            entries_.erase(it);
            ++stats_.staleEvictions;
        }
    }

    AceD2DViewportTextureKey AceD2DViewportTextureKeyFromResource(
        const am::renderer::rhi::AceViewportTextureResource& resource,
        std::uint32_t format,
        std::uint64_t resourceEpoch,
        std::uint64_t resizeEpoch)
    {
        AceD2DViewportTextureKey key{};
        key.nativeResource = resource.nativeResource;
        key.width = resource.extent.width;
        key.height = resource.extent.height;
        key.format = format;
        key.resourceEpoch = resourceEpoch;
        key.resizeEpoch = resizeEpoch;
        return key;
    }
}
