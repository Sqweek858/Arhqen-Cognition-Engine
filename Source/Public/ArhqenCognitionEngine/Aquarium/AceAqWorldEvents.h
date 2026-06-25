#pragma once

#include "ArhqenCognitionEngine/Aquarium/AceAqActions.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqBody.h"

#include <string>
#include <vector>

namespace ace::aquarium
{
    struct AceAqWorldEvent
    {
        std::string eventType;
        int step = 0;
        AceAqPoint positionBefore{};
        AceAqPoint positionAfter{};
        std::string objectBefore;
        std::string objectAfter;
        AceAqBodyDelta bodyDelta{};
        bool external = true;
        std::vector<std::string> eventFlags;
        std::string description;
    };

    std::string ToJsonLikeString(const AceAqWorldEvent& event);
    std::string ToJsonLikeString(const std::vector<AceAqWorldEvent>& events);
}
