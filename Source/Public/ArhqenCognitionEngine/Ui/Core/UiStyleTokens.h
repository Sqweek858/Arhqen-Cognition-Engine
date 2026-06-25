#pragma once

namespace am::ui
{
    struct UiColor
    {
        float r = 1.0f;
        float g = 1.0f;
        float b = 1.0f;
        float a = 1.0f;
    };

    struct UiSpacingTokens
    {
        float px0 = 0.0f;
        float px1 = 2.0f;
        float px2 = 4.0f;
        float px3 = 6.0f;
        float px4 = 8.0f;
        float px5 = 12.0f;
        float px6 = 16.0f;
        float px7 = 20.0f;
        float px8 = 24.0f;
        float px9 = 32.0f;
        float px10 = 40.0f;
    };

    struct UiRadiusTokens
    {
        float none = 0.0f;
        float small = 6.0f;
        float medium = 10.0f;
        float large = 14.0f;
        float xlarge = 18.0f;
        float pill = 999.0f;
    };

    struct UiFontScaleTokens
    {
        float tiny = 11.0f;
        float small = 12.0f;
        float body = 14.0f;
        float bodyStrong = 14.0f;
        float subtitle = 15.0f;
        float title = 18.0f;
        float display = 22.0f;
    };

    struct UiMotionTokens
    {
        float instant = 0.0f;
        float fast = 0.10f;
        float normal = 0.18f;
        float slow = 0.32f;
        float glowPulseSeconds = 2.4f;
        float hoverLiftPixels = 1.5f;
    };

    struct UiEffectTokens
    {
        float softShadowAlpha = 0.18f;
        float mediumShadowAlpha = 0.28f;
        float glowAlpha = 0.42f;
        float glowRadius = 18.0f;
        float borderGlowWidth = 1.2f;
        float backgroundGridAlpha = 0.055f;
        float scanlineAlpha = 0.030f;
        float blurRadiusFuture = 18.0f;
    };

    struct UiPaletteTokens
    {
        UiColor background0 {0.020f, 0.028f, 0.045f, 1.0f};
        UiColor background1 {0.035f, 0.050f, 0.082f, 1.0f};
        UiColor panel0 {0.045f, 0.060f, 0.096f, 1.0f};
        UiColor panel1 {0.060f, 0.080f, 0.128f, 1.0f};
        UiColor panel2 {0.082f, 0.104f, 0.164f, 1.0f};

        UiColor text {0.890f, 0.930f, 0.980f, 1.0f};
        UiColor textMuted {0.520f, 0.600f, 0.730f, 1.0f};
        UiColor textDim {0.350f, 0.430f, 0.560f, 1.0f};

        UiColor borderSoft {0.120f, 0.180f, 0.280f, 1.0f};
        UiColor borderActive {0.160f, 0.820f, 0.660f, 1.0f};

        UiColor accentGreen {0.110f, 0.920f, 0.630f, 1.0f};
        UiColor accentCyan {0.170f, 0.760f, 1.000f, 1.0f};
        UiColor accentBlue {0.220f, 0.410f, 0.920f, 1.0f};
        UiColor accentPurple {0.580f, 0.330f, 1.000f, 1.0f};

        UiColor danger {1.000f, 0.320f, 0.420f, 1.0f};
        UiColor warning {1.000f, 0.640f, 0.240f, 1.0f};
        UiColor success {0.180f, 0.920f, 0.560f, 1.0f};
    };

    struct UiCyberpunkThemeTokens
    {
        UiSpacingTokens spacing {};
        UiRadiusTokens radius {};
        UiFontScaleTokens fonts {};
        UiMotionTokens motion {};
        UiEffectTokens effects {};
        UiPaletteTokens palette {};
    };

    inline UiCyberpunkThemeTokens makeDefaultCyberpunkThemeTokens()
    {
        return {};
    }
}
