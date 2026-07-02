#include "ArhqenCognitionEngine/Ui/D2D/AceD2DViewportBridgeRuntime.h"

#include <algorithm>
#include <sstream>

namespace am::ui
{
    void AceD2DViewportBridgeRuntime::Reset()
    {
        slots_.clear();
        stats_ = {};
        configuredEpoch_ = 0;
        writeCursor_ = 0;
        lastGoodSlot_.reset();
    }

    void AceD2DViewportBridgeRuntime::ConfigureSlots(std::uint32_t slotCount, std::uint64_t resourceEpoch)
    {
        const std::uint32_t clamped = std::max<std::uint32_t>(1, std::min<std::uint32_t>(slotCount, 4));
        if (slots_.size() == clamped && configuredEpoch_ == resourceEpoch)
        {
            return;
        }
        slots_.clear();
        slots_.reserve(clamped);
        for (std::uint32_t i = 0; i < clamped; ++i)
        {
            AceD2DViewportBridgeSlotRuntime slot{};
            slot.index = i;
            slot.resourceEpoch = resourceEpoch;
            slot.state = AceD2DViewportBridgeSlotState::Empty;
            slots_.push_back(slot);
        }
        configuredEpoch_ = resourceEpoch;
        writeCursor_ = 0;
        lastGoodSlot_.reset();
        ++stats_.slotReinitializations;
        stats_.resizeEpochs = resourceEpoch;
    }

    AceD2DViewportBridgeFrameDecision AceD2DViewportBridgeRuntime::BeginFrame(const AceD2DViewportBridgeFrameInput& input)
    {
        ++stats_.frames;
        ConfigureSlots(input.slotCount, input.resourceEpoch);
        EnsureEpoch(input.resourceEpoch);

        AceD2DViewportBridgeFrameDecision decision{};
        decision.valid = input.bridgeUsesSharedIntermediate && !slots_.empty();
        decision.resourceEpoch = input.resourceEpoch;
        decision.keyedMutex = input.keyedMutexEnabled;
        decision.needsFlushAfterCopy = true;
        decision.needsD2DFlushBeforeRelease = input.keyedMutexEnabled;
        decision.allowSameFrameRead = input.keyedMutexEnabled || input.firstFrameMayUseSameSlot;
        decision.copyThisFrame = decision.valid;
        decision.drawThisFrame = decision.valid;

        if (!decision.valid)
        {
            decision.reason = "bridge_runtime_invalid";
            stats_.lastDecision = decision.reason;
            return decision;
        }

        const auto lastGood = FindLastGoodSlot(input.resourceEpoch);
        decision.lastGoodSlot = lastGood;
        decision.firstFrame = !lastGood.has_value();
        decision.writeSlot = PickWriteSlot(input.frameNumber, input.resourceEpoch, lastGood);
        if (decision.keyedMutex)
        {
            // The writer releases key 1 after CopyResource and D2D acquires key 1
            // before drawing. That is an explicit same-frame handoff; reporting or
            // scheduling the previous slot adds phantom latency and contradicts the
            // actual bridge draw path.
            decision.drawSlot = decision.writeSlot;
            decision.drawLastGood = false;
        }
        else if (lastGood.has_value())
        {
            decision.drawSlot = *lastGood;
            decision.drawLastGood = true;
        }
        else
        {
            decision.drawSlot = decision.writeSlot;
            decision.drawLastGood = false;
        }

        if (decision.drawSlot == decision.writeSlot && !decision.firstFrame && !decision.keyedMutex)
        {
            std::optional<std::size_t> fallback;
            for (const auto& slot : slots_)
            {
                if (slot.index != decision.writeSlot &&
                    slot.resourceEpoch == input.resourceEpoch &&
                    (slot.state == AceD2DViewportBridgeSlotState::Presented || slot.state == AceD2DViewportBridgeSlotState::ReadyForDraw))
                {
                    fallback = slot.index;
                    break;
                }
            }
            if (fallback.has_value())
            {
                decision.drawSlot = *fallback;
                decision.drawLastGood = true;
            }
            else if (!input.firstFrameMayUseSameSlot)
            {
                decision.drawThisFrame = false;
                ++stats_.skippedDraws;
            }
        }

        if (decision.drawSlot == decision.writeSlot)
        {
            ++stats_.sameSlotDraws;
        }
        if (decision.drawLastGood)
        {
            ++stats_.drawsFromLastGood;
        }

        std::ostringstream reason;
        reason << "runtime_frame"
            << ";frame=" << input.frameNumber
            << ";epoch=" << input.resourceEpoch
            << ";slots=" << slots_.size()
            << ";write=" << decision.writeSlot
            << ";draw=" << decision.drawSlot
            << ";last_good=" << (lastGood.has_value() ? std::to_string(*lastGood) : std::string("none"))
            << ";same_frame=" << (decision.drawSlot == decision.writeSlot ? "true" : "false")
            << ";keyed=" << (decision.keyedMutex ? "true" : "false")
            << ";draw_last_good=" << (decision.drawLastGood ? "true" : "false")
            << ";full_frame=" << (input.fullFrameRedraw ? "true" : "false")
            << ";live_resize=" << (input.liveResize ? "true" : "false");
        decision.reason = reason.str();
        stats_.lastDecision = decision.reason;
        stats_.lastWriteSlot = decision.writeSlot;
        stats_.lastDrawSlot = decision.drawSlot;
        return decision;
    }

