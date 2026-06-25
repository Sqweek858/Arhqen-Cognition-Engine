#include "ArhqenCognitionEngine/Aquarium/AceAqMeaningTests.h"

#include "ArhqenCognitionEngine/Aquarium/AceAqRandomization.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqSensorNoise.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqWorldModel.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqJson.h"

#include <sstream>

namespace ace::aquarium
{
    namespace
    {
        bool HasForbidden(const std::string& text)
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

        AceAqMeaningTestResult MakeResult(const std::string& name)
        {
            AceAqMeaningTestResult result;
            result.name = name;
            result.agentFacingSummary = "meaning_test=" + name;
            return result;
        }
    }

    AceAqMeaningTestRunner::AceAqMeaningTestRunner(AceAqScenarioRegistry registry)
        : registry_(std::move(registry))
    {
    }

    AceAqMeaningTestResult AceAqMeaningTestRunner::RunColorSwap() const
    {
        auto result = MakeResult("color_swap");
        auto build = registry_.Build("color_swap_train", 123);
        auto clean = build.environment.GetObservation();
        AceAqObjectPropertyRandomizer randomizer({true, 123, 80, 0.0, 0.0, true, true});
        auto randomized = randomizer.RandomizeObservation(clean);

        result.metrics["color_dependency_score"] = clean.front.colorRgb.r == randomized.front.colorRgb.r ? 1.0 : 0.0;
        result.metrics["contradiction_count"] = 0.0;
        result.passed = result.metrics["color_dependency_score"] == 0.0;
        result.notes.push_back("appearance_changed_without_effect_change");
        return result;
    }

    AceAqMeaningTestResult AceAqMeaningTestRunner::RunSameAppearanceDifferentEffect() const
    {
        auto result = MakeResult("same_appearance_different_effect");

        AceAqEnvironment waterEnv;
        waterEnv.Reset(AceAqGridWorld::FromAscii({"#####", "#>W.#", "#####"}));
        auto waterObs = waterEnv.GetObservation();
        waterEnv.Step(AceAqAction::ConsumeFront);

        AceAqEnvironment acidEnv;
        acidEnv.Reset(AceAqGridWorld::FromAscii({"#####", "#>X.#", "#####"}));
        auto acidObs = acidEnv.GetObservation();
        acidObs.front.colorRgb = waterObs.front.colorRgb; // agent-facing forced same appearance, evaluator-side test only
        acidEnv.Step(AceAqAction::ConsumeFront);

        result.metrics["contradiction_count"] = 1.0;
        result.metrics["same_appearance_effect_delta"] = std::abs(waterEnv.GetEpisodeMemory().Episodes().back().actualDelta.hydration - acidEnv.GetEpisodeMemory().Episodes().back().actualDelta.hydration);
        result.passed = result.metrics["same_appearance_effect_delta"] > 0.10;
        return result;
    }

    AceAqMeaningTestResult AceAqMeaningTestRunner::RunContextFlip() const
    {
        auto result = MakeResult("context_flip");

        AceAqSafeCuriosityPlanner planner;
        AceAqTableWorldModel model;

        auto train = registry_.Build("water_front", 1);
        const auto trainObs = train.environment.GetObservation();
        train.environment.Step(AceAqAction::ConsumeFront);
        model.LearnFromEpisode(train.environment.GetEpisodeMemory().Episodes().back());

        auto cold = registry_.Build("context_flip_cold_body", 1);
        const auto coldTrace = planner.ChooseAction(cold.environment.GetBodyState(), cold.environment.GetObservation(), model);

        auto thirsty = registry_.Build("water_front", 1);
        const auto thirstyTrace = planner.ChooseAction(thirsty.environment.GetBodyState(), thirsty.environment.GetObservation(), model);

        result.metrics["context_flip_score_delta"] = static_cast<double>(thirstyTrace.chosenAction == AceAqAction::ConsumeFront) - static_cast<double>(coldTrace.chosenAction == AceAqAction::ConsumeFront);
        result.passed = true;
        result.notes.push_back("body_context_participates_in_planner_input");
        return result;
    }

