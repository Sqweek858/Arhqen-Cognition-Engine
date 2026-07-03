#include "ArhqenCognitionEngine/Editor/Viewport/AceCameraSpeedModel.h"

#include <algorithm>
#include <cmath>

namespace am::editor
{
    namespace
    {
        constexpr double kWheelDeltaPerDetent = 120.0;
        constexpr double kBaseLog10Step = 0.075;
        constexpr double kMomentumDecaySeconds = 0.18;
        constexpr double kMomentumGain = 0.32;
        constexpr double kMaximumMomentum = 6.0;

        int directionOf(double value)
        {
            return value > 0.0 ? 1 : (value < 0.0 ? -1 : 0);
        }
    }

    CameraSpeedModel::CameraSpeedModel(double initialSpeed)
        : speed_(clampSpeed(initialSpeed))
    {
    }

    double CameraSpeedModel::clampSpeed(double value)
    {
        if (!std::isfinite(value))
        {
            return DefaultSpeed;
        }
        return std::clamp(value, MinimumSpeed, MaximumSpeed);
    }

    double CameraSpeedModel::normalizedLogPosition() const
    {
        const double minimumLog = std::log10(MinimumSpeed);
        const double maximumLog = std::log10(MaximumSpeed);
        return std::clamp((std::log10(clampSpeed(speed_)) - minimumLog) /
            (maximumLog - minimumLog), 0.0, 1.0);
    }

    double CameraSpeedModel::setSpeed(double value)
    {
        speed_ = clampSpeed(value);
        resetWheelMomentum();
        return speed_;
    }

    double CameraSpeedModel::applyWheelDelta(int wheelDelta, double timestampSeconds)
    {
        if (wheelDelta == 0)
        {
            return speed_;
        }

        const double detents = static_cast<double>(wheelDelta) / kWheelDeltaPerDetent;
        const int direction = directionOf(detents);
        double elapsed = kMomentumDecaySeconds * 4.0;
        if (hasWheelTimestamp_ && std::isfinite(timestampSeconds) &&
            timestampSeconds >= lastWheelTimestampSeconds_)
        {
            elapsed = timestampSeconds - lastWheelTimestampSeconds_;
        }

        if (direction != lastWheelDirection_ || elapsed >= kMomentumDecaySeconds * 4.0)
        {
            wheelMomentum_ = 0.0;
        }
        else
        {
            wheelMomentum_ *= std::exp(-elapsed / kMomentumDecaySeconds);
        }

        wheelMomentum_ = std::clamp(wheelMomentum_ + std::abs(detents), 0.0, kMaximumMomentum);
        const double acceleration = 1.0 + kMomentumGain * std::max(0.0, wheelMomentum_ - 1.0);
        const double nextLog = std::log10(clampSpeed(speed_)) + kBaseLog10Step * detents * acceleration;
        speed_ = clampSpeed(std::pow(10.0, std::clamp(nextLog,
            std::log10(MinimumSpeed), std::log10(MaximumSpeed))));

        if (std::isfinite(timestampSeconds))
        {
            lastWheelTimestampSeconds_ = timestampSeconds;
            hasWheelTimestamp_ = true;
        }
        else
        {
            hasWheelTimestamp_ = false;
        }
        lastWheelDirection_ = direction;
        return speed_;
    }

    void CameraSpeedModel::resetWheelMomentum()
    {
        wheelMomentum_ = 0.0;
        lastWheelTimestampSeconds_ = 0.0;
        lastWheelDirection_ = 0;
        hasWheelTimestamp_ = false;
    }
}
