#pragma once

namespace am::ui
{
    struct D2DCyberTextState
    {
        bool enabled = false;
        float primaryAlpha = 1.0f;
        float mutedAlpha = 0.86f;
        float driftX = 0.0f;
        float driftY = 0.0f;
        float glowAlpha = 0.0f;
    };

    class D2DCyberText
    {
    public:
        static void setState(D2DCyberTextState state);
        static void reset();
        static D2DCyberTextState state();
    };
}
