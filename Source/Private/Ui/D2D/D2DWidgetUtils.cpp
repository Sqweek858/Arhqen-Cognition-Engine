#include "ArhqenCognitionEngine/Ui/D2D/D2DWidgetUtils.h"

#include "ArhqenCognitionEngine/Ui/D2D/D2DCyberText.h"

namespace am::ui
{
    D2D1_ROUNDED_RECT D2DWidgetUtils::rounded(UiRect rect, float radius)
    {
        return D2D1::RoundedRect(rect.d2d(), radius, radius);
    }

    void D2DWidgetUtils::fillRounded(D2DRenderContext& ctx, UiRect rect, float radius, ID2D1Brush* fill, ID2D1Brush* stroke, float strokeWidth)
    {
        if (!ctx.target || !fill)
        {
            return;
        }

        const auto rr = rounded(rect, radius);
        ctx.target->FillRoundedRectangle(rr, fill);

        if (stroke)
        {
            ctx.target->DrawRoundedRectangle(rr, stroke, strokeWidth);
        }
    }

    void D2DWidgetUtils::fillRect(D2DRenderContext& ctx, UiRect rect, ID2D1Brush* fill)
    {
        if (!ctx.target || !fill)
        {
            return;
        }

        ctx.target->FillRectangle(rect.d2d(), fill);
    }

    void D2DWidgetUtils::drawText(D2DRenderContext& ctx, const std::wstring& text, IDWriteTextFormat* format, UiRect rect, ID2D1Brush* brush, DWRITE_TEXT_ALIGNMENT align, DWRITE_PARAGRAPH_ALIGNMENT valign)
    {
        if (!ctx.target || !format || !brush || text.empty())
        {
            return;
        }

        const auto cyberText = D2DCyberText::state();
        const float oldOpacity = brush->GetOpacity();

        if (cyberText.enabled)
        {
            const bool mutedBrush = brush == ctx.brushes.muted || brush == ctx.brushes.textDim;
            brush->SetOpacity(oldOpacity * (mutedBrush ? cyberText.mutedAlpha : cyberText.primaryAlpha));
            rect.left += cyberText.driftX;
            rect.right += cyberText.driftX;
            rect.top += cyberText.driftY;
            rect.bottom += cyberText.driftY;
        }

        format->SetTextAlignment(align);
        format->SetParagraphAlignment(valign);

        ctx.target->DrawTextW(
            text.c_str(),
            static_cast<UINT32>(text.size()),
            format,
            rect.d2d(),
            brush,
            D2D1_DRAW_TEXT_OPTIONS_CLIP
        );

        if (cyberText.enabled)
        {
            brush->SetOpacity(oldOpacity);
        }
    }

    void D2DWidgetUtils::drawTextEx(D2DRenderContext& ctx, const std::wstring& text, FontRole role, UiRect rect, ID2D1Brush* brush, DWRITE_TEXT_ALIGNMENT align, DWRITE_PARAGRAPH_ALIGNMENT valign)
    {
        if (!brush)
        {
            return;
        }

        const auto cyberText = D2DCyberText::state();
        const float oldOpacity = brush->GetOpacity();

        if (cyberText.enabled)
        {
            const bool mutedBrush = brush == ctx.brushes.muted || brush == ctx.brushes.textDim;
            brush->SetOpacity(oldOpacity * (mutedBrush ? cyberText.mutedAlpha : cyberText.primaryAlpha));
            rect.left += cyberText.driftX;
            rect.right += cyberText.driftX;
            rect.top += cyberText.driftY;
            rect.bottom += cyberText.driftY;
        }

        if (ctx.fontEngine)
        {
            TextLayoutOptions options;
            options.role = role;
            options.width = rect.width();
            options.height = rect.height();
            options.horizontal = align;
            options.vertical = valign;

            if (ctx.textCache)
            {
                const auto layout = ctx.textCache->getOrCreate(*ctx.fontEngine, text, options);
                if (layout.valid && layout.layout)
                {
                    ctx.target->DrawTextLayout(D2D1::Point2F(rect.left, rect.top), layout.layout.Get(), brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
                    if (cyberText.enabled)
                    {
                        brush->SetOpacity(oldOpacity);
                    }
                    return;
                }
            }

            ctx.fontEngine->drawTextInRect(ctx.target, text, options, rect, brush);
            if (cyberText.enabled)
            {
                brush->SetOpacity(oldOpacity);
            }
            return;
        }

        drawText(ctx, text, ctx.fonts.body, rect, brush, align, valign);

        if (cyberText.enabled)
        {
            brush->SetOpacity(oldOpacity);
        }
    }

    DWRITE_TEXT_METRICS D2DWidgetUtils::measureText(D2DRenderContext& ctx, const std::wstring& text, FontRole role, UiRect rect)
    {
        DWRITE_TEXT_METRICS metrics{};

        if (!ctx.fontEngine)
        {
            return metrics;
        }

        TextLayoutOptions options;
        options.role = role;
        options.width = rect.width();
        options.height = rect.height();

        if (ctx.textCache)
        {
            const auto layout = ctx.textCache->getOrCreate(*ctx.fontEngine, text, options);
            return layout.metrics;
        }

        return ctx.fontEngine->measure(text, options);
    }

    void D2DWidgetUtils::drawClippedText(D2DRenderContext& ctx, const std::wstring& text, IDWriteTextFormat* format, UiRect rect, ID2D1Brush* brush)
    {
        drawText(ctx, text, format, rect, brush, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    }

    void D2DWidgetUtils::drawSoftSeparator(D2DRenderContext& ctx, UiRect rect)
    {
        fillRounded(ctx, rect, 2.0f, ctx.brushes.borderDim);
    }

    void D2DWidgetUtils::drawMetricPill(D2DRenderContext& ctx, UiRect rect, const std::wstring& label, const std::wstring& value, ID2D1Brush* accent)
    {
        fillRounded(ctx, rect, 14.0f, ctx.brushes.panelDeep, ctx.brushes.borderDim, 1.0f);

        UiRect accentRect = rect;
        accentRect.right = accentRect.left + 5.0f;
        fillRounded(ctx, accentRect, 3.0f, accent);

        drawText(ctx, label, ctx.fonts.small, rect.inset({16.0f, 7.0f, rect.width() * 0.52f, 4.0f}), ctx.brushes.muted);
        drawText(ctx, value, ctx.fonts.small, rect.inset({rect.width() * 0.48f, 7.0f, 14.0f, 4.0f}), ctx.brushes.text, DWRITE_TEXT_ALIGNMENT_TRAILING);
    }

    bool D2DWidgetUtils::hitTest(UiRect rect, float x, float y)
    {
        return rect.contains(x, y);
    }
}
