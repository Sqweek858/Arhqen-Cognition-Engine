#pragma once

#include "ArhqenCognitionEngine/Aquarium/AceAqActions.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqBody.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqObservation.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqPlanner.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqWorldModel.h"

#include <string>
#include <vector>

namespace ace::aquarium
{
    struct AceAqCounterfactualConfig
    {
        int depth = 2;
        double betaInformationGain = 0.25;
        double gammaRisk = 1.25;
        double stepCost = 0.01;
    };

    struct AceAqImaginedOutcome
    {
        AceAqAction action = AceAqAction::Wait;
        AceAqBodyDelta expectedDelta{};
        double pragmaticValue = 0.0;
        double informationGain = 0.0;
        double risk = 0.0;
        double confidence = 0.0;
        double uncertainty = 1.0;
        double score = 0.0;
        std::vector<std::string> notes;
    };

    struct AceAqActionBranch
    {
        std::vector<AceAqImaginedOutcome> outcomes;
    };

    struct AceAqPlanCandidate
    {
        std::vector<AceAqAction> actions;
        AceAqBodyDelta expectedDelta{};
        double score = 0.0;
        double risk = 0.0;
        double informationGain = 0.0;
        double confidence = 0.0;
        std::vector<std::string> notes;
    };

    struct AceAqCounterfactualTrace
    {
        AceAqAction chosenAction = AceAqAction::Wait;
        std::vector<AceAqAction> chosenPlan;
        std::vector<AceAqPlanCandidate> candidatePlans;
        std::vector<std::string> notes;
    };

    class AceAqCounterfactualPlanner
    {
    public:
        explicit AceAqCounterfactualPlanner(AceAqCounterfactualConfig config = {});
        AceAqCounterfactualTrace ChoosePlan(
            const AceAqBodyState& body,
            const AceAqObservation& observation,
            const AceAqTableWorldModel& worldModel
        ) const;

    private:
        AceAqPlanCandidate BuildCandidate(
            const AceAqBodyState& body,
            const AceAqObservation& observation,
            const AceAqTableWorldModel& worldModel,
            const std::vector<AceAqAction>& actions
        ) const;

        AceAqCounterfactualConfig config_;
    };

    std::string ToJsonLikeString(const AceAqPlanCandidate& candidate);
    std::string ToJsonLikeString(const AceAqCounterfactualTrace& trace);
}
