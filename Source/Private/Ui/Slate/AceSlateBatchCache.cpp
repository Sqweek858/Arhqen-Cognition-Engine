#include "ArhqenCognitionEngine/Ui/Slate/AceSlateBatchCache.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace am::ui::slate
{
    namespace
    {
        std::uint64_t MixHash(std::uint64_t hash, std::uint64_t value)
        {
            value += 0x9e3779b97f4a7c15ull;
            value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ull;
            value = (value ^ (value >> 27)) * 0x94d049bb133111ebull;
            value = value ^ (value >> 31);
            return hash ^ (value + 0x9e3779b97f4a7c15ull + (hash << 6) + (hash >> 2));
        }

        std::uint64_t HashFloat(float value)
        {
            const int scaled = static_cast<int>(value * 1024.0f);
            return static_cast<std::uint64_t>(static_cast<std::int64_t>(scaled));
        }

        std::uint64_t HashRect(UiRect rect)
        {
            std::uint64_t hash = 1469598103934665603ull;
            hash = MixHash(hash, HashFloat(rect.left));
            hash = MixHash(hash, HashFloat(rect.top));
            hash = MixHash(hash, HashFloat(rect.width()));
            hash = MixHash(hash, HashFloat(rect.height()));
            return hash;
        }
    }

    AceSlateBatchCache::AceSlateBatchCache(std::size_t capacity)
        : capacity_(std::max<std::size_t>(16, capacity))
    {
        batches_.reserve(capacity_);
    }

    void AceSlateBatchCache::Reset()
    {
        batches_.clear();
        stats_ = {};
        activeFrame_ = 0;
        activeResourceEpoch_ = 0;
    }

    void AceSlateBatchCache::BeginFrame(const AceSlateBatchCacheInput& input)
    {
        activeFrame_ = input.frameNumber;
        activeResourceEpoch_ = input.resourceEpoch;
        ++stats_.frames;
        if (input.fullFrame && !input.allowRetainedBatches)
        {
            ++stats_.retainedRejected;
            if (!batches_.empty())
            {
                ++stats_.fullFrameInvalidations;
                InvalidateAll("full_frame_flip_no_retained_batches");
            }
        }
        else
        {
            RetireOldFrames(input.frameNumber, 120);
        }
        stats_.liveBatches = batches_.size();
    }

    std::optional<AceSlateCachedBatch> AceSlateBatchCache::Find(std::uint64_t cacheKey, std::uint64_t resourceEpoch)
    {
        ++stats_.lookups;
        const std::size_t index = FindIndex(cacheKey, resourceEpoch);
        if (index == static_cast<std::size_t>(-1))
        {
            ++stats_.misses;
            return std::nullopt;
        }
        auto& batch = batches_[index];
        if (!batch.valid)
        {
            ++stats_.misses;
            return std::nullopt;
        }
        ++batch.hitCount;
        batch.lastUsedFrame = activeFrame_;
        ++stats_.hits;
        return batch;
    }

    void AceSlateBatchCache::Store(const AceSlateCachedBatch& batch)
    {
        if (!batch.valid)
        {
            return;
        }
        const std::size_t existing = FindIndex(batch.cacheKey, batch.resourceEpoch);
        if (existing != static_cast<std::size_t>(-1))
        {
            batches_[existing] = batch;
            batches_[existing].lastUsedFrame = activeFrame_;
            stats_.liveBatches = batches_.size();
            return;
        }
        if (batches_.size() >= capacity_)
        {
            EvictOne();
        }
        batches_.push_back(batch);
        batches_.back().lastUsedFrame = activeFrame_;
        ++stats_.inserts;
        stats_.liveBatches = batches_.size();
    }

    void AceSlateBatchCache::InvalidateAll(const char* reason)
    {
        if (!batches_.empty())
        {
            ++stats_.invalidations;
        }
        batches_.clear();
        stats_.lastReason = reason ? reason : "invalidate_all";
        stats_.liveBatches = 0;
    }

    void AceSlateBatchCache::InvalidateResourceEpoch(std::uint64_t resourceEpoch, const char* reason)
    {
        const auto before = batches_.size();
        batches_.erase(std::remove_if(batches_.begin(), batches_.end(), [&](const AceSlateCachedBatch& batch)
        {
            return batch.resourceEpoch == resourceEpoch;
        }), batches_.end());
        if (batches_.size() != before)
        {
            ++stats_.invalidations;
            ++stats_.resourceEpochInvalidations;
            stats_.lastReason = reason ? reason : "resource_epoch";
        }
        stats_.liveBatches = batches_.size();
    }

    void AceSlateBatchCache::RetireOldFrames(std::uint64_t currentFrame, std::uint64_t maxAgeFrames)
    {
        const auto before = batches_.size();
        batches_.erase(std::remove_if(batches_.begin(), batches_.end(), [&](const AceSlateCachedBatch& batch)
        {
            return currentFrame > batch.lastUsedFrame && currentFrame - batch.lastUsedFrame > maxAgeFrames;
        }), batches_.end());
        if (batches_.size() != before)
        {
            stats_.evictions += before - batches_.size();
            stats_.lastReason = "retire_old_frames";
        }
        stats_.liveBatches = batches_.size();
    }

    std::string AceSlateBatchCache::Diagnostics() const
    {
        std::ostringstream oss;
        oss << "frames=" << stats_.frames
            << ";lookups=" << stats_.lookups
            << ";hits=" << stats_.hits
            << ";misses=" << stats_.misses
            << ";inserts=" << stats_.inserts
            << ";evictions=" << stats_.evictions
            << ";invalidations=" << stats_.invalidations
            << ";full_frame_invalidations=" << stats_.fullFrameInvalidations
            << ";epoch_invalidations=" << stats_.resourceEpochInvalidations
            << ";retained_rejected=" << stats_.retainedRejected
            << ";live=" << stats_.liveBatches
            << ";reason=" << stats_.lastReason;
        return oss.str();
    }

    std::wstring AceSlateBatchCache::WideDiagnostics() const
    {
        const std::string text = Diagnostics();
        return std::wstring(text.begin(), text.end());
    }

    std::size_t AceSlateBatchCache::FindIndex(std::uint64_t cacheKey, std::uint64_t resourceEpoch) const
    {
        for (std::size_t i = 0; i < batches_.size(); ++i)
        {
            if (batches_[i].cacheKey == cacheKey && batches_[i].resourceEpoch == resourceEpoch)
            {
                return i;
            }
        }
        return static_cast<std::size_t>(-1);
    }

    void AceSlateBatchCache::EvictOne()
    {
        if (batches_.empty())
        {
            return;
        }
        const auto it = std::min_element(batches_.begin(), batches_.end(), [](const AceSlateCachedBatch& a, const AceSlateCachedBatch& b)
        {
            if (a.lastUsedFrame != b.lastUsedFrame)
            {
                return a.lastUsedFrame < b.lastUsedFrame;
            }
            return a.hitCount < b.hitCount;
        });
        if (it != batches_.end())
        {
            batches_.erase(it);
            ++stats_.evictions;
        }
    }

    std::uint64_t AceSlateHashElementForCache(const AceSlateElement& element, std::uint64_t resourceEpoch)
    {
        std::uint64_t hash = 1469598103934665603ull;
        hash = MixHash(hash, static_cast<std::uint64_t>(element.type));
        hash = MixHash(hash, static_cast<std::uint64_t>(element.layer));
        hash = MixHash(hash, HashRect(element.geometry.rect));
        hash = MixHash(hash, HashRect(element.clip.rect));
        hash = MixHash(hash, resourceEpoch);
        hash = MixHash(hash, static_cast<std::uint64_t>(element.effects));
        hash = MixHash(hash, static_cast<std::uint64_t>(element.viewport.resourceEpoch));
        return hash;
    }
}
