#pragma once

#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>

namespace am::core
{
    enum class LogLevel
    {
        Info,
        Warning,
        Error
    };

    class Logger
    {
    public:
        Logger() = default;
        ~Logger();

        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;

        bool open(const std::filesystem::path& path, std::string* error);
        void info(const std::string& message);
        void warning(const std::string& message);
        void error(const std::string& message);

    private:
        void write(LogLevel level, const std::string& message);
        static const char* toString(LogLevel level);

        std::ofstream stream_;
        std::mutex mutex_;
    };
}
