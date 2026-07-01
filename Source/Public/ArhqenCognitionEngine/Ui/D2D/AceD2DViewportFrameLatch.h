#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace am::ui
{
    struct AceD2DViewportFrameLatchState
    {
        std::uint64_t producedFrame = 0;
        std::uint64_t consumedFrame = 0;
        std::uint64_t resourceEpoch = 0;
        std::size_t slotIndex = 0;
        bool ready = false;
        bool consumed = false;
    };

    struct AceD2DViewportFrameLatchStats
    {
        std::uint64_t produced = 0;
        std::uint64_t consumed = 0;
        std::uint64_t reusedLastGood = 0;
        std::uint64_t missed = 0;
        std::uint64_t invalidated = 0;
        std::uint64_t epochChanges = 0;
        std::string lastReason;
    };

    class AceD2DViewportFrameLatch
    {
    public:
        void Reset();
        void Produce(std::uint64_t frameNumber, std::uint64_t resourceEpoch, std::size_t slotIndex);
        std::optional<AceD2DViewportFrameLatchState> Consume(std::uint64_t frameNumber, std::uint64_t resourceEpoch);
        std::optional<AceD2DViewportFrameLatchState> LastGood(std::uint64_t resourceEpoch) const;
        void InvalidateEpoch(std::uint64_t resourceEpoch);
        const AceD2DViewportFrameLatchStats& Stats() const { return stats_; }
        std::string Diagnostics() const;
        std::wstring WideDiagnostics() const;
    private:
        std::optional<AceD2DViewportFrameLatchState> ready_{};
        std::optional<AceD2DViewportFrameLatchState> lastGood_{};
        AceD2DViewportFrameLatchStats stats_{};
    };
}
