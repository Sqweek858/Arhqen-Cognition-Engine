#include "ArhqenCognitionEngine/Aquarium/AceAqWorldEvents.h"

#include "ArhqenCognitionEngine/Aquarium/AceAqJson.h"

#include <sstream>

namespace ace::aquarium
{
    namespace
    {
        std::string PointJson(const AceAqPoint& point)
        {
            return "{" + std::string("\"x\":") + std::to_string(point.x) + ",\"y\":" + std::to_string(point.y) + "}";
        }
    }

    std::string ToJsonLikeString(const AceAqWorldEvent& event)
    {
        return "{" +
            std::string("\"event_type\":") + JsonString(event.eventType) + "," +
            "\"step\":" + std::to_string(event.step) + "," +
            "\"position_before\":" + PointJson(event.positionBefore) + "," +
            "\"position_after\":" + PointJson(event.positionAfter) + "," +
            "\"object_before\":" + JsonString(event.objectBefore) + "," +
            "\"object_after\":" + JsonString(event.objectAfter) + "," +
            "\"body_delta\":" + ToJsonLikeString(event.bodyDelta) + "," +
            "\"external\":" + JsonBool(event.external) + "," +
            "\"event_flags\":" + JsonStringArray(event.eventFlags) + "," +
            "\"description\":" + JsonString(event.description) +
            "}";
    }

    std::string ToJsonLikeString(const std::vector<AceAqWorldEvent>& events)
    {
        std::ostringstream out;
        out << "[";
        for (std::size_t i = 0; i < events.size(); ++i)
        {
            if (i > 0) out << ",";
            out << ToJsonLikeString(events[i]);
        }
        out << "]";
        return out.str();
    }
}
