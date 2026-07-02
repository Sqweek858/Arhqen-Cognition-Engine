#pragma once

#include "ArhqenCognitionEngine/Ui/D2D/D2DUiTypes.h"
#include "ArhqenCognitionEngine/Ui/Slate/AceSlateFramePrimitives.h"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace am::ui::slate
{
    enum class AceSlateInvalidationReason : std::uint16_t
    {
        None = 0,
        FullFrameFlipSwapChain = 1 << 0,
        ViewportTextureChanged = 1 << 1,
        ResizeEpochChanged = 1 << 2,
        OverlayVisibilityChanged = 1 << 3,
        LiveResize = 1 << 4,
        LayoutChanged = 1 << 5,
        AnimationTick = 1 << 6,
        TextSelection = 1 << 7,
        ForceDebug = 1 << 8
    };

    inline AceSlateInvalidationReason operator|(AceSlateInvalidationReason a, AceSlateInvalidationReason b)
    {
        return static_cast<AceSlateInvalidationReason>(static_cast<std::uint16_t>(a) | static_cast<std::uint16_t>(b));
    }

    inline AceSlateInvalidationReason& operator|=(AceSlateInvalidationReason& a, AceSlateInvalidationReason b)
    {
        a = a | b;
        return a;
    }

    struct AceSlateInvalidationInput
    {
        UiRect windowRect{};
        UiRect osDirtyRect{};
        UiRect viewportRect{};
        bool flipSwapChain = false;
        bool deviceContextTarget = false;
        bool viewportActive = false;
        bool liveResize = false;
        bool overlayVisible = false;
        bool animationActive = false;
        bool textSelectionActive = false;
        bool diagnosticsVisible = false;
        bool retainedPartialPaintSafe = false;
        std::uint64_t invalidationSerial = 0;
        std::uint64_t resizeEpoch = 0;
        std::uint64_t viewportResourceEpoch = 0;
        std::uint64_t frameNumber = 0;
    };

    struct AceSlateInvalidationPlan
    {
        bool fullFrame = true;
        bool rejectPartialPaint = true;
        bool allowRetainedContents = false;
        bool allowDirtyRectPresent = false;
        bool forceViewportRedraw = true;
        UiRect paintRect{};
        UiRect originalDirtyRect{};
        AceSlateInvalidationReason reasons = AceSlateInvalidationReason::None;
        std::uint64_t resizeEpoch = 0;
        std::uint64_t viewportEpoch = 0;
        std::uint64_t invalidationSerial = 0;
        bool overlayVisible = false;
        bool viewportActive = false;
        std::string diagnostics;
    };

    struct AceSlateInvalidationHistoryEntry
    {
        std::uint64_t frameNumber = 0;
        UiRect paintRect{};
        UiRect dirtyRect{};
        bool fullFrame = true;
        AceSlateInvalidationReason reasons = AceSlateInvalidationReason::None;
    };

    class AceSlateInvalidationPlanner
    {
    public:
        void Reset();
        AceSlateInvalidationPlan BuildPlan(const AceSlateInvalidationInput& input);
        void CommitPresentedPlan(const AceSlateInvalidationPlan& plan, HRESULT presentHr);
        const std::vector<AceSlateInvalidationHistoryEntry>& History() const { return history_; }
        std::string Diagnostics() const;
        std::wstring WideDiagnostics() const;
        std::uint64_t PartialPaintRejectedCount() const { return partialPaintRejectedCount_; }
        std::uint64_t FullFramePlanCount() const { return fullFramePlanCount_; }
    private:
        bool HasSerialChanged(const AceSlateInvalidationInput& input) const;
        bool HasResizeEpochChanged(const AceSlateInvalidationInput& input) const;
        bool HasViewportEpochChanged(const AceSlateInvalidationInput& input) const;
        UiRect ClampRectToWindow(UiRect rect, UiRect window) const;
        std::string BuildReasonText(AceSlateInvalidationReason reasons) const;
        std::uint64_t lastInvalidationSerial_ = 0;
        std::uint64_t lastResizeEpoch_ = 0;
        std::uint64_t lastViewportEpoch_ = 0;
        bool lastOverlayVisible_ = false;
        bool lastViewportActive_ = false;
        HRESULT lastPresentHr_ = S_OK;
        std::uint64_t fullFramePlanCount_ = 0;
        std::uint64_t partialPaintRejectedCount_ = 0;
        std::vector<AceSlateInvalidationHistoryEntry> history_{};
    };

    const char* AceSlateInvalidationReasonName(AceSlateInvalidationReason reason);
}
