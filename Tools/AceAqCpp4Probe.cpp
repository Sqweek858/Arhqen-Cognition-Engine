#include "ArhqenCognitionEngine/Aquarium/AceAqCounterfactual.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqDelayedEffects.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqExperiment.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqMeaningTests.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqParity.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqRandomization.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqSensorNoise.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqSelfModel.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

namespace aq = ace::aquarium;

namespace
{
    int g_failures = 0;

    void Pass(const std::string& name) { std::cout << "PASS|" << name << "\n"; }
    void Fail(const std::string& name, const std::string& reason) { ++g_failures; std::cout << "FAIL|" << name << "|" << reason << "\n"; }
    void Check(const std::string& name, bool ok, const std::string& reason) { ok ? Pass(name) : Fail(name, reason); }

    bool HasForbidden(const std::string& text)
    {
        static constexpr const char* forbidden[] = {"WATER","ACID","FOOD","WALL","STONE","ICE","POISON_FOOD","SLOW_MEDICINE","COLD_LIQUID","MOVING_HAZARD","SPREADING_ACID","ObjectKind","debug_truth"};
        for (const char* item : forbidden) if (text.find(item) != std::string::npos) return true;
        return false;
    }

    bool HasFlag(const aq::AceAqLastActionResult& result, const std::string& flag)
    {
        return std::find(result.eventFlags.begin(), result.eventFlags.end(), flag) != result.eventFlags.end();
    }
}

