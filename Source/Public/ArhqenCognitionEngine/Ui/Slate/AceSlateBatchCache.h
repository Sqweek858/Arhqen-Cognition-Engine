#pragma once

#include "ArhqenCognitionEngine/Ui/Slate/AceSlateElementList.h"
#include "ArhqenCognitionEngine/Ui/Slate/AceSlateFramePrimitives.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace am::ui::slate
{
    struct AceSlateCachedBatch
    {
        std::uint64_t cacheKey = 0;
        std::uint32_t layer = 0;
        AceSlateElementType kind = AceSlateElementType::Box;
        UiRect bounds{};
        UiRect clip{};
        std::uint64_t resourceEpoch = 0;
        std::uint64_t lastUsedFrame = 0;
        std::uint64_t hitCount = 0;
        bool valid = false;
        std::string debugName;
    };

    struct AceSlateBatchCacheInput
    {
        std::uint64_t frameNumber = 0;
        std::uint64_t resourceEpoch = 0;
        bool fullFrame = true;
        bool allowRetainedBatches = false;
        UiRect frameRect{};
    };

    struct AceSlateBatchCacheStats
    {
        std::uint64_t frames = 0;
        std::uint64_t lookups = 0;
        std::uint64_t hits = 0;
        std::uint64_t misses = 0;
        std::uint64_t inserts = 0;
        std::uint64_t evictions = 0;
        std::uint64_t invalidations = 0;
        std::uint64_t fullFrameInvalidations = 0;
        std::uint64_t resourceEpochInvalidations = 0;
        std::uint64_t retainedRejected = 0;
        std::size_t liveBatches = 0;
        std::string lastReason;
    };

    class AceSlateBatchCache
    {
    public:
        explicit AceSlateBatchCache(std::size_t capacity = 512);
        void Reset();
        void BeginFrame(const AceSlateBatchCacheInput& input);
        std::optional<AceSlateCachedBatch> Find(std::uint64_t cacheKey, std::uint64_t resourceEpoch);
        void Store(const AceSlateCachedBatch& batch);
        void InvalidateAll(const char* reason);
        void InvalidateResourceEpoch(std::uint64_t resourceEpoch, const char* reason);
        void RetireOldFrames(std::uint64_t currentFrame, std::uint64_t maxAgeFrames);
        const AceSlateBatchCacheStats& Stats() const { return stats_; }
        std::string Diagnostics() const;
        std::wstring WideDiagnostics() const;
    private:
        std::size_t FindIndex(std::uint64_t cacheKey, std::uint64_t resourceEpoch) const;
        void EvictOne();
        std::size_t capacity_ = 512;
        std::vector<AceSlateCachedBatch> batches_{};
        AceSlateBatchCacheStats stats_{};
        std::uint64_t activeFrame_ = 0;
        std::uint64_t activeResourceEpoch_ = 0;
    };

    std::uint64_t AceSlateHashElementForCache(const AceSlateElement& element, std::uint64_t resourceEpoch);
}
