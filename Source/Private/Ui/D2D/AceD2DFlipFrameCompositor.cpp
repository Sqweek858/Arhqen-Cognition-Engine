#include "ArhqenCognitionEngine/Ui/D2D/AceD2DFlipFrameCompositor.h"

#include <iomanip>

namespace am::ui
{
    namespace
    {
        std::string RectForStats(UiRect rect)
        {
            return slate::RectText(rect);
        }

        std::wstring WidenAscii(const std::string& text)
        {
            std::wstring out;
            out.reserve(text.size());
            for (char ch : text)
            {
                out.push_back(static_cast<unsigned char>(ch));
            }
            return out;
        }
    }

    AceD2DFramePlan AceD2DFlipFrameCompositor::BeginFrame(const AceD2DFrameInput& input)
    {
        currentInput_ = input;
        frameStart_ = std::chrono::steady_clock::now();
        ++stats_.frameNumber;
        ++stats_.beginFrameCount;
        currentPlan_ = BuildPlan(input);
        currentPlan_.reason = BuildReason(input, currentPlan_);
        if (currentPlan_.fullFrameRedraw)
        {
            ++stats_.fullFrameRedrawCount;
        }
        if (currentPlan_.disableFastPartialViewportPaint)
        {
            ++stats_.partialPaintRejectedCount;
        }
        if (input.liveResize)
        {
            ++stats_.liveResizeFullFrameCount;
        }
        if (currentPlan_.useViewportAsNormalElement)
        {
            ++stats_.viewportNormalElementFrames;
        }
        if (!currentPlan_.allowRetainedContents)
        {
            ++stats_.retainedContentsForbiddenFrames;
        }
        stats_.lastPresentMode = currentPlan_.presentMode;
        stats_.lastReason = currentPlan_.reason;
        PushHistory(currentPlan_);
        return currentPlan_;
    }

    void AceD2DFlipFrameCompositor::RecordSlateElements(const slate::AceSlateWindowElementList& elements)
    {
        frameElements_ = elements;
        stats_.lastDiagnostics = elements.Diagnostics();
    }

    void AceD2DFlipFrameCompositor::RecordEndDraw(HRESULT hr)
    {
        stats_.lastEndDrawHr = hr;
        if (FAILED(hr))
        {
            ++stats_.endDrawFailureCount;
        }
    }

    void AceD2DFlipFrameCompositor::RecordPresent(HRESULT hr, double presentMs)
    {
        stats_.lastPresentHr = hr;
        stats_.lastPresentMs = presentMs;
        ++stats_.presentCount;
        if (FAILED(hr))
        {
            ++stats_.presentFailureCount;
        }
    }

    void AceD2DFlipFrameCompositor::EndFrame()
    {
        const auto now = std::chrono::steady_clock::now();
        stats_.lastFrameMs = std::chrono::duration<double, std::milli>(now - frameStart_).count();
    }

    void AceD2DFlipFrameCompositor::Reset()
    {
        currentInput_ = {};
        currentPlan_ = {};
        stats_ = {};
        frameElements_.Reset();
        history_.clear();
        frameStart_ = {};
        resizeEpoch_ = 0;
    }

    void AceD2DFlipFrameCompositor::NotifyResize(std::uint32_t width, std::uint32_t height)
    {
        ++resizeEpoch_;
        currentInput_.width = width;
        currentInput_.height = height;
    }

    std::string AceD2DFlipFrameCompositor::Diagnostics() const
    {
        std::ostringstream oss;
        oss << "vtbridge5_frame_compositor"
            << ";frame=" << stats_.frameNumber
            << ";mode=" << AceD2DPresentModeName(stats_.lastPresentMode)
            << ";full_frame=" << (currentPlan_.fullFrameRedraw ? "true" : "false")
            << ";partial_fast_disabled=" << (currentPlan_.disableFastPartialViewportPaint ? "true" : "false")
            << ";retained_contents=" << (currentPlan_.allowRetainedContents ? "true" : "false")
            << ";viewport_element=" << (currentPlan_.useViewportAsNormalElement ? "true" : "false")
            << ";paint=" << RectForStats(currentPlan_.paintRect)
            << ";endDraw=" << slate::HResultHex(stats_.lastEndDrawHr)
            << ";present=" << slate::HResultHex(stats_.lastPresentHr)
            << ";present_ms=" << std::fixed << std::setprecision(3) << stats_.lastPresentMs
            << ";frame_ms=" << std::fixed << std::setprecision(3) << stats_.lastFrameMs
            << ";reason=" << currentPlan_.reason;
        if (!stats_.lastDiagnostics.empty())
        {
            oss << ";elements=" << stats_.lastDiagnostics;
        }
        return oss.str();
    }

