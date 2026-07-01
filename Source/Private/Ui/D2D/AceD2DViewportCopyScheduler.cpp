#include "ArhqenCognitionEngine/Ui/D2D/AceD2DViewportCopyScheduler.h"

#include <sstream>

namespace am::ui
{
    void AceD2DViewportCopyScheduler::Reset()
    {
        stats_ = {};
        executedCopies_ = 0;
        executedDraws_ = 0;
        failedCopies_ = 0;
        failedDraws_ = 0;
    }

    AceD2DViewportCopySchedule AceD2DViewportCopyScheduler::BuildSchedule(const AceD2DViewportCopyScheduleInput& input)
    {
        ++stats_.schedules;
        AceD2DViewportCopySchedule schedule{};
        schedule.copy = input.bridgeDecision.copyThisFrame;
        schedule.draw = input.bridgeDecision.drawThisFrame;
        schedule.copySlot = input.bridgeDecision.writeSlot;
        schedule.drawSlot = input.bridgeDecision.drawSlot;
        schedule.drawPreviousFrame = input.bridgeDecision.drawLastGood;
        schedule.requiresFlush = input.bridgeDecision.needsFlushAfterCopy;
        schedule.requiresD2DFlush = input.bridgeDecision.needsD2DFlushBeforeRelease;
        schedule.safe = true;
        schedule.hazard = AceD2DViewportCopyHazard::None;

        if (schedule.copySlot == schedule.drawSlot && !input.bridgeDecision.firstFrame && !input.keyedMutexEnabled)
        {
            schedule.safe = false;
            schedule.hazard = AceD2DViewportCopyHazard::SameSlotReadWrite;
            ++stats_.sameSlotAvoided;
        }
        else if (!input.bridgeDecision.lastGoodSlot.has_value() && !input.bridgeDecision.firstFrame)
        {
            schedule.safe = false;
            schedule.hazard = AceD2DViewportCopyHazard::MissingLastGood;
        }
        else if (!input.fullFrame)
        {
            schedule.safe = false;
            schedule.hazard = AceD2DViewportCopyHazard::CopyWithoutFullFrame;
        }
        else if (input.liveResize && input.textureKey.resizeEpoch != input.resizeEpoch)
        {
            schedule.safe = false;
            schedule.hazard = AceD2DViewportCopyHazard::ResizeEpochMismatch;
            ++stats_.resizeMismatch;
        }
        else if (input.keyedMutexEnabled)
        {
            schedule.hazard = AceD2DViewportCopyHazard::KeyedMutexStallRisk;
            ++stats_.keyedRisk;
        }

        if (schedule.safe)
        {
            ++stats_.safeSchedules;
        }
        else
        {
            ++stats_.hazards;
        }
        if (schedule.drawPreviousFrame)
        {
            ++stats_.previousFrameDraws;
        }
        else
        {
            ++stats_.sameFrameDraws;
        }

        std::ostringstream reason;
        reason << "copy_schedule"
            << ";copy=" << (schedule.copy ? "true" : "false")
            << ";draw=" << (schedule.draw ? "true" : "false")
            << ";copy_slot=" << schedule.copySlot
            << ";draw_slot=" << schedule.drawSlot
            << ";previous=" << (schedule.drawPreviousFrame ? "true" : "false")
            << ";flush=" << (schedule.requiresFlush ? "true" : "false")
            << ";d2d_flush=" << (schedule.requiresD2DFlush ? "true" : "false")
            << ";safe=" << (schedule.safe ? "true" : "false")
            << ";hazard=" << AceD2DViewportCopyHazardName(schedule.hazard)
            << ";frame=" << input.frameNumber
            << ";epoch=" << input.textureKey.resourceEpoch
            << ";resize=" << input.resizeEpoch;
        schedule.reason = reason.str();
        stats_.lastReason = schedule.reason;
        return schedule;
    }

    void AceD2DViewportCopyScheduler::RecordExecuted(const AceD2DViewportCopySchedule& schedule, bool copyOk, bool drawOk)
    {
        if (schedule.copy)
        {
            if (copyOk)
            {
                ++executedCopies_;
            }
            else
            {
                ++failedCopies_;
            }
        }
        if (schedule.draw)
        {
            if (drawOk)
            {
                ++executedDraws_;
            }
            else
            {
                ++failedDraws_;
            }
        }
    }

    std::string AceD2DViewportCopyScheduler::Diagnostics() const
    {
        std::ostringstream oss;
        oss << "schedules=" << stats_.schedules
            << ";safe=" << stats_.safeSchedules
            << ";hazards=" << stats_.hazards
            << ";same_slot_avoided=" << stats_.sameSlotAvoided
            << ";previous_draws=" << stats_.previousFrameDraws
            << ";same_frame_draws=" << stats_.sameFrameDraws
            << ";resize_mismatch=" << stats_.resizeMismatch
            << ";keyed_risk=" << stats_.keyedRisk
            << ";copies=" << executedCopies_ << '/' << failedCopies_
            << ";draws=" << executedDraws_ << '/' << failedDraws_
            << ";last=" << stats_.lastReason;
        return oss.str();
    }

    std::wstring AceD2DViewportCopyScheduler::WideDiagnostics() const
    {
        const std::string text = Diagnostics();
        return std::wstring(text.begin(), text.end());
    }

    const char* AceD2DViewportCopyHazardName(AceD2DViewportCopyHazard hazard)
    {
        switch (hazard)
        {
        case AceD2DViewportCopyHazard::None: return "none";
        case AceD2DViewportCopyHazard::SameSlotReadWrite: return "same_slot_read_write";
        case AceD2DViewportCopyHazard::MissingLastGood: return "missing_last_good";
        case AceD2DViewportCopyHazard::ResizeEpochMismatch: return "resize_epoch_mismatch";
        case AceD2DViewportCopyHazard::CopyWithoutFullFrame: return "copy_without_full_frame";
        case AceD2DViewportCopyHazard::KeyedMutexStallRisk: return "keyed_mutex_stall_risk";
        default: return "unknown";
        }
    }
}
