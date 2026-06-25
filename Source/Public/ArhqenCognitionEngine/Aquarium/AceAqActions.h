#pragma once

#include <optional>
#include <string>

namespace ace::aquarium
{
    enum class AceAqAction
    {
        TurnLeft,
        TurnRight,
        MoveForward,
        Wait,
        TouchFront,
        ConsumeFront,
        PushFront
    };

    enum class AceAqDirection
    {
        North,
        East,
        South,
        West
    };

    struct AceAqPoint
    {
        int x = 0;
        int y = 0;

        bool operator==(const AceAqPoint& other) const
        {
            return x == other.x && y == other.y;
        }

        bool operator!=(const AceAqPoint& other) const
        {
            return !(*this == other);
        }
    };

    std::string ToString(AceAqAction action);
    std::string ToString(AceAqDirection direction);

    bool TryParseAction(const std::string& text, AceAqAction* outAction);

    AceAqDirection TurnLeft(AceAqDirection direction);
    AceAqDirection TurnRight(AceAqDirection direction);
    AceAqPoint DirectionDelta(AceAqDirection direction);
}
