#include "ArhqenCognitionEngine/Aquarium/AceAqObservation.h"

#include "ArhqenCognitionEngine/Aquarium/AceAqJson.h"

#include <array>
#include <sstream>

namespace ace::aquarium
{
    AceAqCellObservation ObserveCell(const AceAqWorldObject& object)
    {
        return {
            object.blocksMovement,
            object.liquidLike,
            object.colorRgb,
            object.temperatureSignal,
            object.smellSignal,
        };
    }

    std::string ToJsonLikeString(const AceAqRgb& rgb)
    {
        return "{" +
            std::string("\"r\":") + std::to_string(rgb.r) + "," +
            "\"g\":" + std::to_string(rgb.g) + "," +
            "\"b\":" + std::to_string(rgb.b) +
            "}";
    }

    std::string ToJsonLikeString(const AceAqCellObservation& obs)
    {
        return "{" +
            std::string("\"solid_hint\":") + JsonBool(obs.solidHint) + "," +
            "\"liquid_like_hint\":" + JsonBool(obs.liquidLikeHint) + "," +
            "\"color_rgb\":" + ToJsonLikeString(obs.colorRgb) + "," +
            "\"temperature_signal\":" + JsonNumber(obs.temperatureSignal) + "," +
            "\"smell_signal\":" + JsonNumber(obs.smellSignal) +
            "}";
    }

    std::string ToJsonLikeString(const AceAqObservation& obs)
    {
        std::ostringstream out;
        out << "{";
        out << "\"local_view\":[";
        for (std::size_t i = 0; i < obs.localView.size(); ++i)
        {
            if (i > 0)
            {
                out << ",";
            }
            out << ToJsonLikeString(obs.localView[i]);
        }
        out << "],";
        out << "\"front\":" << ToJsonLikeString(obs.front) << ",";
        out << "\"left\":" << ToJsonLikeString(obs.left) << ",";
        out << "\"right\":" << ToJsonLikeString(obs.right) << ",";
        out << "\"current\":" << ToJsonLikeString(obs.current) << ",";
        out << "\"body_state\":" << ToJsonLikeString(obs.bodyState) << ",";
        out << "\"last_action_reason\":" << JsonString(obs.lastActionReason);
        out << "}";
        return out.str();
    }

    bool ObservationJsonContainsWorldTruthLabels(const std::string& jsonLike)
    {
        static constexpr const char* forbidden[] =
        {
            "WATER",
            "ACID",
            "FOOD",
            "WALL",
            "STONE",
            "ICE",
            "ObjectKind",
            "debug_truth"
        };

        for (const char* label : forbidden)
        {
            if (jsonLike.find(label) != std::string::npos)
            {
                return true;
            }
        }

        return false;
    }
}
