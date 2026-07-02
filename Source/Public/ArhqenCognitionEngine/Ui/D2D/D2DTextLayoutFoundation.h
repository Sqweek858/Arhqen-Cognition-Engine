#pragma once

#include "ArhqenCognitionEngine/Ui/D2D/D2DRenderContext.h"

#include <cstdint>
#include <string>

namespace am::ui
{
    enum class D2DTextOverflowMode
    {
        Clip,
        Ellipsis,
        Wrap
    };

    struct D2DTextLayoutStats
    {
        std::uint64_t measureCount = 0;
        std::uint64_t drawCount = 0;
        std::uint64_t ellipsisCount = 0;
        std::uint64_t clipPushCount = 0;
        std::uint64_t cacheFriendlyLayoutCount = 0;
    };

    class D2DTextLayoutFoundation
    {
    public:
        static void ResetStats();
        static D2DTextLayoutStats Stats();

        static DWRITE_TEXT_METRICS Measure(D2DRenderContext& ctx, const std::wstring& text, FontRole role, UiRect rect, DWRITE_WORD_WRAPPING wrapping = DWRITE_WORD_WRAPPING_NO_WRAP);
        static std::wstring EllipsizeToFit(D2DRenderContext& ctx, const std::wstring& text, FontRole role, float width);
        static float SnapTextCoordinate(float value);
        static void Draw(D2DRenderContext& ctx, const std::wstring& text, FontRole role, UiRect rect, ID2D1Brush* brush, D2DTextOverflowMode overflow = D2DTextOverflowMode::Ellipsis, DWRITE_TEXT_ALIGNMENT align = DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT valign = DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
        static void PushClip(D2DRenderContext& ctx, UiRect rect);
        static void PopClip(D2DRenderContext& ctx);

    private:
        static D2DTextLayoutStats stats_;
    };
}
