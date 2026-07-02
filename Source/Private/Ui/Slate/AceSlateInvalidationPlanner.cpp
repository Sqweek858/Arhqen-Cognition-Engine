#include "ArhqenCognitionEngine/Ui/Slate/AceSlateInvalidationPlanner.h"
#include "ArhqenCognitionEngine/Ui/Slate/AceSlateFramePrimitives.h"

#include <algorithm>
#include <sstream>

namespace am::ui::slate
{
    namespace
    {
        bool HasReason(AceSlateInvalidationReason reasons, AceSlateInvalidationReason reason)
        {
            return (static_cast<std::uint16_t>(reasons) & static_cast<std::uint16_t>(reason)) != 0;
        }
    }

    void AceSlateInvalidationPlanner::Reset()
    {
        lastInvalidationSerial_ = 0;
        lastResizeEpoch_ = 0;
        lastViewportEpoch_ = 0;
        lastOverlayVisible_ = false;
        lastViewportActive_ = false;
        lastPresentHr_ = S_OK;
        fullFramePlanCount_ = 0;
        partialPaintRejectedCount_ = 0;
        history_.clear();
    }

    AceSlateInvalidationPlan AceSlateInvalidationPlanner::BuildPlan(const AceSlateInvalidationInput& input)
    {
        AceSlateInvalidationPlan plan{};
        plan.originalDirtyRect = input.osDirtyRect;
        plan.paintRect = input.windowRect;
        plan.resizeEpoch = input.resizeEpoch;
        plan.viewportEpoch = input.viewportResourceEpoch;
        plan.invalidationSerial = input.invalidationSerial;
        plan.overlayVisible = input.overlayVisible;
        plan.viewportActive = input.viewportActive;
        plan.fullFrame = true;
        plan.rejectPartialPaint = true;
        plan.allowRetainedContents = false;
        plan.allowDirtyRectPresent = false;
        plan.forceViewportRedraw = input.viewportActive;

        if (input.flipSwapChain && input.deviceContextTarget && !input.retainedPartialPaintSafe)
        {
            plan.reasons |= AceSlateInvalidationReason::FullFrameFlipSwapChain;
        }
        if (HasResizeEpochChanged(input))
        {
            plan.reasons |= AceSlateInvalidationReason::ResizeEpochChanged;
        }
        if (HasViewportEpochChanged(input) || input.viewportActive != lastViewportActive_)
        {
            plan.reasons |= AceSlateInvalidationReason::ViewportTextureChanged;
        }
        if (input.overlayVisible != lastOverlayVisible_)
        {
            plan.reasons |= AceSlateInvalidationReason::OverlayVisibilityChanged;
        }
        if (input.liveResize)
        {
            plan.reasons |= AceSlateInvalidationReason::LiveResize;
        }
        if (HasSerialChanged(input))
        {
            plan.reasons |= AceSlateInvalidationReason::LayoutChanged;
        }
        if (input.animationActive)
        {
            plan.reasons |= AceSlateInvalidationReason::AnimationTick;
        }
        if (input.textSelectionActive)
        {
            plan.reasons |= AceSlateInvalidationReason::TextSelection;
        }
        if (input.diagnosticsVisible)
        {
            plan.reasons |= AceSlateInvalidationReason::ForceDebug;
        }

        if (input.retainedPartialPaintSafe && !input.liveResize)
        {
            const UiRect dirty = input.osDirtyRect.empty() ? input.windowRect : ClampRectToWindow(input.osDirtyRect, input.windowRect);
            plan.fullFrame = false;
            plan.rejectPartialPaint = false;
            plan.allowRetainedContents = true;
            plan.allowDirtyRectPresent = input.flipSwapChain;
            plan.paintRect = dirty.empty() ? input.windowRect : dirty;
        }
        else if (!input.flipSwapChain && !input.deviceContextTarget && !input.viewportActive && !input.liveResize)
        {
            const UiRect dirty = input.osDirtyRect.empty() ? input.windowRect : ClampRectToWindow(input.osDirtyRect, input.windowRect);
            plan.fullFrame = false;
            plan.rejectPartialPaint = false;
            plan.allowRetainedContents = true;
            plan.allowDirtyRectPresent = false;
            plan.paintRect = dirty.empty() ? input.windowRect : dirty;
            plan.reasons = AceSlateInvalidationReason::LayoutChanged;
        }
        else
        {
            ++partialPaintRejectedCount_;
            ++fullFramePlanCount_;
        }

        plan.diagnostics = BuildReasonText(plan.reasons);
        return plan;
    }

