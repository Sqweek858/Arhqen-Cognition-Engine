#include "ArhqenCognitionEngine/Aquarium/AceAqExperiment.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqMeaningTests.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqRandomization.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqSensorNoise.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

namespace aq = ace::aquarium;

namespace
{
    int g_failures = 0;

    void Pass(const std::string& name)
    {
        std::cout << "PASS|" << name << "\n";
    }

    void Fail(const std::string& name, const std::string& reason)
    {
        ++g_failures;
        std::cout << "FAIL|" << name << "|" << reason << "\n";
    }

    void Check(const std::string& name, bool condition, const std::string& reason)
    {
        if (condition) Pass(name);
        else Fail(name, reason);
    }

    bool HasForbiddenAgentLabel(const std::string& text)
    {
        static constexpr const char* forbidden[] = {"WATER", "ACID", "FOOD", "WALL", "STONE", "ICE", "ObjectKind", "debug_truth"};
        for (const char* item : forbidden)
        {
            if (text.find(item) != std::string::npos)
            {
                return true;
            }
        }
        return false;
    }

    bool HasMeaning(const std::vector<aq::AceAqMeaningTestResult>& results, const std::string& name)
    {
        return std::any_of(results.begin(), results.end(), [&](const auto& result)
        {
            return result.name == name && result.passed;
        });
    }
}

