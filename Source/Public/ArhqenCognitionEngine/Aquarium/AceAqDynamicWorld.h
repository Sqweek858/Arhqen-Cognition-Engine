#pragma once

#include "ArhqenCognitionEngine/Aquarium/AceAqBody.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqGrid.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqWorldEvents.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ace::aquarium
{
    struct AceAqDynamicWorldConfig
    {
        bool enabled = false;
        std::uint32_t seed = 0;
        bool enableMovingHazards = true;
        bool enableSpreadingAcid = true;
        bool enableFoodDecay = true;
        int foodDecayStep = 3;
    };

    class AceAqDynamicWorldSystem
    {
    public:
        explicit AceAqDynamicWorldSystem(AceAqDynamicWorldConfig config = {});
        void Reset(AceAqDynamicWorldConfig config);
        std::vector<AceAqWorldEvent> Tick(int step, AceAqGridWorld& world, AceAqBodyState& body);
        const AceAqDynamicWorldConfig& Config() const { return config_; }
        std::string ToJsonLikeString() const;

    private:
        std::uint32_t NextRandom();

        AceAqDynamicWorldConfig config_{};
        std::uint32_t state_ = 1;
    };

    std::string ToJsonLikeString(const AceAqDynamicWorldConfig& config);
}
