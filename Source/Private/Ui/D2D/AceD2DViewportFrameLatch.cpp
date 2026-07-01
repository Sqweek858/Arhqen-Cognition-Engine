#include "ArhqenCognitionEngine/Ui/D2D/AceD2DViewportFrameLatch.h"

#include <sstream>

namespace am::ui
{
    void AceD2DViewportFrameLatch::Reset()
    {
        ready_.reset();
        lastGood_.reset();
        stats_ = {};
    }

    void AceD2DViewportFrameLatch::Produce(std::uint64_t frameNumber, std::uint64_t resourceEpoch, std::size_t slotIndex)
    {
        AceD2DViewportFrameLatchState state{};
        state.producedFrame = frameNumber;
        state.resourceEpoch = resourceEpoch;
        state.slotIndex = slotIndex;
        state.ready = true;
        state.consumed = false;
        ready_ = state;
        ++stats_.produced;
        stats_.lastReason = "produced";
    }

    std::optional<AceD2DViewportFrameLatchState> AceD2DViewportFrameLatch::Consume(std::uint64_t frameNumber, std::uint64_t resourceEpoch)
    {
        if (ready_.has_value() && ready_->resourceEpoch == resourceEpoch)
        {
            auto state = *ready_;
            state.consumedFrame = frameNumber;
            state.consumed = true;
            lastGood_ = state;
            ready_.reset();
            ++stats_.consumed;
            stats_.lastReason = "consume_ready";
            return state;
        }
        if (lastGood_.has_value() && lastGood_->resourceEpoch == resourceEpoch)
        {
            ++stats_.reusedLastGood;
            stats_.lastReason = "reuse_last_good";
            return lastGood_;
        }
        ++stats_.missed;
        stats_.lastReason = "miss";
        return std::nullopt;
    }

    std::optional<AceD2DViewportFrameLatchState> AceD2DViewportFrameLatch::LastGood(std::uint64_t resourceEpoch) const
    {
        if (lastGood_.has_value() && lastGood_->resourceEpoch == resourceEpoch)
        {
            return lastGood_;
        }
        return std::nullopt;
    }

    void AceD2DViewportFrameLatch::InvalidateEpoch(std::uint64_t resourceEpoch)
    {
        bool changed = false;
        if (ready_.has_value() && ready_->resourceEpoch != resourceEpoch)
        {
            ready_.reset();
            changed = true;
        }
        if (lastGood_.has_value() && lastGood_->resourceEpoch != resourceEpoch)
        {
            lastGood_.reset();
            changed = true;
        }
        if (changed)
        {
            ++stats_.invalidated;
            ++stats_.epochChanges;
            stats_.lastReason = "epoch_change";
        }
    }

    std::string AceD2DViewportFrameLatch::Diagnostics() const
    {
        std::ostringstream oss;
        oss << "produced=" << stats_.produced
            << ";consumed=" << stats_.consumed
            << ";reuse=" << stats_.reusedLastGood
            << ";missed=" << stats_.missed
            << ";invalidated=" << stats_.invalidated
            << ";epoch_changes=" << stats_.epochChanges
            << ";ready=" << (ready_.has_value() ? "true" : "false")
            << ";last_good=" << (lastGood_.has_value() ? "true" : "false")
            << ";reason=" << stats_.lastReason;
        return oss.str();
    }

    std::wstring AceD2DViewportFrameLatch::WideDiagnostics() const
    {
        const std::string text = Diagnostics();
        return std::wstring(text.begin(), text.end());
    }
}