int main()
{
    auto registry = aq::DefaultAceAqScenarioRegistry();

    const std::vector<std::string> requiredScenarios = {
        "basic_wall", "water_front", "acid_front", "food_front", "stone_push",
        "unknown_liquid_fragile", "context_flip_cold_body", "same_appearance_liquids",
        "color_swap_train", "color_swap_test", "randomized_water_front",
        "randomized_acid_front", "cross_color_water_acid", "randomized_food_front"
    };

    bool hasAll = true;
    for (const auto& name : requiredScenarios)
    {
        hasAll = hasAll && registry.HasScenario(name);
    }
    Check("scenario_registry_has_required_scenarios", hasAll, "missing required scenario");

    Check("scenario_build_basic_wall", registry.Build("basic_wall", 1).environment.GetDebugTruthJsonLike().find("\"WALL\"") != std::string::npos, "basic_wall did not build");
    Check("scenario_build_water_front", registry.Build("water_front", 1).environment.GetDebugTruthJsonLike().find("\"WATER\"") != std::string::npos, "water_front did not build");
    Check("scenario_build_acid_front", registry.Build("acid_front", 1).environment.GetDebugTruthJsonLike().find("\"ACID\"") != std::string::npos, "acid_front did not build");
    Check("scenario_expectations_evaluate", !aq::EvaluateScenarioExpectations(registry.Build("water_front", 1)).empty(), "expectations missing");

    aq::AceAqMeaningTestRunner meaning(registry);
    const auto meaningResults = meaning.RunAll();
    Check("meaning_color_swap_runs", HasMeaning(meaningResults, "color_swap"), "color_swap failed");
    Check("meaning_same_appearance_different_effect", HasMeaning(meaningResults, "same_appearance_different_effect"), "same appearance test failed");
    Check("meaning_context_flip_score_delta", HasMeaning(meaningResults, "context_flip"), "context flip failed");
    Check("meaning_unknown_liquid_safety", HasMeaning(meaningResults, "unknown_liquid_safety"), "unknown liquid safety failed");
    Check("meaning_symbol_grounding_sanity", HasMeaning(meaningResults, "symbol_grounding_sanity"), "symbol grounding failed");
    Check("meaning_self_model_sanity", HasMeaning(meaningResults, "self_model_sanity"), "self model sanity failed");
    Check("meaning_integrated_contextual_run", HasMeaning(meaningResults, "integrated_contextual_run"), "integrated contextual run failed");

    aq::AceAqExperimentHarness harness(registry);
    const auto randomResult = harness.Run({"water_front", "random", 11, 5});
    const auto safeResult = harness.Run({"water_front", "safe", 11, 5});
    const auto cfResult = harness.Run({"food_front", "counterfactual", 11, 5});
    const auto many = harness.RunMany({
        {"water_front", "random", 1, 4},
        {"acid_front", "safe", 2, 4},
        {"food_front", "counterfactual", 3, 4},
    });

    Check("experiment_harness_random_baseline", randomResult.stepsRun > 0, "random harness did not step");
    Check("experiment_harness_safe_planner", safeResult.stepsRun > 0, "safe harness did not step");
    Check("experiment_harness_counterfactual_planner", cfResult.stepsRun > 0, "counterfactual harness did not step");
    Check("experiment_run_many_summary", many.results.size() == 3 && many.aggregateMetrics.snapshot.steps > 0, "run_many failed");

    const auto metrics = aq::ComputeRunMetrics(safeResult.metrics.snapshot.steps > 0 ? registry.Build("water_front", 1).environment.GetEpisodeMemory() : registry.Build("basic_wall", 1).environment.GetEpisodeMemory());
    (void)metrics;
    Check("metrics_compute_run_metrics", safeResult.metrics.snapshot.steps > 0, "metrics missing");
    Check("metrics_aggregate_results", many.aggregateMetrics.snapshot.steps >= 3, "aggregate metrics bad");

    auto cleanBuild = registry.Build("water_front", 99);
    const auto cleanObs = cleanBuild.environment.GetObservation();

    aq::AceAqObjectPropertyRandomizer randomizerA({true, 77, 90, 0.20, 0.20, true, true});
    aq::AceAqObjectPropertyRandomizer randomizerB({true, 77, 90, 0.20, 0.20, true, true});
    const auto randA = randomizerA.RandomizeObservation(cleanObs);
    const auto randB = randomizerB.RandomizeObservation(cleanObs);
    Check("randomization_deterministic_seed", randA.front.colorRgb.r == randB.front.colorRgb.r && randA.front.colorRgb.g == randB.front.colorRgb.g, "randomization seed not deterministic");
    Check("randomization_color_jitter_changes_appearance", randA.front.colorRgb.r != cleanObs.front.colorRgb.r || randA.front.colorRgb.g != cleanObs.front.colorRgb.g || randA.front.colorRgb.b != cleanObs.front.colorRgb.b, "appearance did not change");

    auto causalBuild = registry.Build("water_front", 1);
    causalBuild.environment.Step(aq::AceAqAction::ConsumeFront);
    Check("randomization_preserves_causal_effect", causalBuild.environment.GetEpisodeMemory().Episodes().back().actualDelta.hydration > 0.10, "water causal effect not preserved");
    Check("randomization_cross_color_liquids", randA.front.colorRgb.r != cleanObs.front.colorRgb.r || randA.front.colorRgb.b != cleanObs.front.colorRgb.b, "cross color did not alter liquid appearance");

    aq::AceAqSensorNoiseModel noiseA({true, 123, 30, 0.20, 0.20, 0.0, true});
    aq::AceAqSensorNoiseModel noiseB({true, 123, 30, 0.20, 0.20, 0.0, true});
    aq::AceAqNoisyObservationMetadata meta{};
    const auto noisyA = noiseA.ApplyNoise(cleanObs, &meta);
    const auto noisyB = noiseB.ApplyNoise(cleanObs, nullptr);
    Check("sensor_noise_deterministic_seed", noisyA.front.colorRgb.r == noisyB.front.colorRgb.r && noisyA.front.smellSignal == noisyB.front.smellSignal, "sensor noise seed not deterministic");
    Check("sensor_noise_changes_observation_features", noisyA.front.colorRgb.r != cleanObs.front.colorRgb.r || noisyA.front.smellSignal != cleanObs.front.smellSignal, "noise did not change features");
    Check("sensor_noise_clamps_values", noisyA.front.colorRgb.r >= 0 && noisyA.front.colorRgb.r <= 255 && noisyA.front.smellSignal >= -1.0 && noisyA.front.smellSignal <= 1.0, "noise values not clamped");
    Check("sensor_noise_metadata_confidence", meta.colorConfidence < 1.0 && meta.smellConfidence < 1.0, "metadata confidence not set");

    bool privacyOk = !aq::ObservationJsonContainsWorldTruthLabels(aq::ToJsonLikeString(cleanObs)) &&
                     !HasForbiddenAgentLabel(randomResult.agentFacingSummary) &&
                     !HasForbiddenAgentLabel(aq::ToJsonLikeString(randomResult.metrics)) &&
                     !HasForbiddenAgentLabel(aq::ToJsonLikeString(meaningResults.front()));

    Check("privacy_no_objectkind_in_agent_outputs", privacyOk, "agent-facing output leaked truth labels");
    Check("debug_truth_separate", registry.Build("water_front", 1).environment.GetDebugTruthJsonLike().find("\"WATER\"") != std::string::npos, "debug truth missing evaluator label");

    return g_failures == 0 ? 0 : 1;
}
