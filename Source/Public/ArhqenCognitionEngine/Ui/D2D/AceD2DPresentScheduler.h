#pragma once

#include "ArhqenCognitionEngine/Ui/D2D/D2DUiTypes.h"
#include "ArhqenCognitionEngine/Ui/Slate/AceSlateFramePrimitives.h"

#include <cstdint>
#include <deque>
#include <string>

namespace am::ui
{
    enum class AceD2DPresentSchedulerMode : std::uint8_t
    {
        FullFrameFlipSequential,
        FullFrameFlipDiscard,
        DirtyRectPresent1,
        LegacyRetainedContentsForbidden
    };

    struct AceD2DPresentInput
    {
        bool hasSwapChain = false;
        bool fullFrameRedraw = true;
        bool liveResize = false;
        bool viewportActive = false;
        bool deviceLost = false;
        std::uint64_t frameNumber = 0;
        std::uint64_t resizeEpoch = 0;
        UiRect frameRect{};
        UiRect requestedDirtyRect{};
    };

    struct AceD2DPresentPlan
    {
        AceD2DPresentSchedulerMode mode = AceD2DPresentSchedulerMode::FullFrameFlipSequential;
        bool callPresent = true;
        bool usePresent1DirtyRects = false;
        bool waitForVsync = true;
        bool discardRetainedContentsAssumption = true;
        bool allowTearing = false;
        bool resizeSafeFrame = false;
        UINT syncInterval = 1;
        UINT flags = 0;
        UiRect presentRect{};
        std::string reason;
    };

    struct AceD2DPresentStats
    {
        std::uint64_t plans = 0;
        std::uint64_t presents = 0;
        std::uint64_t presentFailures = 0;
        std::uint64_t resizeSafeFrames = 0;
        std::uint64_t dirtyRectRejected = 0;
        std::uint64_t retainedContentRejected = 0;
        std::uint64_t deviceLostEvents = 0;
        double lastPresentMs = 0.0;
        double maxPresentMs = 0.0;
        double averagePresentMs = 0.0;
        HRESULT lastPresentHr = S_OK;
        std::string lastReason;
    };

    class AceD2DPresentScheduler
    {
    public:
        void Reset();
        AceD2DPresentPlan BuildPlan(const AceD2DPresentInput& input);
        void RecordPresent(const AceD2DPresentPlan& plan, HRESULT hr, double presentMs);
        const AceD2DPresentStats& Stats() const { return stats_; }
        std::string Diagnostics() const;
        std::wstring WideDiagnostics() const;
    private:
        void PushPresentMs(double valueMs);
        AceD2DPresentStats stats_{};
        std::uint64_t lastResizeEpoch_ = 0;
        std::deque<double> recentPresentMs_{};
    };

    const char* AceD2DPresentSchedulerModeName(AceD2DPresentSchedulerMode mode);
}
