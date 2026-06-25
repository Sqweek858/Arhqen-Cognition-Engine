#include "ArhqenCognitionEngine/Aquarium/AceAqParity.h"

#include "ArhqenCognitionEngine/Aquarium/AceAqCounterfactual.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqExperiment.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqJson.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqMeaningTests.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqProtoConcept.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqRandomization.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqSensorNoise.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqSelfModel.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqSymbol.h"

#include <sstream>

namespace ace::aquarium
{
    namespace
    {
        AceAqParityCheck Check(std::string milestone, bool passed, std::string note)
        {
            return {std::move(milestone), passed, std::move(note)};
        }
    }

    std::vector<AceAqParityCheck> RunAceAqM0M15ParityChecks()
    {
        std::vector<AceAqParityCheck> checks;
        auto registry = DefaultAceAqScenarioRegistry();

        AceAqBodyState body;
        checks.push_back(Check("m0_body", HomeostaticError(body) == 0.0, "body/homeostasis available"));

        auto water = registry.Build("water_front", 1);
        checks.push_back(Check("m1_observation", !ObservationJsonContainsWorldTruthLabels(ToJsonLikeString(water.environment.GetObservation())), "observation privacy"));

        water.environment.Step(AceAqAction::ConsumeFront);
        checks.push_back(Check("m2_episodes", water.environment.GetEpisodeMemory().Size() == 1, "episodes recorded"));

        AceAqTableWorldModel model;
        model.LearnFromEpisode(water.environment.GetEpisodeMemory().Episodes().back());
        checks.push_back(Check("m3_world_model", model.EntryCount() > 0, "world model learned"));

        AceAqSafeCuriosityPlanner safe;
        checks.push_back(Check("m4_safe_planner", safe.ChooseAction(water.environment.GetBodyState(), water.environment.GetObservation(), model).evaluations.size() == 7, "safe planner"));

        AceAqProtoConceptMiner miner;
        checks.push_back(Check("m5_proto_concepts", !miner.Mine(water.environment.GetEpisodeMemory()).empty(), "proto concepts"));

        AceAqCounterfactualPlanner cf;
        checks.push_back(Check("m6_counterfactual", !cf.ChoosePlan(water.environment.GetBodyState(), water.environment.GetObservation(), model).candidatePlans.empty(), "counterfactual"));

        AceAqSymbolTable symbols;
        symbols.BindSymbolToConcept("x", 1);
        checks.push_back(Check("m7_symbols", !symbols.ActivateSymbol("X").empty(), "symbols"));

        AceAqSelfModel self;
        self.ObserveEpisode(water.environment.GetEpisodeMemory().Episodes().back());
        checks.push_back(Check("m8_self_model", self.State().totalEpisodes == 1, "self model"));

        AceAqMeaningTestRunner meaning(registry);
        checks.push_back(Check("m9_meaning_tests", !meaning.RunAll().empty(), "meaning tests"));

        AceAqExperimentHarness harness(registry);
        checks.push_back(Check("m10_experiment_harness", harness.Run({"water_front", "safe", 1, 2}).stepsRun > 0, "experiment harness"));

        checks.push_back(Check("m11_scenarios", registry.HasScenario("basic_wall") && registry.HasScenario("poison_food_front"), "scenarios"));

        AceAqObjectPropertyRandomizer randomizer({true, 1, 10, 0.1, 0.1, true, true});
        checks.push_back(Check("m12_randomization", randomizer.RandomizeObservation(registry.Build("water_front", 1).environment.GetObservation()).front.colorRgb.r >= 0, "randomization"));

        AceAqSensorNoiseModel noise({true, 1, 10, 0.1, 0.1, 0.0, true});
        checks.push_back(Check("m13_sensor_noise", noise.ApplyNoise(registry.Build("water_front", 1).environment.GetObservation()).front.colorRgb.r >= 0, "sensor noise"));

        auto poison = registry.Build("poison_food_front", 1);
        poison.environment.Step(AceAqAction::ConsumeFront);
        checks.push_back(Check("m14_delayed_effects", poison.environment.DelayedEffects().PendingCount() > 0, "delayed effects"));

        auto dyn = registry.Build("moving_hazard", 1);
        dyn.environment.Step(AceAqAction::Wait);
        checks.push_back(Check("m15_dynamic_world", !dyn.environment.GetEpisodeMemory().Episodes().back().externalWorldEvents.empty(), "dynamic world"));

        auto integrated = harness.Run({"delayed_poison_damage", "random", 1, 5});
        checks.push_back(Check("m0_m15_integrated_run", integrated.metrics.snapshot.appliedDelayedEffectCount > 0, "integrated run"));

        return checks;
    }

    std::string ToJsonLikeString(const AceAqParityCheck& check)
    {
        return "{" +
            std::string("\"milestone\":") + JsonString(check.milestone) + "," +
            "\"passed\":" + JsonBool(check.passed) + "," +
            "\"note\":" + JsonString(check.note) +
            "}";
    }

    std::string ToJsonLikeString(const std::vector<AceAqParityCheck>& checks)
    {
        std::ostringstream out;
        out << "[";
        for (std::size_t i = 0; i < checks.size(); ++i)
        {
            if (i > 0) out << ",";
            out << ToJsonLikeString(checks[i]);
        }
        out << "]";
        return out.str();
    }
}
