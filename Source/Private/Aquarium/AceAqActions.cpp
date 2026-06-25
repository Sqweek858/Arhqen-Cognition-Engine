#include "ArhqenCognitionEngine/Aquarium/AceAqActions.h"

#include <algorithm>
#include <cctype>

namespace ace::aquarium
{
    namespace
    {
        std::string Normalize(std::string text)
        {
            text.erase(std::remove_if(text.begin(), text.end(), [](unsigned char ch)
            {
                return ch == '_' || ch == '-' || std::isspace(ch) != 0;
            }), text.end());

            std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch)
            {
                return static_cast<char>(std::tolower(ch));
            });

            return text;
        }
    }

    std::string ToString(AceAqAction action)
    {
        switch (action)
        {
        case AceAqAction::TurnLeft: return "turn_left";
        case AceAqAction::TurnRight: return "turn_right";
        case AceAqAction::MoveForward: return "move_forward";
        case AceAqAction::Wait: return "wait";
        case AceAqAction::TouchFront: return "touch_front";
        case AceAqAction::ConsumeFront: return "consume_front";
        case AceAqAction::PushFront: return "push_front";
        }

        return "unknown";
    }

    std::string ToString(AceAqDirection direction)
    {
        switch (direction)
        {
        case AceAqDirection::North: return "north";
        case AceAqDirection::East: return "east";
        case AceAqDirection::South: return "south";
        case AceAqDirection::West: return "west";
        }

        return "unknown";
    }

    bool TryParseAction(const std::string& text, AceAqAction* outAction)
    {
        const auto value = Normalize(text);

        struct Row
        {
            const char* name;
            AceAqAction action;
        };

        static constexpr Row rows[] =
        {
            {"turnleft", AceAqAction::TurnLeft},
            {"turnright", AceAqAction::TurnRight},
            {"moveforward", AceAqAction::MoveForward},
            {"wait", AceAqAction::Wait},
            {"touchfront", AceAqAction::TouchFront},
            {"consumefront", AceAqAction::ConsumeFront},
            {"pushfront", AceAqAction::PushFront},
        };

        for (const auto& row : rows)
        {
            if (value == row.name)
            {
                if (outAction)
                {
                    *outAction = row.action;
                }
                return true;
            }
        }

        return false;
    }

    AceAqDirection TurnLeft(AceAqDirection direction)
    {
        switch (direction)
        {
        case AceAqDirection::North: return AceAqDirection::West;
        case AceAqDirection::West: return AceAqDirection::South;
        case AceAqDirection::South: return AceAqDirection::East;
        case AceAqDirection::East: return AceAqDirection::North;
        }

        return AceAqDirection::North;
    }

    AceAqDirection TurnRight(AceAqDirection direction)
    {
        switch (direction)
        {
        case AceAqDirection::North: return AceAqDirection::East;
        case AceAqDirection::East: return AceAqDirection::South;
        case AceAqDirection::South: return AceAqDirection::West;
        case AceAqDirection::West: return AceAqDirection::North;
        }

        return AceAqDirection::North;
    }

    AceAqPoint DirectionDelta(AceAqDirection direction)
    {
        switch (direction)
        {
        case AceAqDirection::North: return {0, -1};
        case AceAqDirection::East: return {1, 0};
        case AceAqDirection::South: return {0, 1};
        case AceAqDirection::West: return {-1, 0};
        }

        return {0, 0};
    }
}
