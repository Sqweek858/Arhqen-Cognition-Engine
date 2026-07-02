#include "ArhqenCognitionEngine/Ui/D2D/D2DTextLayoutFoundation.h"

#include "ArhqenCognitionEngine/Ui/D2D/D2DWidgetUtils.h"

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

namespace am::ui
{
    namespace
    {
        bool isHighSurrogate(wchar_t value)
        {
            return value >= static_cast<wchar_t>(0xD800) && value <= static_cast<wchar_t>(0xDBFF);
        }

        bool isLowSurrogate(wchar_t value)
        {
            return value >= static_cast<wchar_t>(0xDC00) && value <= static_cast<wchar_t>(0xDFFF);
        }

        std::vector<std::size_t> utf16PrefixBoundaries(const std::wstring& text)
        {
            std::vector<std::size_t> boundaries;
            boundaries.reserve(text.size() + 1);
            boundaries.push_back(0);
            for (std::size_t index = 0; index < text.size();)
            {
                if (isHighSurrogate(text[index]) && index + 1 < text.size() && isLowSurrogate(text[index + 1]))
                {
                    index += 2;
                }
                else
                {
                    ++index;
                }
                boundaries.push_back(index);
            }
            return boundaries;
        }
    }

    D2DTextLayoutStats D2DTextLayoutFoundation::stats_{};

    void D2DTextLayoutFoundation::ResetStats()
    {
        stats_ = {};
    }

    D2DTextLayoutStats D2DTextLayoutFoundation::Stats()
    {
        return stats_;
    }

    float D2DTextLayoutFoundation::SnapTextCoordinate(float value)
    {
        return std::isfinite(value) ? std::round(value) : 0.0f;
    }

    DWRITE_TEXT_METRICS D2DTextLayoutFoundation::Measure(D2DRenderContext& ctx, const std::wstring& text, FontRole role, UiRect rect, DWRITE_WORD_WRAPPING wrapping)
    {
        ++stats_.measureCount;
        TextLayoutOptions options;
        options.role = role;
        options.width = std::max(1.0f, rect.width());
        options.height = std::max(1.0f, rect.height());
        options.wrapping = wrapping;

        if (ctx.textCache && ctx.fontEngine)
        {
            ++stats_.cacheFriendlyLayoutCount;
            return ctx.textCache->getOrCreate(*ctx.fontEngine, text, options).metrics;
        }

        if (ctx.fontEngine)
        {
            return ctx.fontEngine->measure(text, options);
        }

        DWRITE_TEXT_METRICS metrics{};
        return metrics;
    }

    std::wstring D2DTextLayoutFoundation::EllipsizeToFit(D2DRenderContext& ctx, const std::wstring& text, FontRole role, float width)
    {
        if (text.empty() || width <= 1.0f)
        {
            return L"";
        }

        const UiRect measureRect = makeUiRect(0.0f, 0.0f, width, 4096.0f);
        const auto full = Measure(ctx, text, role, measureRect, DWRITE_WORD_WRAPPING_NO_WRAP);
        if (full.widthIncludingTrailingWhitespace <= width + 0.5f)
        {
            return text;
        }

        static constexpr wchar_t kEllipsis = L'\u2026';
        ++stats_.ellipsisCount;

        if (text.size() <= 1)
        {
            return std::wstring(1, kEllipsis);
        }

        const auto boundaries = utf16PrefixBoundaries(text);
        std::size_t lo = 0;
        std::size_t hi = boundaries.size() - 1;
        std::wstring best(1, kEllipsis);
        while (lo < hi)
        {
            const std::size_t mid = (lo + hi + 1) / 2;
            std::wstring candidate = text.substr(0, boundaries[mid]);
            candidate.push_back(kEllipsis);
            const auto m = Measure(ctx, candidate, role, measureRect, DWRITE_WORD_WRAPPING_NO_WRAP);
            if (m.widthIncludingTrailingWhitespace <= width + 0.5f)
            {
                best = std::move(candidate);
                lo = mid;
            }
            else
            {
                hi = mid - 1;
            }
        }
        return best;
    }

    void D2DTextLayoutFoundation::Draw(D2DRenderContext& ctx, const std::wstring& text, FontRole role, UiRect rect, ID2D1Brush* brush, D2DTextOverflowMode overflow, DWRITE_TEXT_ALIGNMENT align, DWRITE_PARAGRAPH_ALIGNMENT valign)
    {
        if (!ctx.target || !brush || rect.empty() || text.empty())
        {
            return;
        }

        ++stats_.drawCount;
        PushClip(ctx, rect);

        const std::wstring& toDraw = text;
        DWRITE_WORD_WRAPPING wrapping = DWRITE_WORD_WRAPPING_NO_WRAP;
        if (overflow == D2DTextOverflowMode::Wrap)
        {
            wrapping = DWRITE_WORD_WRAPPING_WRAP;
        }

        if (ctx.fontEngine)
        {
            TextLayoutOptions options;
            options.role = role;
            options.width = rect.width();
            options.height = rect.height();
            options.horizontal = align;
            options.vertical = valign;
            options.wrapping = wrapping;
            options.trimEnd = overflow == D2DTextOverflowMode::Ellipsis;
            if (ctx.textCache)
            {
                const auto layout = ctx.textCache->getOrCreate(*ctx.fontEngine, toDraw, options);
                if (layout.valid && layout.layout)
                {
                    ctx.target->DrawTextLayout(
                        D2D1::Point2F(SnapTextCoordinate(rect.left), SnapTextCoordinate(rect.top)),
                        layout.layout.Get(),
                        brush,
                        D2D1_DRAW_TEXT_OPTIONS_CLIP);
                    PopClip(ctx);
                    return;
                }
            }
            ctx.fontEngine->drawTextInRect(ctx.target, toDraw, options, rect, brush);
        }
        else
        {
            D2DWidgetUtils::drawTextEx(ctx, toDraw, role, rect, brush, align, valign);
        }

        PopClip(ctx);
    }

    void D2DTextLayoutFoundation::PushClip(D2DRenderContext& ctx, UiRect rect)
    {
        if (!ctx.target || rect.empty())
        {
            return;
        }
        ++stats_.clipPushCount;
        ctx.target->PushAxisAlignedClip(rect.d2d(), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    }

    void D2DTextLayoutFoundation::PopClip(D2DRenderContext& ctx)
    {
        if (!ctx.target)
        {
            return;
        }
        ctx.target->PopAxisAlignedClip();
    }
}
