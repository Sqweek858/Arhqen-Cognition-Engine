#pragma once

#include <cstddef>
#include <deque>
#include <functional>
#include <string>

namespace am::core
{
    class Logger;

    class TaskQueue
    {
    public:
        using Task = std::function<void()>;

        void enqueue(std::string name, Task task);
        bool drain(Logger& logger);

        std::size_t pendingCount() const;
        std::size_t executedCount() const;

    private:
        struct QueuedTask
        {
            std::string name;
            Task task;
        };

        std::deque<QueuedTask> tasks_;
        std::size_t executedCount_ = 0;
    };
}
