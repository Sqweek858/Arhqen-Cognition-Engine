#pragma once

#include <string>
#include <vector>

namespace ace::aquarium
{
    struct AceAqParityCheck
    {
        std::string milestone;
        bool passed = false;
        std::string note;
    };

    std::vector<AceAqParityCheck> RunAceAqM0M15ParityChecks();
    std::string ToJsonLikeString(const AceAqParityCheck& check);
    std::string ToJsonLikeString(const std::vector<AceAqParityCheck>& checks);
}
