#include "ArhqenCognitionEngine/Aquarium/AceAqPlanner.h"

#include "ArhqenCognitionEngine/Aquarium/AceAqJson.h"

#include <algorithm>
#include <sstream>

namespace ace::aquarium
{
    namespace
    {
        AceAqBodyState ApplyExpectedDelta(AceAqBodyState body, const AceAqBodyDelta& delta)
        {
            body.ApplyDelta(delta);
            return body;
        }

        double PredictedIntegrityRisk(const AceAqPrediction& prediction)
        {
            if (!prediction.hasPrediction)
            {
                return 0.0;
            }

            double risk = 0.0;
            if (prediction.predictedDelta.integrity < 0.0)
            {
                risk += -prediction.predictedDelta.integrity * 3.0;
            }
            if (prediction.predictedDelta.temperature < -0.10 || prediction.predictedDelta.temperature > 0.10)
            {
                risk += std::abs(prediction.predictedDelta.temperature);
            }
            return risk;
        }
    }

    std::vector<AceAqAction> AllAceAqActions()
    {
        return {
            AceAqAction::TurnLeft,
            AceAqAction::TurnRight,
            AceAqAction::MoveForward,
            AceAqAction::Wait,
            AceAqAction::TouchFront,
            AceAqAction::ConsumeFront,
            AceAqAction::PushFront,
        };
    }

    AceAqSafeCuriosityPlanner::AceAqSafeCuriosityPlanner(AceAqPlannerConfig config)
        : config_(config)
    {
    }

    double AceAqSafeCuriosityPlanner::EstimateRisk(
        AceAqAction action,
        const AceAqObservation& observation,
        const AceAqPrediction& prediction,
        const AceAqBodyState& body) const
    {
        double risk = PredictedIntegrityRisk(prediction);

        if (action == AceAqAction::MoveForward && observation.front.solidHint)
        {
            risk += 0.35;
        }

        if (action == AceAqAction::ConsumeFront && !prediction.hasPrediction)
        {
            if (observation.front.liquidLikeHint || observation.front.smellSignal > 0.20)
            {
                risk += body.hydration < 0.20 ? 0.35 : 1.00;
            }
            else
            {
                risk += 0.25;
            }
        }

        if (action == AceAqAction::TouchFront && !prediction.hasPrediction && observation.front.smellSignal > 0.20)
        {
            risk += 0.45;
        }

        return risk;
    }

    AceAqDecisionTrace AceAqSafeCuriosityPlanner::ChooseAction(
        const AceAqBodyState& body,
        const AceAqObservation& observation,
        const AceAqTableWorldModel& worldModel) const
    {
        AceAqDecisionTrace trace;
        trace.reason = "score = pragmatic_value + beta * information_gain - gamma * risk - step_cost";

        bool first = true;
        AceAqActionEvaluation best;

        for (const auto action : AllAceAqActions())
        {
            const auto prediction = worldModel.Predict(observation, action);
            const auto expectedBody = ApplyExpectedDelta(body, prediction.predictedDelta);
            const double pragmatic = prediction.hasPrediction ? HomeostaticReward(body, expectedBody) : 0.0;
            const double informationGain = prediction.uncertainty;
            const double risk = EstimateRisk(action, observation, prediction, body);
            const double score = pragmatic +
                                 config_.betaInformationGain * informationGain -
                                 config_.gammaRisk * risk -
                                 config_.stepCost;

            AceAqActionEvaluation evaluation;
            evaluation.action = action;
            evaluation.score = score;
            evaluation.pragmaticValue = pragmatic;
            evaluation.informationGain = informationGain;
            evaluation.risk = risk;
            evaluation.confidence = prediction.confidence;
            evaluation.uncertainty = prediction.uncertainty;
            evaluation.expectedDelta = prediction.predictedDelta;
            evaluation.reason = prediction.hasPrediction ? "prediction_available" : "unknown_transition";

            trace.evaluations.push_back(evaluation);

            if (first || evaluation.score > best.score)
            {
                first = false;
                best = evaluation;
            }
        }

        trace.chosenAction = best.action;
        trace.reason += "; chosen=" + ToString(best.action);
        return trace;
    }

    std::string ToJsonLikeString(const AceAqActionEvaluation& evaluation)
    {
        return "{" +
            std::string("\"action\":") + JsonString(ToString(evaluation.action)) + "," +
            "\"score\":" + JsonNumber(evaluation.score) + "," +
            "\"pragmatic_value\":" + JsonNumber(evaluation.pragmaticValue) + "," +
            "\"information_gain\":" + JsonNumber(evaluation.informationGain) + "," +
            "\"risk\":" + JsonNumber(evaluation.risk) + "," +
            "\"confidence\":" + JsonNumber(evaluation.confidence) + "," +
            "\"uncertainty\":" + JsonNumber(evaluation.uncertainty) + "," +
            "\"expected_delta\":" + ToJsonLikeString(evaluation.expectedDelta) + "," +
            "\"reason\":" + JsonString(evaluation.reason) +
            "}";
    }

    std::string ToJsonLikeString(const AceAqDecisionTrace& trace)
    {
        std::ostringstream out;
        out << "{\"chosen_action\":" << JsonString(ToString(trace.chosenAction)) << ",\"reason\":" << JsonString(trace.reason) << ",\"evaluations\":[";
        for (std::size_t i = 0; i < trace.evaluations.size(); ++i)
        {
            if (i > 0)
            {
                out << ",";
            }
            out << ToJsonLikeString(trace.evaluations[i]);
        }
        out << "]}";
        return out.str();
    }
}
