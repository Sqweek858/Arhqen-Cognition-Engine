#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace am::core::assets
{
    enum class AssetFileChangeAction : std::uint8_t
    {
        Added,
        Modified,
        Removed,
        RenamedOld,
        RenamedNew,
        RescanRequired
    };

    struct AssetFileChange
    {
        std::filesystem::path relativePath;
        AssetFileChangeAction action = AssetFileChangeAction::Modified;
    };

    struct AssetDirectoryWatcherStats
    {
        std::uint64_t nativeBatches = 0;
        std::uint64_t queuedEvents = 0;
        std::uint64_t deliveredEvents = 0;
        std::uint64_t droppedEvents = 0;
        std::uint64_t rescanSignals = 0;
    };

    class AssetDirectoryWatcher final
    {
    public:
        AssetDirectoryWatcher();
        ~AssetDirectoryWatcher();

        AssetDirectoryWatcher(const AssetDirectoryWatcher&) = delete;
        AssetDirectoryWatcher& operator=(const AssetDirectoryWatcher&) = delete;

        bool start(std::filesystem::path root, std::string* error = nullptr);
        void stop() noexcept;
        [[nodiscard]] bool running() const noexcept;
        [[nodiscard]] const std::filesystem::path& root() const noexcept;
        [[nodiscard]] std::vector<AssetFileChange> drainChanges(std::size_t maximumEvents = 4096);
        [[nodiscard]] AssetDirectoryWatcherStats stats() const noexcept;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
}
