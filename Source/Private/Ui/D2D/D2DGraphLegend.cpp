#include "ArhqenCognitionEngine/Ui/D2D/D2DGraphLegend.h"

#include <algorithm>
#include <utility>

namespace am::ui
{
    void D2DGraphLegend::setRect(UiRect rect)
    {
        rect_ = rect;
    }

    void D2DGraphLegend::setItems(std::vector<D2DGraphLegendItem> items)
    {
        items_ = std::move(items);
    }

    void D2DGraphLegend::render(D2DRenderContext& ctx)
    {
        if (rect_.empty())
        {
            return;
        }

        D2DWidgetUtils::fillRounded(ctx, rect_, 12.0f, ctx.brushes.panelDeep, ctx.brushes.borderDim, 1.0f);
        D2DWidgetUtils::drawTextEx(ctx, L"LEGEND", FontRole::Small, rect_.inset({8.0f, 6.0f, 8.0f, rect_.height() - 24.0f}), ctx.brushes.muted);

        float y = rect_.top + 28.0f;
        for (std::size_t i = 0; i < std::min<std::size_t>(items_.size(), 4); ++i)
        {
            const auto& item = items_[i];
            UiRect row{rect_.left + 8.0f, y, rect_.right - 8.0f, y + 28.0f};
            D2DWidgetUtils::fillRounded(ctx, makeUiRect(row.left, row.top + 7.0f, row.left + 10.0f, row.top + 17.0f), 5.0f, accentBrush(ctx, item.accentIndex));
            D2DWidgetUtils::drawTextEx(ctx, item.label, FontRole::Small, row.inset({16.0f, 2.0f, 6.0f, 13.0f}), ctx.brushes.text);
            D2DWidgetUtils::drawTextEx(ctx, item.detail, FontRole::Small, row.inset({16.0f, 15.0f, 6.0f, 1.0f}), ctx.brushes.muted);
            y += 30.0f;
        }
    }

    ID2D1Brush* D2DGraphLegend::accentBrush(D2DRenderContext& ctx, int accentIndex) const
    {
        if (accentIndex == 1)
        {
            return ctx.brushes.accentBlue;
        }

        if (accentIndex == 2)
        {
            return ctx.brushes.accentWarm;
        }

        if (accentIndex == 3)
        {
            return ctx.brushes.danger;
        }

        return ctx.brushes.accent;
    }
}
