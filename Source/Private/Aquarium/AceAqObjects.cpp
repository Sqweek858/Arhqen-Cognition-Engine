#include "ArhqenCognitionEngine/Aquarium/AceAqObjects.h"

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

    std::string ToString(AceAqObjectKind kind)
    {
        switch (kind)
        {
        case AceAqObjectKind::Empty: return "EMPTY";
        case AceAqObjectKind::Wall: return "WALL";
        case AceAqObjectKind::Water: return "WATER";
        case AceAqObjectKind::Acid: return "ACID";
        case AceAqObjectKind::Food: return "FOOD";
        case AceAqObjectKind::Ice: return "ICE";
        case AceAqObjectKind::Stone: return "STONE";
        case AceAqObjectKind::PoisonFood: return "POISON_FOOD";
        case AceAqObjectKind::SlowMedicine: return "SLOW_MEDICINE";
        case AceAqObjectKind::ColdLiquid: return "COLD_LIQUID";
        case AceAqObjectKind::MovingHazard: return "MOVING_HAZARD";
        case AceAqObjectKind::SpreadingAcid: return "SPREADING_ACID";
        }

        return "UNKNOWN";
    }

    bool TryParseObjectKind(const std::string& text, AceAqObjectKind* outKind)
    {
        const auto value = Normalize(text);

        struct Row
        {
            const char* name;
            AceAqObjectKind kind;
        };

        static constexpr Row rows[] =
        {
            {"empty", AceAqObjectKind::Empty},
            {"wall", AceAqObjectKind::Wall},
            {"water", AceAqObjectKind::Water},
            {"acid", AceAqObjectKind::Acid},
            {"food", AceAqObjectKind::Food},
            {"ice", AceAqObjectKind::Ice},
            {"stone", AceAqObjectKind::Stone},
            {"poisonfood", AceAqObjectKind::PoisonFood},
            {"slowmedicine", AceAqObjectKind::SlowMedicine},
            {"coldliquid", AceAqObjectKind::ColdLiquid},
            {"movinghazard", AceAqObjectKind::MovingHazard},
            {"spreadingacid", AceAqObjectKind::SpreadingAcid},
        };

        for (const auto& row : rows)
        {
            if (value == row.name)
            {
                if (outKind)
                {
                    *outKind = row.kind;
                }
                return true;
            }
        }

        return false;
    }

    AceAqWorldObject MakeObject(AceAqObjectKind kind)
    {
        AceAqWorldObject object;
        object.kind = kind;

        switch (kind)
        {
        case AceAqObjectKind::Empty:
            object.colorRgb = {8, 12, 18};
            return object;

        case AceAqObjectKind::Wall:
            object.blocksMovement = true;
            object.colorRgb = {90, 95, 105};
            return object;

        case AceAqObjectKind::Water:
            object.liquidLike = true;
            object.consumable = true;
            object.colorRgb = {45, 120, 220};
            object.temperatureSignal = -0.10;
            object.smellSignal = 0.05;
            object.consumeEffect = {0.35, 0.0, 0.0, -0.03};
            return object;

        case AceAqObjectKind::Acid:
            object.liquidLike = true;
            object.consumable = true;
            object.colorRgb = {60, 210, 80};
            object.temperatureSignal = 0.05;
            object.smellSignal = 0.60;
            object.touchEffect = {0.0, 0.0, -0.20, 0.0};
            object.consumeEffect = {0.0, 0.0, -0.55, 0.0};
            return object;

        case AceAqObjectKind::Food:
            object.consumable = true;
            object.colorRgb = {210, 165, 70};
            object.smellSignal = 0.80;
            object.consumeEffect = {0.0, 0.35, 0.0, 0.0};
            return object;

        case AceAqObjectKind::Ice:
            object.blocksMovement = true;
            object.colorRgb = {160, 220, 255};
            object.temperatureSignal = -0.80;
            object.touchEffect = {0.0, 0.0, 0.0, -0.10};
            return object;

        case AceAqObjectKind::Stone:
            object.blocksMovement = true;
            object.pushable = true;
            object.colorRgb = {90, 85, 82};
            return object;

        case AceAqObjectKind::PoisonFood:
            object.consumable = true;
            object.colorRgb = {190, 120, 70};
            object.smellSignal = 0.75;
            object.consumeEffect = {0.0, 0.20, 0.0, 0.0};
            return object;

        case AceAqObjectKind::SlowMedicine:
            object.consumable = true;
            object.colorRgb = {120, 80, 220};
            object.smellSignal = 0.35;
            object.consumeEffect = {0.0, 0.0, 0.03, 0.0};
            return object;

        case AceAqObjectKind::ColdLiquid:
            object.liquidLike = true;
            object.consumable = true;
            object.colorRgb = {110, 200, 255};
            object.temperatureSignal = -0.65;
            object.smellSignal = 0.08;
            object.consumeEffect = {0.20, 0.0, 0.0, -0.06};
            return object;

        case AceAqObjectKind::MovingHazard:
            object.blocksMovement = true;
            object.colorRgb = {220, 50, 50};
            object.temperatureSignal = 0.30;
            object.smellSignal = 0.70;
            object.touchEffect = {0.0, 0.0, -0.20, 0.0};
            return object;

        case AceAqObjectKind::SpreadingAcid:
            object.liquidLike = true;
            object.consumable = true;
            object.colorRgb = {70, 240, 70};
            object.temperatureSignal = 0.10;
            object.smellSignal = 0.65;
            object.touchEffect = {0.0, 0.0, -0.20, 0.0};
            object.consumeEffect = {0.0, 0.0, -0.55, 0.0};
            return object;
        }

        return object;
    }

    char ToAscii(AceAqObjectKind kind)
    {
        switch (kind)
        {
        case AceAqObjectKind::Empty: return '.';
        case AceAqObjectKind::Wall: return '#';
        case AceAqObjectKind::Water: return 'W';
        case AceAqObjectKind::Acid: return 'X';
        case AceAqObjectKind::Food: return 'F';
        case AceAqObjectKind::Ice: return 'I';
        case AceAqObjectKind::Stone: return 'S';
        case AceAqObjectKind::PoisonFood: return 'P';
        case AceAqObjectKind::SlowMedicine: return 'M';
        case AceAqObjectKind::ColdLiquid: return 'C';
        case AceAqObjectKind::MovingHazard: return 'H';
        case AceAqObjectKind::SpreadingAcid: return 'D';
        }

        return '?';
    }

    AceAqObjectKind ObjectKindFromAscii(char ch)
    {
        switch (ch)
        {
        case '#': return AceAqObjectKind::Wall;
        case 'W': return AceAqObjectKind::Water;
        case 'X':
        case 'A': return AceAqObjectKind::Acid;
        case 'F': return AceAqObjectKind::Food;
        case 'I': return AceAqObjectKind::Ice;
        case 'S': return AceAqObjectKind::Stone;
        case 'P': return AceAqObjectKind::PoisonFood;
        case 'M': return AceAqObjectKind::SlowMedicine;
        case 'C': return AceAqObjectKind::ColdLiquid;
        case 'H': return AceAqObjectKind::MovingHazard;
        case 'D': return AceAqObjectKind::SpreadingAcid;
        case '.':
        default: return AceAqObjectKind::Empty;
        }
    }
}
