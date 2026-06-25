#pragma once

#include "ArhqenCognitionEngine/Aquarium/AceAqCounterfactual.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqMetrics.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqScenario.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ace::aquarium
{
    struct AceAqExperimentConfig
    {
        std::string scenarioName = "water_front";
        std::string plannerName = "safe";
        std::uint32_t seed = 0;
        int maxSteps = 20;
    };

    struct AceAqExperimentResult
    {
        AceAqExperimentConfig config{};
        AceAqRunMetrics metrics{};
        int stepsRun = 0;
        bool terminated = false;
        std::string agentFacingSummary;
        std::string debugTruth;
    };

    struct AceAqExperimentRunSummary
    {
        std::vector<AceAqExperimentResult> results;
        AceAqRunMetrics aggregateMetrics{};
    };

    class AceAqExperimentHarness
    {
    public:
        explicit AceAqExperimentHarness(AceAqScenarioRegistry registry = DefaultAceAqScenarioRegistry());
        AceAqExperimentResult Run(const AceAqExperimentConfig& config) const;
        AceAqExperimentRunSummary RunMany(const std::vector<AceAqExperimentConfig>& configs) const;

    private:
        AceAqAction ChooseRandom(std::uint32_t& state) const;
        AceAqScenarioRegistry registry_;
    };

    std::string ToJsonLikeString(const AceAqExperimentResult& result);
    std::string ToJsonLikeString(const AceAqExperimentRunSummary& summary);
}
