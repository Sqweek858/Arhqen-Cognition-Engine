#include "ArhqenCognitionEngine/Ui/D2D/D2DDiagnosticsOverlay.h"

#include <algorithm>
#include <utility>

namespace am::ui
{
    void D2DDiagnosticsOverlay::setRect(UiRect rect)
    {
        rect_ = rect;
    }

    void D2DDiagnosticsOverlay::setSnapshot(D2DDiagnosticsSnapshot snapshot)
    {
        snapshot_ = std::move(snapshot);
    }

    void D2DDiagnosticsOverlay::setVisible(bool visible)
    {
        visible_ = visible;
    }

    bool D2DDiagnosticsOverlay::visible() const
    {
        return visible_;
    }

    void D2DDiagnosticsOverlay::toggle()
    {
        visible_ = !visible_;
    }

    bool D2DDiagnosticsOverlay::onKeyDown(WPARAM key)
    {
        if (!visible_)
        {
            return false;
        }

        if (key == VK_ESCAPE || key == VK_F12)
        {
            visible_ = false;
            return true;
        }

        return false;
    }

    bool D2DDiagnosticsOverlay::onMouseDown(float x, float y)
    {
        if (!visible_)
        {
            return false;
        }

        if (!rect_.contains(x, y))
        {
            visible_ = false;
            return true;
        }

        return true;
    }

    void D2DDiagnosticsOverlay::render(D2DRenderContext& ctx)
    {
        if (!visible_)
        {
            return;
        }

        const UiRect scrim{0.0f, 0.0f, ctx.width, ctx.height};
        D2DWidgetUtils::fillRounded(ctx, scrim, 0.0f, ctx.brushes.panelDeep);
        D2DWidgetUtils::fillRounded(ctx, rect_, 24.0f, ctx.brushes.panelElevated, ctx.brushes.accentWarm, 1.6f);

        D2DWidgetUtils::drawTextEx(
            ctx,
            snapshot_.title.empty() ? L"Diagnostics" : snapshot_.title,
            FontRole::BodyStrong,
            rect_.inset({22.0f, 18.0f, 22.0f, rect_.height() - 48.0f}),
            ctx.brushes.text
        );

        D2DWidgetUtils::drawTextEx(
            ctx,
            L"Esc/F12 closes. This panel is local UI state, because apparently even rectangles need telemetry.",
            FontRole::Small,
            rect_.inset({22.0f, 46.0f, 22.0f, rect_.height() - 72.0f}),
            ctx.brushes.muted
        );

        UiRect rowArea = rect_.inset({18.0f, 84.0f, 18.0f, 160.0f});
        D2DWidgetUtils::fillRounded(ctx, rowArea, 16.0f, ctx.brushes.panelDeep, ctx.brushes.borderDim, 1.0f);

        float y = rowArea.top + 14.0f;
        const std::size_t rowCount = std::min<std::size_t>(snapshot_.rows.size(), 12);

        for (std::size_t i = 0; i < rowCount; ++i)
        {
            const auto& row = snapshot_.rows[i];
            UiRect r{rowArea.left + 12.0f, y, rowArea.right - 12.0f, y + 28.0f};

            if (i % 2 == 0)
            {
                D2DWidgetUtils::fillRounded(ctx, r, 8.0f, ctx.brushes.panel, nullptr, 0.0f);
            }

            D2DWidgetUtils::drawTextEx(ctx, row.first, FontRole::Small, r.inset({10.0f, 5.0f, r.width() * 0.48f, 3.0f}), ctx.brushes.muted);
            D2DWidgetUtils::drawTextEx(ctx, row.second, FontRole::Small, r.inset({r.width() * 0.44f, 5.0f, 10.0f, 3.0f}), ctx.brushes.text, DWRITE_TEXT_ALIGNMENT_TRAILING);
            y += 31.0f;
        }

        UiRect noteArea{rect_.left + 18.0f, rowArea.bottom + 12.0f, rect_.right - 18.0f, rect_.bottom - 18.0f};
        D2DWidgetUtils::fillRounded(ctx, noteArea, 16.0f, ctx.brushes.panelDeep, ctx.brushes.borderDim, 1.0f);
        D2DWidgetUtils::drawTextEx(ctx, L"NOTES", FontRole::Small, noteArea.inset({14.0f, 9.0f, 14.0f, noteArea.height() - 31.0f}), ctx.brushes.accentWarm);

        float noteY = noteArea.top + 38.0f;
        const std::size_t noteCount = std::min<std::size_t>(snapshot_.notes.size(), 6);

        for (std::size_t i = 0; i < noteCount; ++i)
        {
            UiRect n{noteArea.left + 14.0f, noteY, noteArea.right - 14.0f, noteY + 38.0f};
            D2DWidgetUtils::drawTextEx(ctx, snapshot_.notes[i], FontRole::Small, n, ctx.brushes.muted);
            noteY += 40.0f;
        }
    }
}