    void AceD2DViewportBridgeRuntime::MarkCopyStarted(std::size_t slotIndex)
    {
        if (auto* slot = MutableSlot(slotIndex))
        {
            slot->state = AceD2DViewportBridgeSlotState::CopyInProgress;
            ++stats_.copiesRequested;
        }
    }

    void AceD2DViewportBridgeRuntime::MarkCopyCompleted(std::size_t slotIndex)
    {
        if (auto* slot = MutableSlot(slotIndex))
        {
            slot->state = AceD2DViewportBridgeSlotState::ReadyForDraw;
            slot->copyFrame = stats_.frames;
            lastGoodSlot_ = slotIndex;
            stats_.lastGoodEpoch = slot->resourceEpoch;
            ++stats_.copiesCompleted;
        }
    }

    void AceD2DViewportBridgeRuntime::MarkDrawStarted(std::size_t slotIndex)
    {
        if (auto* slot = MutableSlot(slotIndex))
        {
            slot->state = AceD2DViewportBridgeSlotState::Drawing;
            slot->drawFrame = stats_.frames;
            ++stats_.drawsRequested;
        }
    }

    void AceD2DViewportBridgeRuntime::MarkDrawCompleted(std::size_t slotIndex)
    {
        if (auto* slot = MutableSlot(slotIndex))
        {
            slot->state = AceD2DViewportBridgeSlotState::Presented;
            slot->presentFrame = stats_.frames;
            if (!lastGoodSlot_.has_value())
            {
                lastGoodSlot_ = slotIndex;
                stats_.lastGoodEpoch = slot->resourceEpoch;
            }
            else if (const auto* current = Slot(*lastGoodSlot_))
            {
                // ACE-VTBRIDGE5R1: lastGoodSlot_ means "freshest drawable",
                // not "whatever D2D happened to draw this frame". The previous
                // VT5 runtime copied the live camera frame into one shared slot,
                // then MarkDrawCompleted() immediately demoted lastGoodSlot_ back
                // to the old drawn slot. The result was perfect input counters and
                // a visually frozen viewport, a tiny masterpiece of self-sabotage.
                if (slot->resourceEpoch == current->resourceEpoch && slot->copyFrame >= current->copyFrame)
                {
                    lastGoodSlot_ = slotIndex;
                    stats_.lastGoodEpoch = slot->resourceEpoch;
                }
            }
            else
            {
                lastGoodSlot_ = slotIndex;
                stats_.lastGoodEpoch = slot->resourceEpoch;
            }
            ++stats_.drawsCompleted;
        }
    }

    void AceD2DViewportBridgeRuntime::MarkPresented(std::size_t slotIndex)
    {
        if (auto* slot = MutableSlot(slotIndex))
        {
            slot->state = AceD2DViewportBridgeSlotState::Presented;
            slot->presentFrame = stats_.frames;
            if (!lastGoodSlot_.has_value())
            {
                lastGoodSlot_ = slotIndex;
            }
            else if (const auto* current = Slot(*lastGoodSlot_))
            {
                if (slot->resourceEpoch == current->resourceEpoch && slot->copyFrame >= current->copyFrame)
                {
                    lastGoodSlot_ = slotIndex;
                }
            }
        }
    }

    void AceD2DViewportBridgeRuntime::MarkSlotFailure(std::size_t slotIndex, const std::string& failure)
    {
        if (auto* slot = MutableSlot(slotIndex))
        {
            slot->state = AceD2DViewportBridgeSlotState::Failed;
            slot->lastFailure = failure;
            ++slot->failureCount;
        }
        ++stats_.failures;
        stats_.lastFailure = failure;
    }

    void AceD2DViewportBridgeRuntime::MarkAllStale()
    {
        for (auto& slot : slots_)
        {
            if (slot.state != AceD2DViewportBridgeSlotState::Empty)
            {
                slot.state = AceD2DViewportBridgeSlotState::Stale;
            }
        }
        lastGoodSlot_.reset();
    }

    void AceD2DViewportBridgeRuntime::OnResizeOrResourceEpoch(std::uint64_t resourceEpoch)
    {
        if (configuredEpoch_ != resourceEpoch)
        {
            configuredEpoch_ = resourceEpoch;
            for (auto& slot : slots_)
            {
                slot.resourceEpoch = resourceEpoch;
                slot.state = AceD2DViewportBridgeSlotState::Empty;
                slot.copyFrame = 0;
                slot.drawFrame = 0;
                slot.presentFrame = 0;
                slot.lastFailure.clear();
            }
            lastGoodSlot_.reset();
            ++stats_.slotReinitializations;
            stats_.resizeEpochs = resourceEpoch;
        }
    }

