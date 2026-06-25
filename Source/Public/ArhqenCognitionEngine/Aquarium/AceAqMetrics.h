#pragma once

#include "ArhqenCognitionEngine/Aquarium/AceAqEpisode.h"

#include <string>
#include <vector>

namespace ace::aquarium
{
    struct AceAqMetricSnapshot
    {
        int steps = 0;
        int consumedCount = 0;
        int blockedCount = 0;
        int unsafeUnknownConsumeCount = 0;
        int safeProbeBeforeConsumeCount = 0;
        int pendingDelayedEffectCount = 0;
        int appliedDelayedEffectCount = 0;
        int externalWorldEventCount = 0;
        int dynamicDamageCount = 0;
        int foodDecayCount = 0;
        double finalHomeostaticError = 0.0;
        double totalRewardLikeDelta = 0.0;
    };

    struct AceAqRunMetrics
    {
        AceAqMetricSnapshot snapshot{};
        std::vector<std::string> notes;
    };

    AceAqRunMetrics ComputeRunMetrics(const AceAqEpisodeMemory& memory);
    AceAqRunMetrics AggregateMetricResults(const std::vector<AceAqRunMetrics>& runs);

    std::string ToJsonLikeString(const AceAqMetricSnapshot& snapshot);
    std::string ToJsonLikeString(const AceAqRunMetrics& metrics);
}
