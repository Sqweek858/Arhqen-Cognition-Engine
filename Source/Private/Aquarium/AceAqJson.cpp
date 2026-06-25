#include "ArhqenCognitionEngine/Aquarium/AceAqJson.h"

#include <iomanip>
#include <sstream>

namespace ace::aquarium
{
    std::string JsonEscape(const std::string& value)
    {
        std::ostringstream out;
        for (const char ch : value)
        {
            switch (ch)
            {
            case '\\': out << "\\\\"; break;
            case '"': out << "\\\""; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default: out << ch; break;
            }
        }
        return out.str();
    }

    std::string JsonString(const std::string& value)
    {
        return "\"" + JsonEscape(value) + "\"";
    }

    std::string JsonBool(bool value)
    {
        return value ? "true" : "false";
    }

    std::string JsonNumber(double value)
    {
        std::ostringstream out;
        out << std::fixed << std::setprecision(6) << value;
        return out.str();
    }

    std::string JsonStringArray(const std::vector<std::string>& values)
    {
        std::ostringstream out;
        out << "[";
        for (std::size_t i = 0; i < values.size(); ++i)
        {
            if (i > 0)
            {
                out << ",";
            }
            out << JsonString(values[i]);
        }
        out << "]";
        return out.str();
    }
}
