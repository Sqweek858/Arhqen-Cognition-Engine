#pragma once

#include "ArhqenCognitionEngine/Ui/Slate/AceSlateElementList.h"

#include <chrono>
#include <deque>

namespace am::ui
{
    enum class AceD2DPresentMode : std::uint8_t
    {
        Unknown,
        FullFrameFlip,
        DirtyRectPresent1,
        LegacyRetained,
        Disabled
    };

    struct AceD2DFrameInput
    {
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        UiRect dirtyRect{};
        bool environmentOpen = false;
        bool viewport3DActive = false;
        bool hasDxgiSwapChain = false;
        bool liveResize = false;
        bool diagnosticsVisible = false;
        bool overlayVisible = false;
        std::uint64_t invalidationSerial = 0;
        std::uint64_t viewportResourceEpoch = 0;
    };

    struct AceD2DFramePlan
    {
        AceD2DPresentMode presentMode = AceD2DPresentMode::Unknown;
        UiRect paintRect{};
        bool fullFrameRedraw = true;
        bool disableFastPartialViewportPaint = true;
        bool clearTarget = true;
        bool useViewportAsNormalElement = true;
        bool allowDirtyRects = false;
        bool allowRetainedContents = false;
        bool drawLastGoodViewport = true;
        std::string reason;
    };

    struct AceD2DFrameCompositorStats
    {
        std::uint64_t frameNumber = 0;
        std::uint64_t beginFrameCount = 0;
        std::uint64_t fullFrameRedrawCount = 0;
        std::uint64_t partialPaintRejectedCount = 0;
        std::uint64_t presentCount = 0;
        std::uint64_t presentFailureCount = 0;
        std::uint64_t endDrawFailureCount = 0;
        std::uint64_t liveResizeFullFrameCount = 0;
        std::uint64_t viewportNormalElementFrames = 0;
        std::uint64_t retainedContentsForbiddenFrames = 0;
        HRESULT lastEndDrawHr = S_OK;
        HRESULT lastPresentHr = S_OK;
        double lastFrameMs = 0.0;
        double lastPresentMs = 0.0;
        AceD2DPresentMode lastPresentMode = AceD2DPresentMode::Unknown;
        std::string lastReason;
        std::string lastDiagnostics;
    };

    class AceD2DFlipFrameCompositor
    {
    public:
        AceD2DFlipFrameCompositor() = default;

        AceD2DFramePlan BeginFrame(const AceD2DFrameInput& input);
        void RecordSlateElements(const slate::AceSlateWindowElementList& elements);
        void RecordEndDraw(HRESULT hr);
        void RecordPresent(HRESULT hr, double presentMs);
        void EndFrame();
        void Reset();
        void NotifyResize(std::uint32_t width, std::uint32_t height);

        const AceD2DFramePlan& CurrentPlan() const { return currentPlan_; }
        const AceD2DFrameCompositorStats& Stats() const { return stats_; }
        bool AllowsFastPartialViewportPaint() const { return !currentPlan_.disableFastPartialViewportPaint; }
        bool RequiresFullFrameRedraw() const { return currentPlan_.fullFrameRedraw; }
        bool ShouldClearTarget() const { return currentPlan_.clearTarget; }
        UiRect PaintRect() const { return currentPlan_.paintRect; }
        std::string Diagnostics() const;
        std::wstring WideDiagnostics() const;
        std::vector<std::pair<std::string, std::string>> KeyValues() const;

    private:
        AceD2DFramePlan BuildPlan(const AceD2DFrameInput& input) const;
        std::string BuildReason(const AceD2DFrameInput& input, const AceD2DFramePlan& plan) const;
        void PushHistory(const AceD2DFramePlan& plan);

        AceD2DFrameInput currentInput_{};
        AceD2DFramePlan currentPlan_{};
        AceD2DFrameCompositorStats stats_{};
        slate::AceSlateWindowElementList frameElements_{};
        slate::AceSlateElementBatcher batcher_{};
        std::deque<AceD2DFramePlan> history_;
        std::chrono::steady_clock::time_point frameStart_{};
        std::uint32_t resizeEpoch_ = 0;
    };

    const char* AceD2DPresentModeName(AceD2DPresentMode mode);
}
