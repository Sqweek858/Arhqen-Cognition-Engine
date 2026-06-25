#pragma once

#include "ArhqenCognitionEngine/Aquarium/AceAqActions.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqBody.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqObservation.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqWorldModel.h"

#include <string>
#include <vector>

namespace ace::aquarium
{
    struct AceAqPlannerConfig
    {
        double betaInformationGain = 0.30;
        double gammaRisk = 1.35;
        double stepCost = 0.01;
    };

    struct AceAqActionEvaluation
    {
        AceAqAction action = AceAqAction::Wait;
        double score = 0.0;
        double pragmaticValue = 0.0;
        double informationGain = 0.0;
        double risk = 0.0;
        double confidence = 0.0;
        double uncertainty = 1.0;
        AceAqBodyDelta expectedDelta{};
        std::string reason;
    };

    struct AceAqDecisionTrace
    {
        AceAqAction chosenAction = AceAqAction::Wait;
        std::vector<AceAqActionEvaluation> evaluations;
        std::string reason;
    };

    class AceAqSafeCuriosityPlanner
    {
    public:
        explicit AceAqSafeCuriosityPlanner(AceAqPlannerConfig config = {});
        AceAqDecisionTrace ChooseAction(
            const AceAqBodyState& body,
            const AceAqObservation& observation,
            const AceAqTableWorldModel& worldModel
        ) const;

    private:
        double EstimateRisk(AceAqAction action, const AceAqObservation& observation, const AceAqPrediction& prediction, const AceAqBodyState& body) const;

        AceAqPlannerConfig config_;
    };

    std::vector<AceAqAction> AllAceAqActions();
    std::string ToJsonLikeString(const AceAqActionEvaluation& evaluation);
    std::string ToJsonLikeString(const AceAqDecisionTrace& trace);
}
