#pragma once

#include "ArhqenCognitionEngine/Ui/D2D/AceD2DViewportBridgeRuntime.h"
#include "ArhqenCognitionEngine/Ui/D2D/AceD2DViewportTextureCache.h"

#include <cstdint>
#include <string>
#include <vector>

namespace am::ui
{
    enum class AceD2DViewportCopyHazard : std::uint8_t
    {
        None,
        SameSlotReadWrite,
        MissingLastGood,
        ResizeEpochMismatch,
        CopyWithoutFullFrame,
        KeyedMutexStallRisk
    };

    struct AceD2DViewportCopyScheduleInput
    {
        AceD2DViewportBridgeFrameDecision bridgeDecision{};
        AceD2DViewportTextureKey textureKey{};
        bool fullFrame = true;
        bool liveResize = false;
        bool allowOneFrameLatency = true;
        bool keyedMutexEnabled = false;
        std::uint64_t frameNumber = 0;
        std::uint64_t resizeEpoch = 0;
    };

    struct AceD2DViewportCopySchedule
    {
        bool copy = true;
        bool draw = true;
        bool drawPreviousFrame = true;
        bool requiresFlush = true;
        bool requiresD2DFlush = false;
        bool safe = true;
        std::size_t copySlot = 0;
        std::size_t drawSlot = 0;
        AceD2DViewportCopyHazard hazard = AceD2DViewportCopyHazard::None;
        std::string reason;
    };

    struct AceD2DViewportCopySchedulerStats
    {
        std::uint64_t schedules = 0;
        std::uint64_t safeSchedules = 0;
        std::uint64_t hazards = 0;
        std::uint64_t sameSlotAvoided = 0;
        std::uint64_t previousFrameDraws = 0;
        std::uint64_t sameFrameDraws = 0;
        std::uint64_t resizeMismatch = 0;
        std::uint64_t keyedRisk = 0;
        std::string lastReason;
    };

    class AceD2DViewportCopyScheduler
    {
    public:
        void Reset();
        AceD2DViewportCopySchedule BuildSchedule(const AceD2DViewportCopyScheduleInput& input);
        void RecordExecuted(const AceD2DViewportCopySchedule& schedule, bool copyOk, bool drawOk);
        const AceD2DViewportCopySchedulerStats& Stats() const { return stats_; }
        std::string Diagnostics() const;
        std::wstring WideDiagnostics() const;
    private:
        AceD2DViewportCopySchedulerStats stats_{};
        std::uint64_t executedCopies_ = 0;
        std::uint64_t executedDraws_ = 0;
        std::uint64_t failedCopies_ = 0;
        std::uint64_t failedDraws_ = 0;
    };

    const char* AceD2DViewportCopyHazardName(AceD2DViewportCopyHazard hazard);
}
