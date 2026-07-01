#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace am::ui::slate
{
    struct AceSlateFrameAllocation
    {
        std::size_t offset = 0;
        std::size_t size = 0;
        std::size_t alignment = 1;
        std::uint64_t frameNumber = 0;
        const char* tag = nullptr;
        bool valid = false;
    };

    struct AceSlateFrameAllocatorStats
    {
        std::uint64_t frames = 0;
        std::uint64_t allocations = 0;
        std::uint64_t failedAllocations = 0;
        std::uint64_t rewinds = 0;
        std::size_t capacity = 0;
        std::size_t used = 0;
        std::size_t highWater = 0;
        std::string lastFailure;
    };

    class AceSlateFrameAllocator
    {
    public:
        explicit AceSlateFrameAllocator(std::size_t capacity = 1024 * 1024);
        void Reset(std::size_t capacity);
        void BeginFrame(std::uint64_t frameNumber);
        AceSlateFrameAllocation Allocate(std::size_t size, std::size_t alignment, const char* tag = nullptr);
        void RewindTo(std::size_t offset);
        void Clear();
        std::byte* Data() { return buffer_.empty() ? nullptr : buffer_.data(); }
        const std::byte* Data() const { return buffer_.empty() ? nullptr : buffer_.data(); }
        const AceSlateFrameAllocatorStats& Stats() const { return stats_; }
        std::string Diagnostics() const;
        std::wstring WideDiagnostics() const;
    private:
        std::size_t AlignUp(std::size_t value, std::size_t alignment) const;
        std::vector<std::byte> buffer_{};
        std::size_t cursor_ = 0;
        std::uint64_t frameNumber_ = 0;
        AceSlateFrameAllocatorStats stats_{};
    };
}
