#include "ArhqenCognitionEngine/Core/TaskQueue.h"

#include "ArhqenCognitionEngine/Core/Logger.h"

#include <exception>
#include <utility>

namespace am::core
{
    void TaskQueue::enqueue(std::string name, Task task)
    {
        tasks_.push_back(QueuedTask{std::move(name), std::move(task)});
    }

    bool TaskQueue::drain(Logger& logger)
    {
        bool ok = true;

        while (!tasks_.empty())
        {
            auto task = std::move(tasks_.front());
            tasks_.pop_front();

            logger.info("Task start: " + task.name);

            try
            {
                if (task.task)
                {
                    task.task();
                }

                ++executedCount_;
                logger.info("Task complete: " + task.name);
            }
            catch (const std::exception& ex)
            {
                ok = false;
                logger.error("Task failed: " + task.name + " | " + ex.what());
            }
            catch (...)
            {
                ok = false;
                logger.error("Task failed: " + task.name + " | unknown exception");
            }
        }

        return ok;
    }

    std::size_t TaskQueue::pendingCount() const
    {
        return tasks_.size();
    }

    std::size_t TaskQueue::executedCount() const
    {
        return executedCount_;
    }
}
