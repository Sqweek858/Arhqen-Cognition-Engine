#pragma once

#include "ArhqenCognitionEngine/Aquarium/AceAqObservation.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ace::aquarium
{
    struct AceAqRandomizationConfig
    {
        bool enabled = true;
        std::uint32_t seed = 0;
        int colorJitter = 32;
        double smellJitter = 0.10;
        double temperatureJitter = 0.10;
        bool allowCrossColorLiquids = false;
        bool preserveCausalEffects = true;
    };

    struct AceAqRandomizedObjectProfile
    {
        AceAqCellObservation before{};
        AceAqCellObservation after{};
        std::string profileId;
    };

    class AceAqObjectPropertyRandomizer
    {
    public:
        explicit AceAqObjectPropertyRandomizer(AceAqRandomizationConfig config = {});
        AceAqObservation RandomizeObservation(const AceAqObservation& observation);
        AceAqCellObservation RandomizeCell(const AceAqCellObservation& cell);
        const AceAqRandomizationConfig& Config() const { return config_; }

    private:
        double NextUnit();
        int JitterChannel(int value);
        double JitterSignal(double value, double amount);

        AceAqRandomizationConfig config_;
        std::uint32_t state_ = 1;
    };

    std::string AceAqRandomizationToJsonLike(const AceAqRandomizationConfig& config);
}
