#pragma once

#include "ArhqenCognitionEngine/Aquarium/AceAqBody.h"

#include <string>
#include <vector>

namespace ace::aquarium
{
    struct AceAqDelayedEffectConfig
    {
        bool enabled = true;
    };

    struct AceAqDelayedEffect
    {
        int id = 0;
        int createdStep = 0;
        int dueStep = 0;
        std::string source;
        AceAqBodyDelta delta{};
        bool external = false;
    };

    struct AceAqDelayedEffectApplication
    {
        int effectId = 0;
        int stepApplied = 0;
        std::string source;
        AceAqBodyDelta appliedDelta{};
        bool external = false;
    };

    class AceAqDelayedEffectQueue
    {
    public:
        void Clear();
        AceAqDelayedEffect Schedule(int currentStep, int delaySteps, std::string source, AceAqBodyDelta delta, bool external = false);
        std::vector<AceAqDelayedEffectApplication> TickAndCollectDue(int currentStep);
        int PendingCount() const;
        const std::vector<AceAqDelayedEffect>& Pending() const;
        std::string ToJsonLikeString() const;

    private:
        int nextId_ = 1;
        std::vector<AceAqDelayedEffect> pending_;
    };

    std::string ToJsonLikeString(const AceAqDelayedEffect& effect);
    std::string ToJsonLikeString(const AceAqDelayedEffectApplication& application);
    std::string ToJsonLikeString(const std::vector<AceAqDelayedEffect>& effects);
    std::string ToJsonLikeString(const std::vector<AceAqDelayedEffectApplication>& applications);
}
