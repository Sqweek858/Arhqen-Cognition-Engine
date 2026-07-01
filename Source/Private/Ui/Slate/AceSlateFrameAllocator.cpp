#include "ArhqenCognitionEngine/Ui/Slate/AceSlateFrameAllocator.h"

#include <algorithm>
#include <sstream>

namespace am::ui::slate
{
    AceSlateFrameAllocator::AceSlateFrameAllocator(std::size_t capacity)
    {
        Reset(capacity);
    }

    void AceSlateFrameAllocator::Reset(std::size_t capacity)
    {
        buffer_.assign(std::max<std::size_t>(1024, capacity), std::byte{});
        cursor_ = 0;
        frameNumber_ = 0;
        stats_ = {};
        stats_.capacity = buffer_.size();
    }

    void AceSlateFrameAllocator::BeginFrame(std::uint64_t frameNumber)
    {
        frameNumber_ = frameNumber;
        cursor_ = 0;
        ++stats_.frames;
        ++stats_.rewinds;
        stats_.used = 0;
    }

    AceSlateFrameAllocation AceSlateFrameAllocator::Allocate(std::size_t size, std::size_t alignment, const char* tag)
    {
        AceSlateFrameAllocation allocation{};
        allocation.alignment = std::max<std::size_t>(1, alignment);
        allocation.offset = AlignUp(cursor_, allocation.alignment);
        allocation.size = size;
        allocation.frameNumber = frameNumber_;
        allocation.tag = tag;
        if (allocation.offset + size > buffer_.size())
        {
            ++stats_.failedAllocations;
            std::ostringstream oss;
            oss << "frame_allocator_oom;size=" << size
                << ";alignment=" << alignment
                << ";capacity=" << buffer_.size()
                << ";cursor=" << cursor_
                << ";tag=" << (tag ? tag : "none");
            stats_.lastFailure = oss.str();
            return allocation;
        }
        allocation.valid = true;
        cursor_ = allocation.offset + size;
        ++stats_.allocations;
        stats_.used = cursor_;
        stats_.highWater = std::max(stats_.highWater, cursor_);
        return allocation;
    }

    void AceSlateFrameAllocator::RewindTo(std::size_t offset)
    {
        cursor_ = std::min(offset, buffer_.size());
        stats_.used = cursor_;
        ++stats_.rewinds;
    }

    void AceSlateFrameAllocator::Clear()
    {
        cursor_ = 0;
        stats_.used = 0;
    }

    std::string AceSlateFrameAllocator::Diagnostics() const
    {
        std::ostringstream oss;
        oss << "frames=" << stats_.frames
            << ";allocations=" << stats_.allocations
            << ";failed=" << stats_.failedAllocations
            << ";rewinds=" << stats_.rewinds
            << ";capacity=" << stats_.capacity
            << ";used=" << stats_.used
            << ";high_water=" << stats_.highWater
            << ";last_failure=" << stats_.lastFailure;
        return oss.str();
    }

    std::wstring AceSlateFrameAllocator::WideDiagnostics() const
    {
        const std::string text = Diagnostics();
        return std::wstring(text.begin(), text.end());
    }

    std::size_t AceSlateFrameAllocator::AlignUp(std::size_t value, std::size_t alignment) const
    {
        if (alignment <= 1)
        {
            return value;
        }
        const std::size_t mask = alignment - 1;
        if ((alignment & mask) != 0)
        {
            alignment = 1;
            while (alignment < mask + 1)
            {
                alignment <<= 1;
            }
        }
        return (value + alignment - 1) & ~(alignment - 1);
    }
}
