#include "ArhqenCognitionEngine/Aquarium/AceAqCounterfactual.h"

#include "ArhqenCognitionEngine/Aquarium/AceAqJson.h"

#include <algorithm>
#include <numeric>
#include <sstream>

namespace ace::aquarium
{
    namespace
    {
        AceAqBodyDelta AddDelta(AceAqBodyDelta a, const AceAqBodyDelta& b)
        {
            a.hydration += b.hydration;
            a.nutrition += b.nutrition;
            a.integrity += b.integrity;
            a.temperature += b.temperature;
            return a;
        }

        AceAqBodyState ApplyDelta(AceAqBodyState body, const AceAqBodyDelta& delta)
        {
            body.ApplyDelta(delta);
            return body;
        }

        double EstimateOutcomeRisk(AceAqAction action, const AceAqObservation& observation, const AceAqPrediction& prediction)
        {
            double risk = 0.0;
            if (prediction.hasPrediction && prediction.predictedDelta.integrity < 0.0)
            {
                risk += -prediction.predictedDelta.integrity * 3.0;
            }

            if (!prediction.hasPrediction && action == AceAqAction::ConsumeFront && (observation.front.liquidLikeHint || observation.front.smellSignal > 0.20))
            {
                risk += 1.0;
            }

            if (action == AceAqAction::MoveForward && observation.front.solidHint)
            {
                risk += 0.30;
            }

            return risk;
        }
    }

    AceAqCounterfactualPlanner::AceAqCounterfactualPlanner(AceAqCounterfactualConfig config)
        : config_(config)
    {
    }

    AceAqPlanCandidate AceAqCounterfactualPlanner::BuildCandidate(
        const AceAqBodyState& body,
        const AceAqObservation& observation,
        const AceAqTableWorldModel& worldModel,
        const std::vector<AceAqAction>& actions) const
    {
        AceAqPlanCandidate candidate;
        candidate.actions = actions;

        AceAqBodyState imaginedBody = body;
        double score = 0.0;
        double totalRisk = 0.0;
        double totalInfo = 0.0;
        double totalConfidence = 0.0;

        for (std::size_t depthIndex = 0; depthIndex < actions.size(); ++depthIndex)
        {
            const auto action = actions[depthIndex];
            const auto prediction = worldModel.Predict(observation, action);
            const auto nextBody = ApplyDelta(imaginedBody, prediction.predictedDelta);
            const double pragmatic = prediction.hasPrediction ? HomeostaticReward(imaginedBody, nextBody) : 0.0;
            const double info = prediction.uncertainty;
            const double risk = EstimateOutcomeRisk(action, observation, prediction);
            const double actionScore = pragmatic + config_.betaInformationGain * info - config_.gammaRisk * risk - config_.stepCost;

            score += actionScore;
            totalRisk += risk;
            totalInfo += info;
            totalConfidence += prediction.confidence;
            candidate.expectedDelta = AddDelta(candidate.expectedDelta, prediction.predictedDelta);
            imaginedBody = nextBody;

            if (depthIndex > 0)
            {
                candidate.notes.push_back("depth_2_rollout_approximation");
                candidate.notes.push_back("reused_current_observation_for_depth_2");
            }
        }

        candidate.score = score;
        candidate.risk = totalRisk;
        candidate.informationGain = totalInfo;
        candidate.confidence = actions.empty() ? 0.0 : totalConfidence / static_cast<double>(actions.size());
        return candidate;
    }

    AceAqCounterfactualTrace AceAqCounterfactualPlanner::ChoosePlan(
        const AceAqBodyState& body,
        const AceAqObservation& observation,
        const AceAqTableWorldModel& worldModel) const
    {
        AceAqCounterfactualTrace trace;
        trace.notes.push_back("counterfactual_planning_depth_" + std::to_string(config_.depth));

        for (const auto action : AllAceAqActions())
        {
            trace.candidatePlans.push_back(BuildCandidate(body, observation, worldModel, {action}));
        }

        if (config_.depth >= 2)
        {
            trace.notes.push_back("depth_2_rollout_approximation");
            trace.notes.push_back("reused_current_observation_for_depth_2");

            for (const auto first : AllAceAqActions())
            {
                for (const auto second : AllAceAqActions())
                {
                    trace.candidatePlans.push_back(BuildCandidate(body, observation, worldModel, {first, second}));
                }
            }
        }

        const auto best = std::max_element(
            trace.candidatePlans.begin(),
            trace.candidatePlans.end(),
            [](const AceAqPlanCandidate& a, const AceAqPlanCandidate& b)
            {
                return a.score < b.score;
            });

        if (best != trace.candidatePlans.end() && !best->actions.empty())
        {
            trace.chosenPlan = best->actions;
            trace.chosenAction = best->actions.front();
        }

        return trace;
    }

    std::string ToJsonLikeString(const AceAqPlanCandidate& candidate)
    {
        std::ostringstream actions;
        actions << "[";
        for (std::size_t i = 0; i < candidate.actions.size(); ++i)
        {
            if (i > 0) actions << ",";
            actions << JsonString(ToString(candidate.actions[i]));
        }
        actions << "]";

        return "{" +
            std::string("\"actions\":") + actions.str() + "," +
            "\"expected_delta\":" + ToJsonLikeString(candidate.expectedDelta) + "," +
            "\"score\":" + JsonNumber(candidate.score) + "," +
            "\"risk\":" + JsonNumber(candidate.risk) + "," +
            "\"information_gain\":" + JsonNumber(candidate.informationGain) + "," +
            "\"confidence\":" + JsonNumber(candidate.confidence) + "," +
            "\"notes\":" + JsonStringArray(candidate.notes) +
            "}";
    }

    std::string ToJsonLikeString(const AceAqCounterfactualTrace& trace)
    {
        std::ostringstream plan;
        plan << "[";
        for (std::size_t i = 0; i < trace.chosenPlan.size(); ++i)
        {
            if (i > 0) plan << ",";
            plan << JsonString(ToString(trace.chosenPlan[i]));
        }
        plan << "]";

        std::ostringstream candidates;
        candidates << "[";
        for (std::size_t i = 0; i < trace.candidatePlans.size(); ++i)
        {
            if (i > 0) candidates << ",";
            candidates << ToJsonLikeString(trace.candidatePlans[i]);
        }
        candidates << "]";

        return "{" +
            std::string("\"chosen_action\":") + JsonString(ToString(trace.chosenAction)) + "," +
            "\"chosen_plan\":" + plan.str() + "," +
            "\"candidate_plans\":" + candidates.str() + "," +
            "\"notes\":" + JsonStringArray(trace.notes) +
            "}";
    }
}
