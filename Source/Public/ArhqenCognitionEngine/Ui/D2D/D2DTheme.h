#pragma once

#include "ArhqenCognitionEngine/Ui/D2D/D2DUiTypes.h"

namespace am::ui
{
    struct D2DTheme
    {
        D2D1_COLOR_F backgroundTop = D2D1::ColorF(0.030f, 0.044f, 0.088f, 1.0f);
        D2D1_COLOR_F backgroundMid = D2D1::ColorF(0.050f, 0.072f, 0.138f, 1.0f);
        D2D1_COLOR_F backgroundBottom = D2D1::ColorF(0.078f, 0.104f, 0.190f, 1.0f);

        D2D1_COLOR_F panel = D2D1::ColorF(0.070f, 0.155f, 0.260f, 0.40f);
        D2D1_COLOR_F panelDeep = D2D1::ColorF(0.020f, 0.052f, 0.110f, 0.72f);
        D2D1_COLOR_F panelSoft = D2D1::ColorF(0.105f, 0.235f, 0.355f, 0.44f);
        D2D1_COLOR_F panelElevated = D2D1::ColorF(0.070f, 0.140f, 0.260f, 0.78f);

        D2D1_COLOR_F border = D2D1::ColorF(0.130f, 0.780f, 1.000f, 0.82f);
        D2D1_COLOR_F borderDim = D2D1::ColorF(0.080f, 0.350f, 0.560f, 0.58f);

        D2D1_COLOR_F text = D2D1::ColorF(0.760f, 0.985f, 1.000f, 1.0f);
        D2D1_COLOR_F textDim = D2D1::ColorF(0.430f, 0.800f, 0.940f, 0.92f);
        D2D1_COLOR_F muted = D2D1::ColorF(0.360f, 0.650f, 0.790f, 0.82f);

        D2D1_COLOR_F accent = D2D1::ColorF(0.100f, 0.950f, 1.000f, 1.0f);
        D2D1_COLOR_F accentBlue = D2D1::ColorF(0.260f, 0.580f, 1.000f, 1.0f);
        D2D1_COLOR_F accentWarm = D2D1::ColorF(1.000f, 0.640f, 0.220f, 1.0f);
        D2D1_COLOR_F danger = D2D1::ColorF(1.000f, 0.330f, 0.280f, 1.0f);

        D2D1_COLOR_F userBubble = D2D1::ColorF(0.070f, 0.310f, 0.740f, 0.78f);
        D2D1_COLOR_F assistantBubble = D2D1::ColorF(0.045f, 0.115f, 0.210f, 0.68f);
        D2D1_COLOR_F systemBubble = D2D1::ColorF(0.040f, 0.160f, 0.190f, 0.64f);

        D2D1_COLOR_F input = D2D1::ColorF(0.020f, 0.060f, 0.125f, 0.74f);
        D2D1_COLOR_F inputFocused = D2D1::ColorF(0.030f, 0.095f, 0.180f, 0.82f);

        D2D1_COLOR_F buttonTop = D2D1::ColorF(0.130f, 0.890f, 1.000f, 1.0f);
        D2D1_COLOR_F buttonBottom = D2D1::ColorF(0.100f, 0.290f, 0.880f, 1.0f);
        D2D1_COLOR_F buttonHoverTop = D2D1::ColorF(0.240f, 0.980f, 1.000f, 1.0f);
        D2D1_COLOR_F buttonHoverBottom = D2D1::ColorF(0.160f, 0.420f, 1.000f, 1.0f);

        UiLayoutMetrics metrics;
    };
}
