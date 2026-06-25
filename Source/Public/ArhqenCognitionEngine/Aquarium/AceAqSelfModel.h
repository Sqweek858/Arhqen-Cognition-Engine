#pragma once

#include "ArhqenCognitionEngine/Aquarium/AceAqEpisode.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqWorldModel.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ace::aquarium
{
    struct AceAqAgencyAssessment
    {
        bool causedBySelf = true;
        double confidence = 0.0;
        std::string reason;
    };

    struct AceAqSelfEpisodeRecord
    {
        std::uint64_t episodeId = 0;
        AceAqAction action = AceAqAction::Wait;
        AceAqBodyDelta bodyDelta{};
        bool externalEvent = false;
        AceAqAgencyAssessment agency{};
        double predictionError = 0.0;
    };

    struct AceAqSelfModelState
    {
        int totalEpisodes = 0;
        int ownedEpisodes = 0;
        int externalEvents = 0;
        double agencyConfidence = 0.0;
        AceAqBodyState previousBody{};
        AceAqBodyState currentBody{};
        std::vector<double> predictionErrorHistory;
    };

    class AceAqSelfModel
    {
    public:
        AceAqAgencyAssessment ObserveEpisode(const AceAqEpisode& episode, const AceAqPrediction* prediction = nullptr);
        const std::vector<AceAqSelfEpisodeRecord>& Records() const;
        AceAqSelfModelState State() const;
        void Clear();

    private:
        std::vector<AceAqSelfEpisodeRecord> records_;
        AceAqSelfModelState state_{};
    };

    std::string ToJsonLikeString(const AceAqAgencyAssessment& assessment);
    std::string ToJsonLikeString(const AceAqSelfModelState& state);
}
