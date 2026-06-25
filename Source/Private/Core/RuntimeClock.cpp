#include "ArhqenCognitionEngine/Core/RuntimeClock.h"

namespace am::core
{
    void RuntimeClock::reset()
    {
        start_ = Clock::now();
        previous_ = start_;
        nextFrameIndex_ = 0;
        initialized_ = true;
    }

    FrameTiming RuntimeClock::tick()
    {
        if (!initialized_)
        {
            reset();
        }

        const auto now = Clock::now();
        const std::chrono::duration<double> delta = now - previous_;
        const std::chrono::duration<double> elapsed = now - start_;

        previous_ = now;

        FrameTiming timing;
        timing.frameIndex = nextFrameIndex_++;
        timing.deltaSeconds = delta.count();
        timing.elapsedSeconds = elapsed.count();
        return timing;
    }
}
