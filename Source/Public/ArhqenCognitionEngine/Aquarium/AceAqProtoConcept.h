#pragma once

#include "ArhqenCognitionEngine/Aquarium/AceAqEpisode.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqWorldModel.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace ace::aquarium
{
    struct AceAqCausalSignature
    {
        std::string key;
        AceAqAction action = AceAqAction::Wait;
        std::vector<std::string> effectProfile;
    };

    struct AceAqProtoConcept
    {
        int conceptId = 0;
        int supportCount = 0;
        int contradictionCount = 0;
        std::vector<std::string> effectProfile;
        std::vector<std::uint64_t> supportingEpisodeIds;
        std::string summaryLabel;
        std::string signatureKey;
    };

    std::vector<std::string> AceAqEffectProfileFromDelta(
        const AceAqBodyDelta& delta,
        const AceAqLastActionResult& result
    );

    class AceAqProtoConceptMiner
    {
    public:
        std::vector<AceAqProtoConcept> Mine(const AceAqEpisodeMemory& memory) const;
        std::vector<AceAqProtoConcept> Mine(const std::vector<AceAqEpisode>& episodes) const;
    };

    std::string ToJsonLikeString(const AceAqProtoConcept& proto);
}