    std::wstring AceD2DFlipFrameCompositor::WideDiagnostics() const
    {
        return WidenAscii(Diagnostics());
    }

    std::vector<std::pair<std::string, std::string>> AceD2DFlipFrameCompositor::KeyValues() const
    {
        return {
            {"d2d_present_mode", AceD2DPresentModeName(stats_.lastPresentMode)},
            {"d2d_full_frame_redraw", currentPlan_.fullFrameRedraw ? "true" : "false"},
            {"d2d_fast_partial_repaints_disabled", currentPlan_.disableFastPartialViewportPaint ? "true" : "false"},
            {"d2d_retained_contents_assumption", currentPlan_.allowRetainedContents ? "true" : "false"},
            {"d2d_viewport_as_draw_element", currentPlan_.useViewportAsNormalElement ? "true" : "false"},
            {"d2d_frame_compositor_full_frames", std::to_string(stats_.fullFrameRedrawCount)},
            {"d2d_frame_compositor_partial_rejects", std::to_string(stats_.partialPaintRejectedCount)},
            {"d2d_frame_compositor_presents", std::to_string(stats_.presentCount)},
            {"d2d_frame_compositor_present_failures", std::to_string(stats_.presentFailureCount)},
            {"d2d_frame_compositor_reason", currentPlan_.reason}
        };
    }

    AceD2DFramePlan AceD2DFlipFrameCompositor::BuildPlan(const AceD2DFrameInput& input) const
    {
        AceD2DFramePlan plan{};
        plan.presentMode = input.hasDxgiSwapChain ? AceD2DPresentMode::FullFrameFlip : AceD2DPresentMode::Disabled;
        plan.paintRect = makeUiRect(0.0f, 0.0f, static_cast<float>(std::max(1u, input.width)), static_cast<float>(std::max(1u, input.height)));
        plan.fullFrameRedraw = true;
        plan.disableFastPartialViewportPaint = true;
        plan.clearTarget = true;
        plan.useViewportAsNormalElement = true;
        plan.allowDirtyRects = false;
        plan.allowRetainedContents = false;
        plan.drawLastGoodViewport = true;
        return plan;
    }

    std::string AceD2DFlipFrameCompositor::BuildReason(const AceD2DFrameInput& input, const AceD2DFramePlan& plan) const
    {
        std::ostringstream oss;
        oss << "flip_full_frame_no_retained_contents";
        if (input.viewport3DActive) { oss << ";viewport3d"; }
        if (input.environmentOpen) { oss << ";environment"; }
        if (input.liveResize) { oss << ";live_resize"; }
        if (input.diagnosticsVisible) { oss << ";diagnostics"; }
        if (input.overlayVisible) { oss << ";overlay"; }
        oss << ";dirty=" << RectForStats(input.dirtyRect)
            << ";paint=" << RectForStats(plan.paintRect)
            << ";resize_epoch=" << resizeEpoch_
            << ";viewport_epoch=" << input.viewportResourceEpoch
            << ";invalidation=" << input.invalidationSerial;
        return oss.str();
    }

    void AceD2DFlipFrameCompositor::PushHistory(const AceD2DFramePlan& plan)
    {
        history_.push_back(plan);
        while (history_.size() > 120)
        {
            history_.pop_front();
        }
    }

    const char* AceD2DPresentModeName(AceD2DPresentMode mode)
    {
        switch (mode)
        {
        case AceD2DPresentMode::FullFrameFlip: return "full_frame_flip";
        case AceD2DPresentMode::DirtyRectPresent1: return "dirty_rect_present1";
        case AceD2DPresentMode::LegacyRetained: return "legacy_retained";
        case AceD2DPresentMode::Disabled: return "disabled";
        case AceD2DPresentMode::Unknown:
        default: return "unknown";
        }
    }
}
