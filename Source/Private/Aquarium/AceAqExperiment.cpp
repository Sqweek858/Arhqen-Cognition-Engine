#include "ArhqenCognitionEngine/Aquarium/AceAqExperiment.h"

#include "ArhqenCognitionEngine/Aquarium/AceAqJson.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqWorldModel.h"

#include <sstream>

namespace ace::aquarium
{
    namespace
    {
        std::uint32_t NextRandom(std::uint32_t& state)
        {
            state = state * 1664525u + 1013904223u;
            return state;
        }

        bool HasForbiddenAgentFacingLabel(const std::string& text)
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
    }

    AceAqExperimentHarness::AceAqExperimentHarness(AceAqScenarioRegistry registry)
        : registry_(std::move(registry))
    {
    }

    AceAqAction AceAqExperimentHarness::ChooseRandom(std::uint32_t& state) const
    {
        const auto actions = AllAceAqActions();
        return actions[NextRandom(state) % actions.size()];
    }

    AceAqExperimentResult AceAqExperimentHarness::Run(const AceAqExperimentConfig& config) const
    {
        auto build = registry_.Build(config.scenarioName, config.seed);
        auto& environment = build.environment;

        AceAqTableWorldModel worldModel;
        AceAqSafeCuriosityPlanner safePlanner;
        AceAqCounterfactualPlanner counterfactualPlanner;
        std::uint32_t randomState = config.seed == 0 ? 1u : config.seed;

        AceAqExperimentResult result;
        result.config = config;

        for (int step = 0; step < config.maxSteps; ++step)
        {
            const auto observation = environment.GetObservation();
            const auto body = environment.GetBodyState();

            AceAqAction action = AceAqAction::Wait;
            if (config.plannerName == "random")
            {
                action = ChooseRandom(randomState);
            }
            else if (config.plannerName == "counterfactual")
            {
                action = counterfactualPlanner.ChoosePlan(body, observation, worldModel).chosenAction;
            }
            else
            {
                action = safePlanner.ChooseAction(body, observation, worldModel).chosenAction;
            }

            const auto stepResult = environment.Step(action);
            worldModel.LearnFromEpisode(stepResult.episode);
            result.stepsRun += 1;

            if (stepResult.terminated)
            {
                result.terminated = true;
                break;
            }
        }

        result.metrics = ComputeRunMetrics(environment.GetEpisodeMemory());
        result.debugTruth = environment.GetDebugTruthJsonLike();
        result.agentFacingSummary = "scenario=" + config.scenarioName + ";planner=" + config.plannerName + ";steps=" + std::to_string(result.stepsRun);

        if (HasForbiddenAgentFacingLabel(result.agentFacingSummary))
        {
            result.agentFacingSummary = "privacy_violation_removed";
        }

        return result;
    }

    AceAqExperimentRunSummary AceAqExperimentHarness::RunMany(const std::vector<AceAqExperimentConfig>& configs) const
    {
        AceAqExperimentRunSummary summary;
        std::vector<AceAqRunMetrics> metrics;
        for (const auto& config : configs)
        {
            auto result = Run(config);
            metrics.push_back(result.metrics);
            summary.results.push_back(std::move(result));
        }
        summary.aggregateMetrics = AggregateMetricResults(metrics);
        return summary;
    }

    std::string ToJsonLikeString(const AceAqExperimentResult& result)
    {
        return "{" +
            std::string("\"scenario\":") + JsonString(result.config.scenarioName) + "," +
            "\"planner\":" + JsonString(result.config.plannerName) + "," +
            "\"seed\":" + std::to_string(result.config.seed) + "," +
            "\"steps_run\":" + std::to_string(result.stepsRun) + "," +
            "\"terminated\":" + JsonBool(result.terminated) + "," +
            "\"metrics\":" + ToJsonLikeString(result.metrics) + "," +
            "\"agent_facing_summary\":" + JsonString(result.agentFacingSummary) +
            "}";
    }

    std::string ToJsonLikeString(const AceAqExperimentRunSummary& summary)
    {
        std::ostringstream out;
        out << "{\"results\":[";
        for (std::size_t i = 0; i < summary.results.size(); ++i)
        {
            if (i > 0) out << ",";
            out << ToJsonLikeString(summary.results[i]);
        }
        out << "],\"aggregate_metrics\":" << ToJsonLikeString(summary.aggregateMetrics) << "}";
        return out.str();
    }
}