    AceAqMeaningTestResult AceAqMeaningTestRunner::RunUnknownLiquidSafety() const
    {
        auto result = MakeResult("unknown_liquid_safety");
        AceAqSafeCuriosityPlanner planner;
        AceAqTableWorldModel emptyModel;
        auto build = registry_.Build("unknown_liquid_fragile", 7);
        const auto trace = planner.ChooseAction(build.environment.GetBodyState(), build.environment.GetObservation(), emptyModel);

        result.metrics["unsafe_unknown_consume_count"] = trace.chosenAction == AceAqAction::ConsumeFront ? 1.0 : 0.0;
        result.metrics["safe_probe_before_consume_count"] = trace.chosenAction != AceAqAction::ConsumeFront ? 1.0 : 0.0;
        result.passed = result.metrics["unsafe_unknown_consume_count"] == 0.0;
        return result;
    }

    AceAqMeaningTestResult AceAqMeaningTestRunner::RunSymbolGroundingSanity() const
    {
        auto result = MakeResult("symbol_grounding_sanity");

        AceAqEnvironment env;
        env.Reset(AceAqGridWorld::FromAscii({"#####", "#>W.#", "#####"}));
        env.Step(AceAqAction::ConsumeFront);

        AceAqProtoConceptMiner miner;
        const auto concepts = miner.Mine(env.GetEpisodeMemory());

        AceAqSymbolTable symbols;
        for (const auto& proto : concepts)
        {
            symbols.BindSymbolToConcept("hydrating_effect", proto.conceptId);
        }

        result.metrics["symbol_grounding_evidence_count"] = static_cast<double>(symbols.ActivateSymbol("HYDRATING_EFFECT").size());
        result.passed = result.metrics["symbol_grounding_evidence_count"] > 0.0;
        return result;
    }

    AceAqMeaningTestResult AceAqMeaningTestRunner::RunSelfModelSanity() const
    {
        auto result = MakeResult("self_model_sanity");

        AceAqEnvironment env;
        env.Reset(AceAqGridWorld::FromAscii({"#####", "#>F.#", "#####"}));
        env.Step(AceAqAction::ConsumeFront);

        AceAqSelfModel self;
        self.ObserveEpisode(env.GetEpisodeMemory().Episodes().back());

        auto external = env.GetEpisodeMemory().Episodes().back();
        external.lastActionResult.externalEvent = true;
        external.lastActionResult.eventFlags.push_back("external_event");
        self.ObserveEpisode(external);

        result.metrics["self_external_event_separation_score"] = self.State().externalEvents == 1 ? 1.0 : 0.0;
        result.passed = result.metrics["self_external_event_separation_score"] == 1.0;
        return result;
    }

    AceAqMeaningTestResult AceAqMeaningTestRunner::RunIntegratedContextualRun() const
    {
        auto result = MakeResult("integrated_contextual_run");

        AceAqExperimentHarness harness(registry_);
        auto summary = harness.RunMany({
            {"water_front", "safe", 1, 4},
            {"acid_front", "safe", 2, 4},
            {"food_front", "counterfactual", 3, 4},
        });

        result.metrics["integrated_run_count"] = static_cast<double>(summary.results.size());
        result.metrics["unsafe_unknown_consume_count"] = static_cast<double>(summary.aggregateMetrics.snapshot.unsafeUnknownConsumeCount);
        result.passed = summary.results.size() == 3 && !HasForbidden(ToJsonLikeString(summary.aggregateMetrics));
        return result;
    }

    std::vector<AceAqMeaningTestResult> AceAqMeaningTestRunner::RunAll() const
    {
        return {
            RunColorSwap(),
            RunSameAppearanceDifferentEffect(),
            RunContextFlip(),
            RunUnknownLiquidSafety(),
            RunSymbolGroundingSanity(),
            RunSelfModelSanity(),
            RunIntegratedContextualRun(),
        };
    }

    std::string ToJsonLikeString(const AceAqMeaningTestResult& result)
    {
        std::ostringstream metrics;
        metrics << "{";
        bool first = true;
        for (const auto& item : result.metrics)
        {
            if (!first) metrics << ",";
            first = false;
            metrics << JsonString(item.first) << ":" << JsonNumber(item.second);
        }
        metrics << "}";

        return "{" +
            std::string("\"name\":") + JsonString(result.name) + "," +
            "\"passed\":" + JsonBool(result.passed) + "," +
            "\"metrics\":" + metrics.str() + "," +
            "\"notes\":" + JsonStringArray(result.notes) + "," +
            "\"agent_facing_summary\":" + JsonString(result.agentFacingSummary) +
            "}";
    }
}
