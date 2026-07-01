#pragma once

#include "ArhqenCognitionEngine/Ui/D2D/AceD2DSharedViewportBridgePolicy.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DUiTypes.h"
#include "ArhqenCognitionEngine/Ui/Slate/AceSlateFramePrimitives.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace am::ui
{
    enum class AceD2DViewportBridgeSlotState : std::uint8_t
    {
        Empty,
        CopyInProgress,
        ReadyForDraw,
        Drawing,
        Presented,
        Stale,
        Failed
    };

    struct AceD2DViewportBridgeSlotRuntime
    {
        std::size_t index = 0;
        AceD2DViewportBridgeSlotState state = AceD2DViewportBridgeSlotState::Empty;
        std::uint64_t resourceEpoch = 0;
        std::uint64_t copyFrame = 0;
        std::uint64_t drawFrame = 0;
        std::uint64_t presentFrame = 0;
        std::uint64_t failureCount = 0;
        bool hasBitmap = false;
        bool writerHeld = false;
        bool readerHeld = false;
        std::string lastFailure;
    };

    struct AceD2DViewportBridgeFrameInput
    {
        std::uint64_t frameNumber = 0;
        std::uint64_t resourceEpoch = 0;
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        bool fullFrameRedraw = true;
        bool liveResize = false;
        bool bridgeUsesSharedIntermediate = false;
        bool keyedMutexEnabled = false;
        bool firstFrameMayUseSameSlot = true;
        std::uint32_t slotCount = 2;
    };

    struct AceD2DViewportBridgeFrameDecision
    {
        bool valid = false;
        bool copyThisFrame = true;
        bool drawThisFrame = true;
        bool drawLastGood = true;
        bool allowSameFrameRead = false;
        bool keyedMutex = false;
        bool needsFlushAfterCopy = true;
        bool needsD2DFlushBeforeRelease = false;
        bool firstFrame = false;
        std::size_t writeSlot = 0;
        std::size_t drawSlot = 0;
        std::optional<std::size_t> lastGoodSlot;
        std::uint64_t resourceEpoch = 0;
        std::string reason;
    };

    struct AceD2DViewportBridgeRuntimeStats
    {
        std::uint64_t frames = 0;
        std::uint64_t copiesRequested = 0;
        std::uint64_t copiesCompleted = 0;
        std::uint64_t drawsRequested = 0;
        std::uint64_t drawsCompleted = 0;
        std::uint64_t drawsFromLastGood = 0;
        std::uint64_t sameSlotDraws = 0;
        std::uint64_t skippedDraws = 0;
        std::uint64_t slotReinitializations = 0;
        std::uint64_t failures = 0;
        std::uint64_t resizeEpochs = 0;
        std::uint64_t lastGoodEpoch = 0;
        std::size_t lastWriteSlot = 0;
        std::size_t lastDrawSlot = 0;
        std::string lastDecision;
        std::string lastFailure;
    };

    class AceD2DViewportBridgeRuntime
    {
    public:
        void Reset();
        void ConfigureSlots(std::uint32_t slotCount, std::uint64_t resourceEpoch);
        AceD2DViewportBridgeFrameDecision BeginFrame(const AceD2DViewportBridgeFrameInput& input);
        void MarkCopyStarted(std::size_t slotIndex);
        void MarkCopyCompleted(std::size_t slotIndex);
        void MarkDrawStarted(std::size_t slotIndex);
        void MarkDrawCompleted(std::size_t slotIndex);
        void MarkPresented(std::size_t slotIndex);
        void MarkSlotFailure(std::size_t slotIndex, const std::string& failure);
        void MarkAllStale();
        void OnResizeOrResourceEpoch(std::uint64_t resourceEpoch);
        const AceD2DViewportBridgeRuntimeStats& Stats() const { return stats_; }
        const std::vector<AceD2DViewportBridgeSlotRuntime>& Slots() const { return slots_; }
        std::string Diagnostics() const;
        std::wstring WideDiagnostics() const;
    private:
        std::optional<std::size_t> FindLastGoodSlot(std::uint64_t resourceEpoch) const;
        std::size_t PickWriteSlot(std::uint64_t frameNumber, std::uint64_t resourceEpoch, std::optional<std::size_t> lastGood) const;
        bool SlotValid(std::size_t slotIndex) const;
        AceD2DViewportBridgeSlotRuntime* MutableSlot(std::size_t slotIndex);
        const AceD2DViewportBridgeSlotRuntime* Slot(std::size_t slotIndex) const;
        void EnsureEpoch(std::uint64_t resourceEpoch);
        std::vector<AceD2DViewportBridgeSlotRuntime> slots_{};
        AceD2DViewportBridgeRuntimeStats stats_{};
        std::uint64_t configuredEpoch_ = 0;
        std::size_t writeCursor_ = 0;
        std::optional<std::size_t> lastGoodSlot_{};
    };

    const char* AceD2DViewportBridgeSlotStateName(AceD2DViewportBridgeSlotState state);
}
