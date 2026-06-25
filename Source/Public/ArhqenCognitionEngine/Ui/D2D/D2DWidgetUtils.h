#pragma once

#include "ArhqenCognitionEngine/Ui/D2D/D2DRenderContext.h"

namespace am::ui
{
    class D2DWidgetUtils
    {
    public:
        static D2D1_ROUNDED_RECT rounded(UiRect rect, float radius);
        static void fillRounded(D2DRenderContext& ctx, UiRect rect, float radius, ID2D1Brush* fill, ID2D1Brush* stroke = nullptr, float strokeWidth = 1.0f);
        static void fillRect(D2DRenderContext& ctx, UiRect rect, ID2D1Brush* fill);
        static void drawText(D2DRenderContext& ctx, const std::wstring& text, IDWriteTextFormat* format, UiRect rect, ID2D1Brush* brush, DWRITE_TEXT_ALIGNMENT align = DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT valign = DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
        static void drawTextEx(D2DRenderContext& ctx, const std::wstring& text, FontRole role, UiRect rect, ID2D1Brush* brush, DWRITE_TEXT_ALIGNMENT align = DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT valign = DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
        static DWRITE_TEXT_METRICS measureText(D2DRenderContext& ctx, const std::wstring& text, FontRole role, UiRect rect);
        static void drawClippedText(D2DRenderContext& ctx, const std::wstring& text, IDWriteTextFormat* format, UiRect rect, ID2D1Brush* brush);
        static void drawSoftSeparator(D2DRenderContext& ctx, UiRect rect);
        static void drawMetricPill(D2DRenderContext& ctx, UiRect rect, const std::wstring& label, const std::wstring& value, ID2D1Brush* accent);
        static bool hitTest(UiRect rect, float x, float y);
    };
}
