#pragma once

#include "ArhqenCognitionEngine/Ui/D2D/D2DTheme.h"
#include "ArhqenCognitionEngine/Ui/D2D/DWriteTextCache.h"

// Windows/RPC headers can define `small` as a macro, which breaks D2DTextFormats::small.
#ifdef small
#undef small
#endif

namespace am::ui
{
    struct D2DTextFormats
    {
        IDWriteTextFormat* title = nullptr;
        IDWriteTextFormat* subtitle = nullptr;
        IDWriteTextFormat* body = nullptr;
        IDWriteTextFormat* bodyStrong = nullptr;
        IDWriteTextFormat* small = nullptr;
        IDWriteTextFormat* mono = nullptr;
        IDWriteTextFormat* button = nullptr;
    };

    struct D2DBrushes
    {
        ID2D1SolidColorBrush* text = nullptr;
        ID2D1SolidColorBrush* textDim = nullptr;
        ID2D1SolidColorBrush* muted = nullptr;
        ID2D1SolidColorBrush* panel = nullptr;
        ID2D1SolidColorBrush* panelDeep = nullptr;
        ID2D1SolidColorBrush* panelSoft = nullptr;
        ID2D1SolidColorBrush* panelElevated = nullptr;
        ID2D1SolidColorBrush* border = nullptr;
        ID2D1SolidColorBrush* borderDim = nullptr;
        ID2D1SolidColorBrush* accent = nullptr;
        ID2D1SolidColorBrush* accentBlue = nullptr;
        ID2D1SolidColorBrush* accentWarm = nullptr;
        ID2D1SolidColorBrush* danger = nullptr;
        ID2D1SolidColorBrush* userBubble = nullptr;
        ID2D1SolidColorBrush* assistantBubble = nullptr;
        ID2D1SolidColorBrush* systemBubble = nullptr;
        ID2D1SolidColorBrush* input = nullptr;
        ID2D1SolidColorBrush* inputFocused = nullptr;

        ID2D1LinearGradientBrush* backgroundGradient = nullptr;
        ID2D1LinearGradientBrush* accentGradient = nullptr;
        ID2D1LinearGradientBrush* buttonGradient = nullptr;
        ID2D1LinearGradientBrush* buttonHoverGradient = nullptr;
    };

    struct D2DRenderContext
    {
        ID2D1HwndRenderTarget* target = nullptr;
        D2DTheme* theme = nullptr;
        DWriteFontEngine* fontEngine = nullptr;
        DWriteTextCache* textCache = nullptr;
        D2DBrushes brushes;
        D2DTextFormats fonts;
        float width = 0.0f;
        float height = 0.0f;
        float dpiScale = 1.0f;
        UINT dpiX = 96;
        UINT dpiY = 96;
    };
}
