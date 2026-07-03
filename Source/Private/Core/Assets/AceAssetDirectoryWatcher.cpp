#include "ArhqenCognitionEngine/Core/Assets/AceAssetDirectoryWatcher.h"

#if !defined(_WIN32)
#error ACE Asset Directory Watcher currently requires Windows.
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <deque>
#include <mutex>
#include <thread>
#include <unordered_set>

namespace am::core::assets
{
    namespace
    {
        constexpr std::size_t kNativeBufferBytes = 64ull * 1024ull;
        constexpr std::size_t kMaximumQueuedEvents = 4096;

        void fail(std::string* error, std::string message)
        {
            if (error) *error = std::move(message);
        }

        std::string win32Error(const char* operation, DWORD code = GetLastError())
        {
            return std::string(operation) + " failed with Win32 error " + std::to_string(code);
        }

        AssetFileChangeAction mapAction(DWORD action)
        {
            switch (action)
            {
            case FILE_ACTION_ADDED: return AssetFileChangeAction::Added;
            case FILE_ACTION_REMOVED: return AssetFileChangeAction::Removed;
            case FILE_ACTION_MODIFIED: return AssetFileChangeAction::Modified;
            case FILE_ACTION_RENAMED_OLD_NAME: return AssetFileChangeAction::RenamedOld;
            case FILE_ACTION_RENAMED_NEW_NAME: return AssetFileChangeAction::RenamedNew;
            default: return AssetFileChangeAction::RescanRequired;
            }
        }

        bool safeRelativePath(const std::filesystem::path& path)
        {
            if (path.empty() || path.is_absolute() || path.has_root_path()) return false;
            for (const auto& part : path) if (part == L"..") return false;
            return true;
        }
    }

    struct AssetDirectoryWatcher::Impl
    {
        std::filesystem::path root;
        HANDLE directory = INVALID_HANDLE_VALUE;
        HANDLE stopEvent = nullptr;
        HANDLE changeEvent = nullptr;
        std::thread worker;
        std::atomic<bool> running{false};
        mutable std::mutex mutex;
        std::deque<AssetFileChange> changes;
        AssetDirectoryWatcherStats stats{};

        void enqueue(AssetFileChange change)
        {
            std::lock_guard lock(mutex);
            if (change.action == AssetFileChangeAction::RescanRequired)
            {
                changes.clear();
                changes.push_back({{}, AssetFileChangeAction::RescanRequired});
                ++stats.rescanSignals;
                ++stats.queuedEvents;
                return;
            }
            if (!changes.empty() && changes.front().action == AssetFileChangeAction::RescanRequired) return;
            if (changes.size() >= kMaximumQueuedEvents)
            {
                stats.droppedEvents += changes.size() + 1;
                changes.clear();
                changes.push_back({{}, AssetFileChangeAction::RescanRequired});
                ++stats.rescanSignals;
                ++stats.queuedEvents;
                return;
            }
            changes.push_back(std::move(change));
            ++stats.queuedEvents;
        }

        void run()
        {
            std::array<std::byte, kNativeBufferBytes> buffer{};
            while (running.load(std::memory_order_acquire))
            {
                ResetEvent(changeEvent);
                OVERLAPPED overlapped{};
                overlapped.hEvent = changeEvent;
                const DWORD filter = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME |
                    FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION;
                if (!ReadDirectoryChangesW(directory, buffer.data(), static_cast<DWORD>(buffer.size()), TRUE,
                    filter, nullptr, &overlapped, nullptr))
                {
                    if (GetLastError() != ERROR_OPERATION_ABORTED) enqueue({{}, AssetFileChangeAction::RescanRequired});
                    break;
                }

                HANDLE waits[] = {stopEvent, changeEvent};
                const DWORD waitResult = WaitForMultipleObjects(2, waits, FALSE, INFINITE);
                if (waitResult == WAIT_OBJECT_0)
                {
                    CancelIoEx(directory, &overlapped);
                    break;
                }
                if (waitResult != WAIT_OBJECT_0 + 1)
                {
                    CancelIoEx(directory, &overlapped);
                    enqueue({{}, AssetFileChangeAction::RescanRequired});
                    break;
                }

                DWORD bytes = 0;
                if (!GetOverlappedResult(directory, &overlapped, &bytes, FALSE))
                {
                    const DWORD code = GetLastError();
                    if (code != ERROR_OPERATION_ABORTED)
                        enqueue({{}, AssetFileChangeAction::RescanRequired});
                    if (code == ERROR_OPERATION_ABORTED) break;
                    continue;
                }
                {
                    std::lock_guard lock(mutex);
                    ++stats.nativeBatches;
                }
                if (bytes == 0)
                {
                    enqueue({{}, AssetFileChangeAction::RescanRequired});
                    continue;
                }

                std::size_t offset = 0;
                bool malformed = false;
                while (offset < bytes)
                {
                    if (bytes - offset < sizeof(FILE_NOTIFY_INFORMATION))
                    {
                        malformed = true;
                        break;
                    }
                    const auto* info = reinterpret_cast<const FILE_NOTIFY_INFORMATION*>(buffer.data() + offset);
                    if ((info->FileNameLength % sizeof(wchar_t)) != 0 ||
                        info->FileNameLength > bytes - offset - offsetof(FILE_NOTIFY_INFORMATION, FileName))
                    {
                        malformed = true;
                        break;
                    }
                    const std::wstring name(info->FileName, info->FileNameLength / sizeof(wchar_t));
                    const std::filesystem::path relative(name);
                    const auto action = mapAction(info->Action);
                    if (!safeRelativePath(relative) || action == AssetFileChangeAction::RescanRequired)
                    {
                        malformed = true;
                        break;
                    }
                    enqueue({relative, action});
                    if (info->NextEntryOffset == 0) break;
                    if (info->NextEntryOffset < sizeof(FILE_NOTIFY_INFORMATION) ||
                        info->NextEntryOffset > bytes - offset)
                    {
                        malformed = true;
                        break;
                    }
                    offset += info->NextEntryOffset;
                }
                if (malformed) enqueue({{}, AssetFileChangeAction::RescanRequired});
            }
            running.store(false, std::memory_order_release);
        }
    };

