#pragma once

#include "ArhqenCognitionEngine/Aquarium/AceAqBody.h"

#include <array>
#include <string>

namespace ace::aquarium
{
    enum class AceAqObjectKind
    {
        Empty,
        Wall,
        Water,
        Acid,
        Food,
        Ice,
        Stone,
        PoisonFood,
        SlowMedicine,
        ColdLiquid,
        MovingHazard,
        SpreadingAcid
    };

    struct AceAqRgb
    {
        int r = 0;
        int g = 0;
        int b = 0;
    };

    struct AceAqWorldObject
    {
        AceAqObjectKind kind = AceAqObjectKind::Empty;
        bool blocksMovement = false;
        bool pushable = false;
        bool liquidLike = false;
        bool consumable = false;
        AceAqRgb colorRgb{};
        double temperatureSignal = 0.0;
        double smellSignal = 0.0;
        AceAqBodyDelta touchEffect{};
        AceAqBodyDelta consumeEffect{};
    };

    std::string ToString(AceAqObjectKind kind);
    bool TryParseObjectKind(const std::string& text, AceAqObjectKind* outKind);
    AceAqWorldObject MakeObject(AceAqObjectKind kind);
    char ToAscii(AceAqObjectKind kind);
    AceAqObjectKind ObjectKindFromAscii(char ch);
}
