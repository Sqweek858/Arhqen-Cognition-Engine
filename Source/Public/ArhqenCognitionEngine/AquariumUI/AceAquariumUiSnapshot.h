#pragma once

#include "ArhqenCognitionEngine/Aquarium/AceAqBody.h"

#include <string>
#include <vector>

namespace ace::aquarium_ui
{
    struct AceAquariumUiSnapshot
    {
        int step = 0;
        std::string scenarioName;
        std::string plannerName;
        bool running = false;

        ace::aquarium::AceAqBodyState body{};
        double homeostaticError = 0.0;

        std::string agentPositionText;
        std::string agentDirectionText;

        std::string lastActionText;
        std::string lastResultText;

        std::vector<std::string> scenarioNames;
        std::vector<std::string> plannerNames;

        std::vector<std::string> observationLines;
        std::vector<std::string> decisionTraceLines;
        std::vector<std::string> counterfactualTraceLines;
        std::vector<std::string> protoConceptLines;
        std::vector<std::string> selfModelLines;
        std::vector<std::string> delayedEffectLines;
        std::vector<std::string> worldEventLines;
        std::vector<std::string> metricLines;
        std::vector<std::string> logLines;

        bool debugTruthEnabled = false;
        std::vector<std::string> debugTruthLines;
    };

    bool ContainsAquariumTruthLabel(const std::string& text);
    bool SnapshotAgentFacingTextContainsTruthLabels(const AceAquariumUiSnapshot& snapshot);
}
