#pragma once

#include "ArhqenCognitionEngine/Aquarium/AceAqActions.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqBody.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqEpisode.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqObservation.h"

#include <map>
#include <string>
#include <vector>

namespace ace::aquarium
{
    struct AceAqObservationSignature
    {
        std::string key;
    };

    struct AceAqTransitionStats
    {
        int count = 0;
        AceAqBodyDelta sumDelta{};
        std::map<std::string, int> flagCounts;

        AceAqBodyDelta MeanDelta() const;
    };

    struct AceAqPrediction
    {
        AceAqBodyDelta predictedDelta{};
        double confidence = 0.0;
        double uncertainty = 1.0;
        bool hasPrediction = false;
        int sampleCount = 0;
    };

    AceAqObservationSignature MakeObservationSignature(const AceAqObservation& observation);
    std::string MakeTransitionKey(const AceAqObservation& observation, AceAqAction action);

    class AceAqTableWorldModel
    {
    public:
        AceAqPrediction Predict(const AceAqObservation& observation, AceAqAction action) const;
        void LearnFromEpisode(const AceAqEpisode& episode);
        double PredictionError(const AceAqPrediction& prediction, const AceAqBodyDelta& actualDelta) const;

        int EntryCount() const;
        int SampleCountFor(const AceAqObservation& observation, AceAqAction action) const;
        std::string ToJsonLikeString() const;

    private:
        std::map<std::string, AceAqTransitionStats> transitions_;
    };

    std::string ToJsonLikeString(const AceAqPrediction& prediction);
}
