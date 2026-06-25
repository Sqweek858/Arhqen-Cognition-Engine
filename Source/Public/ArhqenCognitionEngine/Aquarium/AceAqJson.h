#pragma once

#include <string>
#include <vector>

namespace ace::aquarium
{
    std::string JsonEscape(const std::string& value);
    std::string JsonString(const std::string& value);
    std::string JsonBool(bool value);
    std::string JsonNumber(double value);
    std::string JsonStringArray(const std::vector<std::string>& values);
}
