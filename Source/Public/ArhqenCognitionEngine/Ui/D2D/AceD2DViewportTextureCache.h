#pragma once

#include "ArhqenCognitionEngine/Renderer/RHI/AceViewportTextureBridge.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DUiTypes.h"
#include "ArhqenCognitionEngine/Ui/Slate/AceSlateFramePrimitives.h"

#include <cstdint>
#include <string>
#include <vector>

namespace am::ui
{
    struct AceD2DViewportTextureKey
    {
        const void* nativeResource = nullptr;
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        std::uint32_t format = 0;
        std::uint64_t resourceEpoch = 0;
        std::uint64_t resizeEpoch = 0;

        bool operator==(const AceD2DViewportTextureKey& rhs) const;
        bool operator!=(const AceD2DViewportTextureKey& rhs) const { return !(*this == rhs); }
    };

    struct AceD2DViewportTextureCacheEntry
    {
        AceD2DViewportTextureKey key{};
        std::uint64_t lastTouchedFrame = 0;
        std::uint64_t firstFrame = 0;
        std::uint64_t hitCount = 0;
        std::uint64_t missCount = 0;
        bool d2dBitmapValid = false;
        bool sharedTextureValid = false;
        bool directSurfaceRejected = true;
        bool stale = false;
        std::string diagnostics;
    };

    struct AceD2DViewportTextureCacheStats
    {
        std::uint64_t lookups = 0;
        std::uint64_t hits = 0;
        std::uint64_t misses = 0;
        std::uint64_t inserts = 0;
        std::uint64_t invalidations = 0;
        std::uint64_t staleEvictions = 0;
        std::uint64_t resizeInvalidations = 0;
        std::uint64_t resourceInvalidations = 0;
        std::uint64_t directSurfaceRejects = 0;
        std::uint64_t sharedTextureAccepts = 0;
        std::size_t liveEntries = 0;
        std::string lastReason;
    };

    class AceD2DViewportTextureCache
    {
    public:
        explicit AceD2DViewportTextureCache(std::size_t capacity = 8);
        void Reset();
        AceD2DViewportTextureCacheEntry* FindMutable(const AceD2DViewportTextureKey& key, std::uint64_t frameNumber);
        const AceD2DViewportTextureCacheEntry* Find(const AceD2DViewportTextureKey& key) const;
        AceD2DViewportTextureCacheEntry& InsertOrAssign(const AceD2DViewportTextureCacheEntry& entry, std::uint64_t frameNumber);
        void RecordDirectSurfaceRejected(const AceD2DViewportTextureKey& key, const std::string& reason);
        void RecordSharedTextureAccepted(const AceD2DViewportTextureKey& key, const std::string& reason);
        void InvalidateResizeEpoch(std::uint64_t resizeEpoch);
        void InvalidateResource(const void* nativeResource);
        void RetireOlderThan(std::uint64_t frameNumber, std::uint64_t maxAgeFrames);
        const AceD2DViewportTextureCacheStats& Stats() const { return stats_; }
        std::string Diagnostics() const;
        std::wstring WideDiagnostics() const;
    private:
        std::size_t FindIndex(const AceD2DViewportTextureKey& key) const;
        void EvictOne();
        std::size_t capacity_ = 8;
        std::vector<AceD2DViewportTextureCacheEntry> entries_{};
        AceD2DViewportTextureCacheStats stats_{};
    };

    AceD2DViewportTextureKey AceD2DViewportTextureKeyFromResource(
        const am::renderer::rhi::AceViewportTextureResource& resource,
        std::uint32_t format,
        std::uint64_t resourceEpoch,
        std::uint64_t resizeEpoch);
}
