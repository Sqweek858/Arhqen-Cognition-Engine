#pragma once

#include "ArhqenCognitionEngine/Aquarium/AceAqObservation.h"

#include <cstdint>
#include <string>

namespace ace::aquarium
{
    struct AceAqSensorNoiseConfig
    {
        bool enabled = true;
        std::uint32_t seed = 0;
        int colorNoise = 12;
        double smellNoise = 0.05;
        double temperatureNoise = 0.05;
        double hintFlipProbability = 0.0;
        bool includeMetadata = true;
    };

    struct AceAqNoisyObservationMetadata
    {
        double colorConfidence = 1.0;
        double smellConfidence = 1.0;
        double temperatureConfidence = 1.0;
        bool hintFlipped = false;
    };

    class AceAqSensorNoiseModel
    {
    public:
        explicit AceAqSensorNoiseModel(AceAqSensorNoiseConfig config = {});
        AceAqObservation ApplyNoise(const AceAqObservation& cleanObservation, AceAqNoisyObservationMetadata* outMetadata = nullptr);

    private:
        double NextUnit();
        int ApplyColorNoise(int value);
        double ApplySignalNoise(double value, double amount);
        bool ShouldFlip();

        AceAqSensorNoiseConfig config_;
        std::uint32_t state_ = 1;
    };

    std::string AceAqSensorNoiseToJsonLike(const AceAqSensorNoiseConfig& config);
    std::string ToJsonLikeString(const AceAqNoisyObservationMetadata& metadata);
}
