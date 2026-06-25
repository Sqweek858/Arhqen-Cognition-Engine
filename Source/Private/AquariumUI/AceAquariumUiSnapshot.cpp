#include "ArhqenCognitionEngine/AquariumUI/AceAquariumUiSnapshot.h"

#include <array>

namespace ace::aquarium_ui
{
    bool ContainsAquariumTruthLabel(const std::string& text)
    {
        static constexpr std::array<const char*, 13> forbidden =
        {
            "WATER",
            "ACID",
            "FOOD",
            "WALL",
            "STONE",
            "ICE",
            "POISON_FOOD",
            "SLOW_MEDICINE",
            "COLD_LIQUID",
            "MOVING_HAZARD",
            "SPREADING_ACID",
            "ObjectKind",
            "debug_truth"
        };

        for (const char* label : forbidden)
        {
            if (text.find(label) != std::string::npos)
            {
                return true;
            }
        }

        return false;
    }

    bool SnapshotAgentFacingTextContainsTruthLabels(const AceAquariumUiSnapshot& snapshot)
    {
        auto checkList = [](const std::vector<std::string>& lines)
        {
            for (const auto& line : lines)
            {
                if (ContainsAquariumTruthLabel(line))
                {
                    return true;
                }
            }
            return false;
        };

        return ContainsAquariumTruthLabel(snapshot.scenarioName) ||
               ContainsAquariumTruthLabel(snapshot.plannerName) ||
               ContainsAquariumTruthLabel(snapshot.agentPositionText) ||
               ContainsAquariumTruthLabel(snapshot.agentDirectionText) ||
               ContainsAquariumTruthLabel(snapshot.lastActionText) ||
               ContainsAquariumTruthLabel(snapshot.lastResultText) ||
               checkList(snapshot.observationLines) ||
               checkList(snapshot.decisionTraceLines) ||
               checkList(snapshot.counterfactualTraceLines) ||
               checkList(snapshot.protoConceptLines) ||
               checkList(snapshot.selfModelLines) ||
               checkList(snapshot.delayedEffectLines) ||
               checkList(snapshot.worldEventLines) ||
               checkList(snapshot.metricLines) ||
               checkList(snapshot.logLines);
    }
}
