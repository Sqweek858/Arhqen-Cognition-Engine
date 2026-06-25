#include "ArhqenCognitionEngine/Core/Logger.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace am::core
{
    Logger::~Logger()
    {
        if (stream_.is_open())
        {
            write(LogLevel::Info, "Logger shutdown.");
            stream_.close();
        }
    }

    bool Logger::open(const std::filesystem::path& path, std::string* error)
    {
        std::error_code ec;
        const auto parent = path.parent_path();
        if (!parent.empty())
        {
            std::filesystem::create_directories(parent, ec);
            if (ec)
            {
                if (error)
                {
                    *error = "Failed to create log directory '" + parent.string() + "': " + ec.message();
                }
                return false;
            }
        }

        stream_.open(path, std::ios::out | std::ios::app);
        if (!stream_.is_open())
        {
            if (error)
            {
                *error = "Failed to open log file '" + path.string() + "'.";
            }
            return false;
        }

        write(LogLevel::Info, "Logger started.");
        return true;
    }

    void Logger::info(const std::string& message)
    {
        write(LogLevel::Info, message);
    }

    void Logger::warning(const std::string& message)
    {
        write(LogLevel::Warning, message);
    }

    void Logger::error(const std::string& message)
    {
        write(LogLevel::Error, message);
    }

    void Logger::write(LogLevel level, const std::string& message)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        const auto now = std::chrono::system_clock::now();
        const auto time = std::chrono::system_clock::to_time_t(now);

        std::tm tm{};
    #if defined(_WIN32)
        localtime_s(&tm, &time);
    #else
        localtime_r(&time, &tm);
    #endif

        std::ostringstream line;
        line << "[" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "]";
        line << "[" << toString(level) << "] " << message;

        if (stream_.is_open())
        {
            stream_ << line.str() << '\n';
            stream_.flush();
        }

        if (level == LogLevel::Error)
        {
            std::cerr << line.str() << '\n';
        }
        else
        {
            std::cout << line.str() << '\n';
        }
    }

    const char* Logger::toString(LogLevel level)
    {
        switch (level)
        {
        case LogLevel::Info:
            return "INFO";
        case LogLevel::Warning:
            return "WARN";
        case LogLevel::Error:
            return "ERROR";
        default:
            return "UNKNOWN";
        }
    }
}
