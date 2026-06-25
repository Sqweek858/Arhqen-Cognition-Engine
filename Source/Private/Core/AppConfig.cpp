#include "ArhqenCognitionEngine/Core/AppConfig.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace am::core
{
    namespace
    {
        std::string trim(std::string value)
        {
            auto notSpace = [](unsigned char ch) { return !std::isspace(ch); };

            value.erase(value.begin(), std::find_if(value.begin(), value.end(), notSpace));
            value.erase(std::find_if(value.rbegin(), value.rend(), notSpace).base(), value.end());
            return value;
        }
    }

    AppConfig AppConfig::loadFromFile(const std::filesystem::path& path)
    {
        AppConfig config;

        std::ifstream input(path);
        if (!input.is_open())
        {
            config.error_ = "Missing config file: " + path.string();
            return config;
        }

        std::string line;
        int lineNumber = 0;

        while (std::getline(input, line))
        {
            ++lineNumber;

            auto clean = trim(line);
            if (clean.empty() || clean.rfind("#", 0) == 0)
            {
                continue;
            }

            const auto equals = clean.find('=');
            if (equals == std::string::npos)
            {
                std::ostringstream msg;
                msg << "Invalid config line " << lineNumber << ": expected key=value.";
                config.error_ = msg.str();
                return config;
            }

            auto key = trim(clean.substr(0, equals));
            auto value = trim(clean.substr(equals + 1));

            if (key.empty())
            {
                std::ostringstream msg;
                msg << "Invalid config line " << lineNumber << ": empty key.";
                config.error_ = msg.str();
                return config;
            }

            config.values_[std::move(key)] = std::move(value);
        }

        config.loaded_ = true;
        return config;
    }

    bool AppConfig::loaded() const
    {
        return loaded_;
    }

    const std::string& AppConfig::error() const
    {
        return error_;
    }

    std::string AppConfig::getString(const std::string& key, const std::string& fallback) const
    {
        const auto found = values_.find(key);
        if (found == values_.end())
        {
            return fallback;
        }

        return found->second;
    }

    int AppConfig::getInt(const std::string& key, int fallback) const
    {
        const auto found = values_.find(key);
        if (found == values_.end())
        {
            return fallback;
        }

        try
        {
            return std::stoi(found->second);
        }
        catch (const std::exception&)
        {
            return fallback;
        }
    }

    float AppConfig::getFloat(const std::string& key, float fallback) const
    {
        const auto found = values_.find(key);
        if (found == values_.end())
        {
            return fallback;
        }

        try
        {
            return std::stof(found->second);
        }
        catch (const std::exception&)
        {
            return fallback;
        }
    }
}
