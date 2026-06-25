#include "ArhqenCognitionEngine/Aquarium/AceAqMetrics.h"

#include "ArhqenCognitionEngine/Aquarium/AceAqJson.h"

namespace ace::aquarium
{
    namespace
    {
        bool HasFlag(const AceAqLastActionResult& result, const std::string& flag)
        {
            for (const auto& item : result.eventFlags)
            {
                if (item == flag) return true;
            }
            return false;
        }
    }

    AceAqRunMetrics ComputeRunMetrics(const AceAqEpisodeMemory& memory)
    {
        AceAqRunMetrics metrics;
        metrics.snapshot.steps = static_cast<int>(memory.Size());

        for (const auto& episode : memory.Episodes())
        {
            if (episode.lastActionResult.consumed) metrics.snapshot.consumedCount += 1;
            if (episode.lastActionResult.blocked) metrics.snapshot.blockedCount += 1;
            if (HasFlag(episode.lastActionResult, "consume_failed")) metrics.snapshot.safeProbeBeforeConsumeCount += 1;
            if (episode.action == AceAqAction::ConsumeFront && episode.lastActionResult.bodyDelta.integrity < -0.01) metrics.snapshot.unsafeUnknownConsumeCount += 1;

            metrics.snapshot.pendingDelayedEffectCount += static_cast<int>(episode.scheduledDelayedEffects.size());
            metrics.snapshot.appliedDelayedEffectCount += static_cast<int>(episode.appliedDelayedEffects.size());
            metrics.snapshot.externalWorldEventCount += static_cast<int>(episode.externalWorldEvents.size());

            for (const auto& event : episode.externalWorldEvents)
            {
                for (const auto& flag : event.eventFlags)
                {
                    if (flag == "external_hazard_damage" || flag == "external_acid_damage") metrics.snapshot.dynamicDamageCount += 1;
                    if (flag == "dynamic_food_decay") metrics.snapshot.foodDecayCount += 1;
                }
            }

            metrics.snapshot.totalRewardLikeDelta += HomeostaticReward(episode.bodyBefore, episode.bodyAfter);
            metrics.snapshot.finalHomeostaticError = HomeostaticError(episode.bodyAfter);
        }

        metrics.notes.push_back("metrics_agent_facing_no_world_truth_labels");
        return metrics;
    }

    AceAqRunMetrics AggregateMetricResults(const std::vector<AceAqRunMetrics>& runs)
    {
        AceAqRunMetrics aggregate;
        if (runs.empty()) return aggregate;

        for (const auto& run : runs)
        {
            aggregate.snapshot.steps += run.snapshot.steps;
            aggregate.snapshot.consumedCount += run.snapshot.consumedCount;
            aggregate.snapshot.blockedCount += run.snapshot.blockedCount;
            aggregate.snapshot.unsafeUnknownConsumeCount += run.snapshot.unsafeUnknownConsumeCount;
            aggregate.snapshot.safeProbeBeforeConsumeCount += run.snapshot.safeProbeBeforeConsumeCount;
            aggregate.snapshot.pendingDelayedEffectCount += run.snapshot.pendingDelayedEffectCount;
            aggregate.snapshot.appliedDelayedEffectCount += run.snapshot.appliedDelayedEffectCount;
            aggregate.snapshot.externalWorldEventCount += run.snapshot.externalWorldEventCount;
            aggregate.snapshot.dynamicDamageCount += run.snapshot.dynamicDamageCount;
            aggregate.snapshot.foodDecayCount += run.snapshot.foodDecayCount;
            aggregate.snapshot.totalRewardLikeDelta += run.snapshot.totalRewardLikeDelta;
            aggregate.snapshot.finalHomeostaticError += run.snapshot.finalHomeostaticError;
        }

        aggregate.snapshot.finalHomeostaticError /= static_cast<double>(runs.size());
        aggregate.notes.push_back("aggregate_metrics");
        return aggregate;
    }

    std::string ToJsonLikeString(const AceAqMetricSnapshot& snapshot)
    {
        return "{" +
            std::string("\"steps\":") + std::to_string(snapshot.steps) + "," +
            "\"consumed_count\":" + std::to_string(snapshot.consumedCount) + "," +
            "\"blocked_count\":" + std::to_string(snapshot.blockedCount) + "," +
            "\"unsafe_unknown_consume_count\":" + std::to_string(snapshot.unsafeUnknownConsumeCount) + "," +
            "\"safe_probe_before_consume_count\":" + std::to_string(snapshot.safeProbeBeforeConsumeCount) + "," +
            "\"pending_delayed_effect_count\":" + std::to_string(snapshot.pendingDelayedEffectCount) + "," +
            "\"applied_delayed_effect_count\":" + std::to_string(snapshot.appliedDelayedEffectCount) + "," +
            "\"external_world_event_count\":" + std::to_string(snapshot.externalWorldEventCount) + "," +
            "\"dynamic_damage_count\":" + std::to_string(snapshot.dynamicDamageCount) + "," +
            "\"food_decay_count\":" + std::to_string(snapshot.foodDecayCount) + "," +
            "\"final_homeostatic_error\":" + JsonNumber(snapshot.finalHomeostaticError) + "," +
            "\"total_reward_like_delta\":" + JsonNumber(snapshot.totalRewardLikeDelta) +
            "}";
    }

    std::string ToJsonLikeString(const AceAqRunMetrics& metrics)
    {
        return "{" +
            std::string("\"snapshot\":") + ToJsonLikeString(metrics.snapshot) + "," +
            "\"notes\":" + JsonStringArray(metrics.notes) +
            "}";
    }
}