    std::optional<std::size_t> AceD2DViewportBridgeRuntime::FindLastGoodSlot(std::uint64_t resourceEpoch) const
    {
        std::optional<std::size_t> best;
        std::uint64_t bestCopyFrame = 0;
        std::uint64_t bestPresentFrame = 0;
        for (const auto& slot : slots_)
        {
            if (slot.resourceEpoch != resourceEpoch ||
                (slot.state != AceD2DViewportBridgeSlotState::ReadyForDraw && slot.state != AceD2DViewportBridgeSlotState::Presented))
            {
                continue;
            }

            const bool preferSlot = !best.has_value() ||
                slot.copyFrame > bestCopyFrame ||
                (slot.copyFrame == bestCopyFrame && slot.presentFrame > bestPresentFrame) ||
                (slot.copyFrame == bestCopyFrame && slot.presentFrame == bestPresentFrame && lastGoodSlot_.has_value() && *lastGoodSlot_ == slot.index);
            if (preferSlot)
            {
                best = slot.index;
                bestCopyFrame = slot.copyFrame;
                bestPresentFrame = slot.presentFrame;
            }
        }
        return best;
    }

    std::size_t AceD2DViewportBridgeRuntime::PickWriteSlot(std::uint64_t frameNumber, std::uint64_t resourceEpoch, std::optional<std::size_t> lastGood) const
    {
        if (slots_.empty())
        {
            return 0;
        }
        const std::size_t count = slots_.size();
        for (std::size_t attempt = 0; attempt < count; ++attempt)
        {
            const std::size_t index = (writeCursor_ + attempt + static_cast<std::size_t>(frameNumber % count)) % count;
            const auto& slot = slots_[index];
            if (lastGood.has_value() && *lastGood == index && count > 1)
            {
                continue;
            }
            if (slot.resourceEpoch == resourceEpoch && slot.state != AceD2DViewportBridgeSlotState::Drawing)
            {
                return index;
            }
        }
        if (lastGood.has_value() && count > 1)
        {
            return (*lastGood + 1) % count;
        }
        return writeCursor_ % count;
    }

    bool AceD2DViewportBridgeRuntime::SlotValid(std::size_t slotIndex) const
    {
        return slotIndex < slots_.size();
    }

    AceD2DViewportBridgeSlotRuntime* AceD2DViewportBridgeRuntime::MutableSlot(std::size_t slotIndex)
    {
        return SlotValid(slotIndex) ? &slots_[slotIndex] : nullptr;
    }

    const AceD2DViewportBridgeSlotRuntime* AceD2DViewportBridgeRuntime::Slot(std::size_t slotIndex) const
    {
        return SlotValid(slotIndex) ? &slots_[slotIndex] : nullptr;
    }

    void AceD2DViewportBridgeRuntime::EnsureEpoch(std::uint64_t resourceEpoch)
    {
        if (configuredEpoch_ != resourceEpoch)
        {
            OnResizeOrResourceEpoch(resourceEpoch);
        }
    }

    std::string AceD2DViewportBridgeRuntime::Diagnostics() const
    {
        std::ostringstream oss;
        oss << "frames=" << stats_.frames
            << ";copies=" << stats_.copiesCompleted << '/' << stats_.copiesRequested
            << ";draws=" << stats_.drawsCompleted << '/' << stats_.drawsRequested
            << ";last_good_draws=" << stats_.drawsFromLastGood
            << ";same_slot_draws=" << stats_.sameSlotDraws
            << ";skipped_draws=" << stats_.skippedDraws
            << ";slot_reinit=" << stats_.slotReinitializations
            << ";failures=" << stats_.failures
            << ";last_write=" << stats_.lastWriteSlot
            << ";last_draw=" << stats_.lastDrawSlot
            << ";last_good_epoch=" << stats_.lastGoodEpoch
            << ";decision=" << stats_.lastDecision;
        for (const auto& slot : slots_)
        {
            oss << ";slot" << slot.index << '=' << AceD2DViewportBridgeSlotStateName(slot.state)
                << "@epoch" << slot.resourceEpoch
                << "/copy" << slot.copyFrame
                << "/draw" << slot.drawFrame
                << "/present" << slot.presentFrame;
        }
        if (!stats_.lastFailure.empty())
        {
            oss << ";last_failure=" << stats_.lastFailure;
        }
        return oss.str();
    }

    std::wstring AceD2DViewportBridgeRuntime::WideDiagnostics() const
    {
        const std::string text = Diagnostics();
        return std::wstring(text.begin(), text.end());
    }

    const char* AceD2DViewportBridgeSlotStateName(AceD2DViewportBridgeSlotState state)
    {
        switch (state)
        {
        case AceD2DViewportBridgeSlotState::Empty: return "empty";
        case AceD2DViewportBridgeSlotState::CopyInProgress: return "copy_in_progress";
        case AceD2DViewportBridgeSlotState::ReadyForDraw: return "ready_for_draw";
        case AceD2DViewportBridgeSlotState::Drawing: return "drawing";
        case AceD2DViewportBridgeSlotState::Presented: return "presented";
        case AceD2DViewportBridgeSlotState::Stale: return "stale";
        case AceD2DViewportBridgeSlotState::Failed: return "failed";
        default: return "unknown";
        }
    }
}
