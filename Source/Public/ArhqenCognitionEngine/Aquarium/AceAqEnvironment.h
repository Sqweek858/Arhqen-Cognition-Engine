#pragma once

#include "ArhqenCognitionEngine/Aquarium/AceAqActions.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqBody.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqDynamicWorld.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqEpisode.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqGrid.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqObservation.h"

#include <filesystem>
#include <string>

namespace ace::aquarium
{
    struct AceAqStepResult
    {
        AceAqObservation observation{};
        double reward = 0.0;
        bool terminated = false;
        bool truncated = false;
        AceAqLastActionResult lastActionResult{};
        AceAqEpisode episode{};
    };

    class AceAqEnvironment
    {
    public:
        AceAqEnvironment();

        void Reset();
        void Reset(AceAqGridWorld world, AceAqBodyState body = {});
        AceAqStepResult Step(AceAqAction action);

        AceAqObservation GetObservation() const;
        AceAqBodyState GetBodyState() const;
        std::string GetDebugTruthJsonLike() const;
        const AceAqEpisodeMemory& GetEpisodeMemory() const;

        void EnableEpisodeLogging(std::filesystem::path path);
        void DisableEpisodeLogging();

        void SetDynamicWorldEnabled(bool enabled);
        void SetDynamicWorldConfig(AceAqDynamicWorldConfig config);
        const AceAqDelayedEffectQueue& DelayedEffects() const { return delayedEffects_; }

        AceAqGridWorld& MutableWorld() { return world_; }
        const AceAqGridWorld& World() const { return world_; }

    private:
        AceAqObservation BuildObservation(const std::string& lastReason = {}) const;
        AceAqLastActionResult ExecuteAction(AceAqAction action);
        void AddFlag(AceAqLastActionResult& result, std::string flag) const;
        std::vector<AceAqDelayedEffect> ScheduleDelayedEffectsForConsumedKind(AceAqObjectKind kind);

        AceAqGridWorld world_;
        AceAqBodyState body_;
        AceAqEpisodeMemory memory_;
        AceAqEpisodeLogger logger_;
        AceAqDelayedEffectQueue delayedEffects_;
        AceAqDynamicWorldSystem dynamicWorld_;
        AceAqDynamicWorldConfig dynamicWorldConfig_{};
        int step_ = 0;
        std::uint64_t nextEpisodeId_ = 1;
        std::string lastActionReason_;
    };
}
