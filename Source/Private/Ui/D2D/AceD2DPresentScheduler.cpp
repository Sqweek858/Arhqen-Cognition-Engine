#include "ArhqenCognitionEngine/Ui/D2D/AceD2DPresentScheduler.h"

#include <algorithm>
#include <iomanip>
#include <numeric>
#include <sstream>

namespace am::ui
{
    void AceD2DPresentScheduler::Reset()
    {
        stats_ = {};
        lastResizeEpoch_ = 0;
        recentPresentMs_.clear();
    }

    AceD2DPresentPlan AceD2DPresentScheduler::BuildPlan(const AceD2DPresentInput& input)
    {
        ++stats_.plans;
        AceD2DPresentPlan plan{};
        const bool dirtyPresent = !input.fullFrameRedraw && !input.liveResize && !input.requestedDirtyRect.empty();
        plan.mode = dirtyPresent
            ? AceD2DPresentSchedulerMode::DirtyRectPresent1
            : AceD2DPresentSchedulerMode::FullFrameFlipSequential;
        plan.callPresent = input.hasSwapChain && !input.deviceLost;
        plan.usePresent1DirtyRects = dirtyPresent;
        plan.waitForVsync = true;
        plan.discardRetainedContentsAssumption = !dirtyPresent;
        plan.allowTearing = false;
        plan.syncInterval = 1;
        plan.flags = 0;
        plan.presentRect = dirtyPresent ? input.requestedDirtyRect : input.frameRect;
        plan.resizeSafeFrame = input.resizeEpoch != lastResizeEpoch_ || input.liveResize;

        std::ostringstream reason;
        reason << (dirtyPresent ? "present1_dirty_rect" : "present_full_frame_flip")
            << ";swapchain=" << (input.hasSwapChain ? "true" : "false")
            << ";full_frame=" << (input.fullFrameRedraw ? "true" : "false")
            << ";dirty_present=" << (dirtyPresent ? "true" : "false")
            << ";retained_contents=" << (dirtyPresent ? "true" : "false")
            << ";viewport=" << (input.viewportActive ? "true" : "false")
            << ";resize_safe=" << (plan.resizeSafeFrame ? "true" : "false")
            << ";frame=" << input.frameNumber;
        if (!input.fullFrameRedraw && !dirtyPresent)
        {
            ++stats_.dirtyRectRejected;
            reason << ";partial_request_rejected=1";
        }
        if (plan.discardRetainedContentsAssumption)
        {
            ++stats_.retainedContentRejected;
        }
        if (input.deviceLost)
        {
            ++stats_.deviceLostEvents;
            reason << ";device_lost=1";
        }
        if (plan.resizeSafeFrame)
        {
            ++stats_.resizeSafeFrames;
        }
        plan.reason = reason.str();
        stats_.lastReason = plan.reason;
        lastResizeEpoch_ = input.resizeEpoch;
        return plan;
    }

    void AceD2DPresentScheduler::RecordPresent(const AceD2DPresentPlan& plan, HRESULT hr, double presentMs)
    {
        if (!plan.callPresent)
        {
            return;
        }
        ++stats_.presents;
        stats_.lastPresentHr = hr;
        if (FAILED(hr))
        {
            ++stats_.presentFailures;
        }
        PushPresentMs(presentMs);
    }

    void AceD2DPresentScheduler::PushPresentMs(double valueMs)
    {
        valueMs = std::max(0.0, valueMs);
        if (recentPresentMs_.size() == 240)
        {
            recentPresentMs_.pop_front();
        }
        recentPresentMs_.push_back(valueMs);
        stats_.lastPresentMs = valueMs;
        stats_.maxPresentMs = std::max(stats_.maxPresentMs, valueMs);
        const double sum = std::accumulate(recentPresentMs_.begin(), recentPresentMs_.end(), 0.0);
        stats_.averagePresentMs = recentPresentMs_.empty() ? 0.0 : sum / static_cast<double>(recentPresentMs_.size());
    }

    std::string AceD2DPresentScheduler::Diagnostics() const
    {
        std::ostringstream oss;
        oss << "plans=" << stats_.plans
            << ";presents=" << stats_.presents
            << ";present_failures=" << stats_.presentFailures
            << ";resize_safe=" << stats_.resizeSafeFrames
            << ";dirty_rejected=" << stats_.dirtyRectRejected
            << ";retained_rejected=" << stats_.retainedContentRejected
            << ";device_lost=" << stats_.deviceLostEvents
            << ";last_ms=" << std::fixed << std::setprecision(2) << stats_.lastPresentMs
            << ";avg_ms=" << stats_.averagePresentMs
            << ";max_ms=" << stats_.maxPresentMs
            << ";last_hr=" << slate::HResultHex(stats_.lastPresentHr)
            << ";reason=" << stats_.lastReason;
        return oss.str();
    }

    std::wstring AceD2DPresentScheduler::WideDiagnostics() const
    {
        const std::string text = Diagnostics();
        return std::wstring(text.begin(), text.end());
    }

    const char* AceD2DPresentSchedulerModeName(AceD2DPresentSchedulerMode mode)
    {
        switch (mode)
        {
        case AceD2DPresentSchedulerMode::FullFrameFlipSequential: return "full_frame_flip_sequential";
        case AceD2DPresentSchedulerMode::FullFrameFlipDiscard: return "full_frame_flip_discard";
        case AceD2DPresentSchedulerMode::DirtyRectPresent1: return "dirty_rect_present1";
        case AceD2DPresentSchedulerMode::LegacyRetainedContentsForbidden: return "legacy_retained_contents_forbidden";
        default: return "unknown";
        }
    }
}
