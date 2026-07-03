#pragma once

namespace am::editor
{
    // Logarithmic editor-camera speed with cadence-sensitive wheel acceleration.
    // The model owns no frame state: every result depends only on wheel-event time,
    // which keeps it deterministic and independent of render/UI frame rate.
    class CameraSpeedModel
    {
    public:
        static constexpr double MinimumSpeed = 0.0001;
        static constexpr double MaximumSpeed = 100000.0;
        static constexpr double DefaultSpeed = 5.25;

        explicit CameraSpeedModel(double initialSpeed = DefaultSpeed);

        double speed() const { return speed_; }
        double normalizedLogPosition() const;
        double wheelMomentum() const { return wheelMomentum_; }

        double setSpeed(double value);
        double applyWheelDelta(int wheelDelta, double timestampSeconds);
        void resetWheelMomentum();

        static double clampSpeed(double value);

    private:
        double speed_ = DefaultSpeed;
        double wheelMomentum_ = 0.0;
        double lastWheelTimestampSeconds_ = 0.0;
        int lastWheelDirection_ = 0;
        bool hasWheelTimestamp_ = false;
    };
}