    void AceSlateInvalidationPlanner::CommitPresentedPlan(const AceSlateInvalidationPlan& plan, HRESULT presentHr)
    {
        lastInvalidationSerial_ = plan.invalidationSerial;
        lastResizeEpoch_ = plan.resizeEpoch;
        lastViewportEpoch_ = plan.viewportEpoch;
        lastOverlayVisible_ = plan.overlayVisible;
        lastViewportActive_ = plan.viewportActive;
        lastPresentHr_ = presentHr;
        AceSlateInvalidationHistoryEntry entry{};
        entry.frameNumber = history_.empty() ? 1 : history_.back().frameNumber + 1;
        entry.paintRect = plan.paintRect;
        entry.dirtyRect = plan.originalDirtyRect;
        entry.fullFrame = plan.fullFrame;
        entry.reasons = plan.reasons;
        history_.push_back(entry);
        if (history_.size() > 64)
        {
            history_.erase(history_.begin());
        }
    }

    bool AceSlateInvalidationPlanner::HasSerialChanged(const AceSlateInvalidationInput& input) const
    {
        return input.invalidationSerial != lastInvalidationSerial_;
    }

    bool AceSlateInvalidationPlanner::HasResizeEpochChanged(const AceSlateInvalidationInput& input) const
    {
        return input.resizeEpoch != lastResizeEpoch_;
    }

    bool AceSlateInvalidationPlanner::HasViewportEpochChanged(const AceSlateInvalidationInput& input) const
    {
        return input.viewportResourceEpoch != lastViewportEpoch_;
    }

    UiRect AceSlateInvalidationPlanner::ClampRectToWindow(UiRect rect, UiRect window) const
    {
        if (rect.empty())
        {
            return window;
        }
        const float left = std::max(window.left, rect.left);
        const float top = std::max(window.top, rect.top);
        const float right = std::min(window.right, rect.right);
        const float bottom = std::min(window.bottom, rect.bottom);
        const UiRect out = makeUiRect(left, top, std::max(left, right), std::max(top, bottom));
        return out.empty() ? window : out;
    }

    std::string AceSlateInvalidationPlanner::BuildReasonText(AceSlateInvalidationReason reasons) const
    {
        if (reasons == AceSlateInvalidationReason::None)
        {
            return "none";
        }
        std::ostringstream oss;
        bool first = true;
        auto append = [&](AceSlateInvalidationReason reason)
        {
            if (HasReason(reasons, reason))
            {
                if (!first)
                {
                    oss << '|';
                }
                first = false;
                oss << AceSlateInvalidationReasonName(reason);
            }
        };
        append(AceSlateInvalidationReason::FullFrameFlipSwapChain);
        append(AceSlateInvalidationReason::ViewportTextureChanged);
        append(AceSlateInvalidationReason::ResizeEpochChanged);
        append(AceSlateInvalidationReason::OverlayVisibilityChanged);
        append(AceSlateInvalidationReason::LiveResize);
        append(AceSlateInvalidationReason::LayoutChanged);
        append(AceSlateInvalidationReason::AnimationTick);
        append(AceSlateInvalidationReason::TextSelection);
        append(AceSlateInvalidationReason::ForceDebug);
        return oss.str();
    }

    std::string AceSlateInvalidationPlanner::Diagnostics() const
    {
        std::ostringstream oss;
        oss << "full_frame_plans=" << fullFramePlanCount_
            << ";partial_rejected=" << partialPaintRejectedCount_
            << ";last_serial=" << lastInvalidationSerial_
            << ";resize_epoch=" << lastResizeEpoch_
            << ";viewport_epoch=" << lastViewportEpoch_
            << ";last_present=" << HResultHex(lastPresentHr_)
            << ";history=" << history_.size();
        if (!history_.empty())
        {
            const auto& last = history_.back();
            oss << ";last_full=" << (last.fullFrame ? "true" : "false")
                << ";last_reason=" << BuildReasonText(last.reasons);
        }
        return oss.str();
    }

    std::wstring AceSlateInvalidationPlanner::WideDiagnostics() const
    {
        const std::string text = Diagnostics();
        return std::wstring(text.begin(), text.end());
    }

    const char* AceSlateInvalidationReasonName(AceSlateInvalidationReason reason)
    {
        switch (reason)
        {
        case AceSlateInvalidationReason::None: return "none";
        case AceSlateInvalidationReason::FullFrameFlipSwapChain: return "full_frame_flip_swapchain";
        case AceSlateInvalidationReason::ViewportTextureChanged: return "viewport_texture_changed";
        case AceSlateInvalidationReason::ResizeEpochChanged: return "resize_epoch_changed";
        case AceSlateInvalidationReason::OverlayVisibilityChanged: return "overlay_visibility_changed";
        case AceSlateInvalidationReason::LiveResize: return "live_resize";
        case AceSlateInvalidationReason::LayoutChanged: return "layout_changed";
        case AceSlateInvalidationReason::AnimationTick: return "animation_tick";
        case AceSlateInvalidationReason::TextSelection: return "text_selection";
        case AceSlateInvalidationReason::ForceDebug: return "force_debug";
        default: return "unknown";
        }
    }
}
