#include "ArhqenCognitionEngine/Aquarium/AceAqSelfModel.h"

#include "ArhqenCognitionEngine/Aquarium/AceAqJson.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace ace::aquarium
{
    namespace
    {
        bool HasFlag(const AceAqLastActionResult& result, const std::string& flag)
        {
            return std::find(result.eventFlags.begin(), result.eventFlags.end(), flag) != result.eventFlags.end();
        }

        double DeltaMagnitude(const AceAqBodyDelta& delta)
        {
            return std::abs(delta.hydration) +
                   std::abs(delta.nutrition) +
                   std::abs(delta.integrity) +
                   std::abs(delta.temperature);
        }
    }

    AceAqAgencyAssessment AceAqSelfModel::ObserveEpisode(const AceAqEpisode& episode, const AceAqPrediction* prediction)
    {
        bool external = episode.lastActionResult.externalEvent || HasFlag(episode.lastActionResult, "external_event");
        for (const auto& item : episode.appliedDelayedEffects)
        {
            external = external || item.external;
        }
        for (const auto& item : episode.externalWorldEvents)
        {
            external = external || item.external;
        }

        AceAqAgencyAssessment assessment;
        assessment.causedBySelf = !external;
        if (external)
        {
            assessment.confidence = 0.0;
            assessment.reason = "external_event_not_self_caused";
        }
        else
        {
            const double magnitude = DeltaMagnitude(episode.actualDelta);
            assessment.confidence = magnitude > 0.005 ? 0.80 : 0.45;
            assessment.reason = magnitude > 0.005 ? "own_action_body_delta_observed" : "own_action_low_body_delta";
        }

        double predictionError = 0.0;
        if (prediction)
        {
            AceAqTableWorldModel model;
            predictionError = model.PredictionError(*prediction, episode.actualDelta);
        }

        AceAqSelfEpisodeRecord record;
        record.episodeId = episode.episodeId;
        record.action = episode.action;
        record.bodyDelta = episode.actualDelta;
        record.externalEvent = external;
        record.agency = assessment;
        record.predictionError = predictionError;
        records_.push_back(record);

        state_.totalEpisodes += 1;
        if (external)
        {
            state_.externalEvents += 1;
        }
        else
        {
            state_.ownedEpisodes += 1;
        }

        state_.previousBody = episode.bodyBefore;
        state_.currentBody = episode.bodyAfter;
        state_.predictionErrorHistory.push_back(predictionError);
        state_.agencyConfidence = state_.totalEpisodes == 0
            ? 0.0
            : static_cast<double>(state_.ownedEpisodes) / static_cast<double>(state_.totalEpisodes);

        return assessment;
    }

    const std::vector<AceAqSelfEpisodeRecord>& AceAqSelfModel::Records() const
    {
        return records_;
    }

    AceAqSelfModelState AceAqSelfModel::State() const
    {
        return state_;
    }

    void AceAqSelfModel::Clear()
    {
        records_.clear();
        state_ = {};
    }

    std::string ToJsonLikeString(const AceAqAgencyAssessment& assessment)
    {
        return "{" +
            std::string("\"caused_by_self\":") + JsonBool(assessment.causedBySelf) + "," +
            "\"confidence\":" + JsonNumber(assessment.confidence) + "," +
            "\"reason\":" + JsonString(assessment.reason) +
            "}";
    }

    std::string ToJsonLikeString(const AceAqSelfModelState& state)
    {
        std::ostringstream errors;
        errors << "[";
        for (std::size_t i = 0; i < state.predictionErrorHistory.size(); ++i)
        {
            if (i > 0) errors << ",";
            errors << JsonNumber(state.predictionErrorHistory[i]);
        }
        errors << "]";

        return "{" +
            std::string("\"total_episodes\":") + std::to_string(state.totalEpisodes) + "," +
            "\"owned_episodes\":" + std::to_string(state.ownedEpisodes) + "," +
            "\"external_events\":" + std::to_string(state.externalEvents) + "," +
            "\"agency_confidence\":" + JsonNumber(state.agencyConfidence) + "," +
            "\"previous_body\":" + ToJsonLikeString(state.previousBody) + "," +
            "\"current_body\":" + ToJsonLikeString(state.currentBody) + "," +
            "\"prediction_error_history\":" + errors.str() +
            "}";
    }
}
