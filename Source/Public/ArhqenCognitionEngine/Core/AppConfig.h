#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>

namespace am::core
{
    class AppConfig
    {
    public:
        static AppConfig loadFromFile(const std::filesystem::path& path);

        bool loaded() const;
        const std::string& error() const;

        std::string getString(const std::string& key, const std::string& fallback) const;
        int getInt(const std::string& key, int fallback) const;
        float getFloat(const std::string& key, float fallback) const;

    private:
        bool loaded_ = false;
        std::string error_;
        std::unordered_map<std::string, std::string> values_;
    };
}