int main()
{
    aq::AceAqDelayedEffectQueue queue;
    auto effect = queue.Schedule(0, 2, "probe", {0.0, 0.0, -0.25, 0.0}, false);
    Check("delayed_effect_schedule", queue.PendingCount() == 1 && effect.dueStep == 2, "schedule failed");
    Check("delayed_effect_not_applied_early", queue.TickAndCollectDue(1).empty() && queue.PendingCount() == 1, "applied early");
    Check("delayed_effect_applied_on_due_step", queue.TickAndCollectDue(2).size() == 1 && queue.PendingCount() == 0, "not applied on due step");

    auto registry = aq::DefaultAceAqScenarioRegistry();

    auto poison = registry.Build("poison_food_front", 1);
    poison.environment.Step(aq::AceAqAction::ConsumeFront);
    Check("poison_food_schedules_damage", poison.environment.GetEpisodeMemory().Episodes().back().scheduledDelayedEffects.size() == 1, "poison did not schedule");
    auto poisonDue = poison.environment.Step(aq::AceAqAction::Wait);
    Check("poison_food_damage_applies_later", !poisonDue.episode.appliedDelayedEffects.empty() && poisonDue.episode.actualDelta.integrity < -0.10, "poison damage missing");

    auto med = registry.Build("slow_medicine_front", 1);
    med.environment.Step(aq::AceAqAction::ConsumeFront);
    Check("slow_medicine_schedules_heal", med.environment.GetEpisodeMemory().Episodes().back().scheduledDelayedEffects.size() == 1, "medicine did not schedule heal");

    auto cold = registry.Build("cold_liquid_front", 1);
    const auto coldBefore = cold.environment.GetBodyState().temperature;
    auto coldStep = cold.environment.Step(aq::AceAqAction::ConsumeFront);
    Check("cold_liquid_affects_temperature", cold.environment.GetBodyState().temperature < coldBefore, "cold liquid did not reduce temp");
    Check("episode_records_scheduled_delayed_effects", !coldStep.episode.scheduledDelayedEffects.empty(), "scheduled delayed missing in episode");
    Check("episode_records_applied_delayed_effects", !coldStep.episode.appliedDelayedEffects.empty(), "applied delayed missing in episode");

    auto hazard = registry.Build("moving_hazard", 1);
    auto hazardStep = hazard.environment.Step(aq::AceAqAction::Wait);
    Check("dynamic_world_moving_hazard_moves", !hazardStep.episode.externalWorldEvents.empty(), "hazard event missing");

    auto hazardDamage = registry.Build("dynamic_hazard_damage", 1);
    auto dmg = hazardDamage.environment.Step(aq::AceAqAction::Wait);
    Check("dynamic_world_moving_hazard_external_damage", !dmg.episode.externalWorldEvents.empty() && dmg.episode.actualDelta.integrity < -0.05, "hazard damage missing");

    auto spread = registry.Build("spreading_acid", 1);
    spread.environment.Step(aq::AceAqAction::Wait);
    auto spreadStep = spread.environment.Step(aq::AceAqAction::Wait);
    Check("dynamic_world_spreading_acid_spreads", !spreadStep.episode.externalWorldEvents.empty(), "acid did not spread");

    auto decay = registry.Build("food_decay", 1);
    decay.environment.Step(aq::AceAqAction::Wait);
    auto decayStep = decay.environment.Step(aq::AceAqAction::Wait);
    Check("dynamic_world_food_decays_to_poison", !decayStep.episode.externalWorldEvents.empty() && decay.environment.GetDebugTruthJsonLike().find("POISON_FOOD") != std::string::npos, "food did not decay");

    Check("world_events_recorded", !decayStep.episode.externalWorldEvents.empty(), "world events absent");
    Check("episode_records_external_world_events", !dmg.episode.externalWorldEvents.empty(), "episode external events absent");

    aq::AceAqSelfModel selfHazard;
    auto hazardAssessment = selfHazard.ObserveEpisode(dmg.episode);
    Check("self_model_dynamic_hazard_not_self_caused", !hazardAssessment.causedBySelf, "hazard marked self-caused");

    aq::AceAqEnvironment ownEnv;
    ownEnv.Reset(aq::AceAqGridWorld::FromAscii({"#####", "#>F.#", "#####"}));
    auto ownStep = ownEnv.Step(aq::AceAqAction::ConsumeFront);
    aq::AceAqSelfModel selfOwn;
    auto ownAssessment = selfOwn.ObserveEpisode(ownStep.episode);
    Check("self_model_action_effect_still_self_caused", ownAssessment.causedBySelf, "own action not self-caused");

    bool hasM14M15 = registry.HasScenario("poison_food_front") && registry.HasScenario("slow_medicine_front") && registry.HasScenario("cold_liquid_front") &&
                     registry.HasScenario("moving_hazard") && registry.HasScenario("spreading_acid") && registry.HasScenario("food_decay") &&
                     registry.HasScenario("dynamic_hazard_damage") && registry.HasScenario("delayed_poison_damage");
    Check("scenario_registry_has_m14_m15_scenarios", hasM14M15, "missing M14/M15 scenarios");

    aq::AceAqExperimentHarness harness(registry);
    auto delayedRun = harness.Run({"delayed_poison_damage", "random", 1, 5});
    auto dynamicRun = harness.Run({"dynamic_hazard_damage", "safe", 1, 3});
    Check("experiment_harness_runs_delayed_scenario", delayedRun.stepsRun > 0 && delayedRun.metrics.snapshot.appliedDelayedEffectCount > 0, "delayed harness failed");
    Check("experiment_harness_runs_dynamic_scenario", dynamicRun.stepsRun > 0 && dynamicRun.metrics.snapshot.externalWorldEventCount > 0, "dynamic harness failed");
    Check("metrics_delayed_dynamic_counts", delayedRun.metrics.snapshot.appliedDelayedEffectCount > 0 && dynamicRun.metrics.snapshot.dynamicDamageCount > 0, "metrics counters missing");

    Check("privacy_no_objectkind_in_agent_outputs_cpp4", !HasForbidden(delayedRun.agentFacingSummary) && !HasForbidden(aq::ToJsonLikeString(delayedRun.metrics)), "agent-facing output leaked truth");
    Check("debug_truth_separate_cpp4", dynamicRun.debugTruth.find("MOVING_HAZARD") != std::string::npos || hazardDamage.environment.GetDebugTruthJsonLike().find("MOVING_HAZARD") != std::string::npos, "debug truth missing");

    const auto parity = aq::RunAceAqM0M15ParityChecks();
    auto hasParity = [&](const std::string& name)
    {
        return std::any_of(parity.begin(), parity.end(), [&](const auto& check){ return check.milestone == name && check.passed; });
    };

    Check("parity_m0_body", hasParity("m0_body"), "m0");
    Check("parity_m1_observation", hasParity("m1_observation"), "m1");
    Check("parity_m2_episodes", hasParity("m2_episodes"), "m2");
    Check("parity_m3_world_model", hasParity("m3_world_model"), "m3");
    Check("parity_m4_safe_planner", hasParity("m4_safe_planner"), "m4");
    Check("parity_m5_proto_concepts", hasParity("m5_proto_concepts"), "m5");
    Check("parity_m6_counterfactual", hasParity("m6_counterfactual"), "m6");
    Check("parity_m7_symbols", hasParity("m7_symbols"), "m7");
    Check("parity_m8_self_model", hasParity("m8_self_model"), "m8");
    Check("parity_m9_meaning_tests", hasParity("m9_meaning_tests"), "m9");
    Check("parity_m10_experiment_harness", hasParity("m10_experiment_harness"), "m10");
    Check("parity_m11_scenarios", hasParity("m11_scenarios"), "m11");
    Check("parity_m12_randomization", hasParity("m12_randomization"), "m12");
    Check("parity_m13_sensor_noise", hasParity("m13_sensor_noise"), "m13");
    Check("parity_m14_delayed_effects", hasParity("m14_delayed_effects"), "m14");
    Check("parity_m15_dynamic_world", hasParity("m15_dynamic_world"), "m15");
    Check("parity_m0_m15_integrated_run", hasParity("m0_m15_integrated_run"), "integrated");

    return g_failures == 0 ? 0 : 1;
}
