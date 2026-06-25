#pragma once

#include "ArhqenCognitionEngine/Aquarium/AceAqActions.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqBody.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqObjects.h"

#include <array>
#include <string>

namespace ace::aquarium
{
    struct AceAqCellObservation
    {
        bool solidHint = false;
        bool liquidLikeHint = false;
        AceAqRgb colorRgb{};
        double temperatureSignal = 0.0;
        double smellSignal = 0.0;
    };

    struct AceAqObservation
    {
        std::array<AceAqCellObservation, 9> localView{};
        AceAqCellObservation front{};
        AceAqCellObservation left{};
        AceAqCellObservation right{};
        AceAqCellObservation current{};
        AceAqBodyState bodyState{};
        std::string lastActionReason;
    };

    AceAqCellObservation ObserveCell(const AceAqWorldObject& object);
    std::string ToJsonLikeString(const AceAqRgb& rgb);
    std::string ToJsonLikeString(const AceAqCellObservation& obs);
    std::string ToJsonLikeString(const AceAqObservation& obs);

    bool ObservationJsonContainsWorldTruthLabels(const std::string& jsonLike);
}
