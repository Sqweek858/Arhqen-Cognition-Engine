#pragma once

#include <chrono>
#include <cstdint>

namespace am::core
{
    struct FrameTiming
    {
        std::uint64_t frameIndex = 0;
        double deltaSeconds = 0.0;
        double elapsedSeconds = 0.0;
    };

    class RuntimeClock
    {
    public:
        using Clock = std::chrono::steady_clock;

        void reset();
        FrameTiming tick();

    private:
        Clock::time_point start_{};
        Clock::time_point previous_{};
        std::uint64_t nextFrameIndex_ = 0;
        bool initialized_ = false;
    };
}