    AssetDirectoryWatcher::AssetDirectoryWatcher() : impl_(std::make_unique<Impl>()) {}
    AssetDirectoryWatcher::~AssetDirectoryWatcher() { stop(); }

    bool AssetDirectoryWatcher::start(std::filesystem::path root, std::string* error)
    {
        if (error) error->clear();
        stop();
        std::error_code ec;
        root = std::filesystem::weakly_canonical(root, ec);
        if (ec || !std::filesystem::is_directory(root, ec))
        {
            fail(error, "Asset watch root is not a readable directory");
            return false;
        }

        impl_->directory = CreateFileW(root.c_str(), FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr);
        if (impl_->directory == INVALID_HANDLE_VALUE)
        {
            fail(error, win32Error("CreateFileW asset watch root"));
            return false;
        }
        impl_->stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        impl_->changeEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (!impl_->stopEvent || !impl_->changeEvent)
        {
            fail(error, win32Error("CreateEventW asset watcher"));
            stop();
            return false;
        }
        {
            std::lock_guard lock(impl_->mutex);
            impl_->root = std::move(root);
            impl_->changes.clear();
            impl_->stats = {};
        }
        impl_->running.store(true, std::memory_order_release);
        try
        {
            impl_->worker = std::thread([this]() { impl_->run(); });
        }
        catch (...)
        {
            fail(error, "Could not create Asset Directory Watcher thread");
            impl_->running.store(false, std::memory_order_release);
            stop();
            return false;
        }
        return true;
    }

    void AssetDirectoryWatcher::stop() noexcept
    {
        if (!impl_) return;
        impl_->running.store(false, std::memory_order_release);
        if (impl_->stopEvent) SetEvent(impl_->stopEvent);
        if (impl_->directory != INVALID_HANDLE_VALUE) CancelIoEx(impl_->directory, nullptr);
        if (impl_->worker.joinable()) impl_->worker.join();
        if (impl_->changeEvent) { CloseHandle(impl_->changeEvent); impl_->changeEvent = nullptr; }
        if (impl_->stopEvent) { CloseHandle(impl_->stopEvent); impl_->stopEvent = nullptr; }
        if (impl_->directory != INVALID_HANDLE_VALUE) { CloseHandle(impl_->directory); impl_->directory = INVALID_HANDLE_VALUE; }
    }

    bool AssetDirectoryWatcher::running() const noexcept
    {
        return impl_ && impl_->running.load(std::memory_order_acquire);
    }

    const std::filesystem::path& AssetDirectoryWatcher::root() const noexcept
    {
        return impl_->root;
    }

    std::vector<AssetFileChange> AssetDirectoryWatcher::drainChanges(std::size_t maximumEvents)
    {
        std::vector<AssetFileChange> output;
        if (!impl_ || maximumEvents == 0) return output;
        std::lock_guard lock(impl_->mutex);
        const auto rescan = std::find_if(impl_->changes.begin(), impl_->changes.end(), [](const AssetFileChange& change)
        {
            return change.action == AssetFileChangeAction::RescanRequired;
        });
        if (rescan != impl_->changes.end())
        {
            output.push_back(*rescan);
            impl_->stats.deliveredEvents += impl_->changes.size();
            impl_->changes.clear();
            return output;
        }

        std::unordered_set<std::wstring> seenModified;
        while (!impl_->changes.empty() && output.size() < maximumEvents)
        {
            AssetFileChange change = std::move(impl_->changes.front());
            impl_->changes.pop_front();
            if (change.action == AssetFileChangeAction::Modified &&
                !seenModified.emplace(change.relativePath.generic_wstring()).second)
                continue;
            output.push_back(std::move(change));
        }
        impl_->stats.deliveredEvents += output.size();
        return output;
    }

    AssetDirectoryWatcherStats AssetDirectoryWatcher::stats() const noexcept
    {
        if (!impl_) return {};
        std::lock_guard lock(impl_->mutex);
        return impl_->stats;
    }
}
