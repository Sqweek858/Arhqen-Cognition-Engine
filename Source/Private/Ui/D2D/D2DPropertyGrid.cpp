#include "ArhqenCognitionEngine/Ui/D2D/D2DPropertyGrid.h"

#include <algorithm>
#include <utility>

namespace am::ui
{
    void D2DPropertyGrid::setRect(UiRect rect)
    {
        rect_ = rect;
    }

    void D2DPropertyGrid::setProperties(std::vector<am::core::AceUiInspectorProperty> properties)
    {
        properties_ = std::move(properties);
    }

    void D2DPropertyGrid::render(D2DRenderContext& ctx)
    {
        D2DWidgetUtils::fillRounded(ctx, rect_, 16.0f, ctx.brushes.panelDeep, ctx.brushes.borderDim, 1.0f);
        D2DWidgetUtils::drawTextEx(ctx, L"PROPERTIES", FontRole::Small, rect_.inset({14.0f, 9.0f, 14.0f, rect_.height() - 31.0f}), ctx.brushes.accent);

        if (properties_.empty())
        {
            D2DWidgetUtils::drawTextEx(ctx, L"No properties selected.", FontRole::Small, rect_.inset({14.0f, 42.0f, 14.0f, 10.0f}), ctx.brushes.muted);
            return;
        }

        float y = rect_.top + 38.0f;
        const std::size_t count = std::min<std::size_t>(properties_.size(), 8);

        for (std::size_t i = 0; i < count; ++i)
        {
            const auto& prop = properties_[i];
            UiRect row{rect_.left + 12.0f, y, rect_.right - 12.0f, y + 34.0f};
            D2DWidgetUtils::fillRounded(ctx, row, 10.0f, ctx.brushes.panelElevated, nullptr, 0.0f);

            UiRect stripe{row.left + 4.0f, row.top + 6.0f, row.left + 7.0f, row.bottom - 6.0f};
            D2DWidgetUtils::fillRounded(ctx, stripe, 2.0f, accentBrush(ctx, prop.accentIndex));

            D2DWidgetUtils::drawTextEx(ctx, prop.name, FontRole::Small, row.inset({14.0f, 7.0f, row.width() * 0.50f, 4.0f}), ctx.brushes.muted);
            D2DWidgetUtils::drawTextEx(ctx, prop.value, FontRole::Small, row.inset({row.width() * 0.42f, 7.0f, 10.0f, 4.0f}), ctx.brushes.text, DWRITE_TEXT_ALIGNMENT_TRAILING);

            y += 39.0f;
            if (y > rect_.bottom - 34.0f)
            {
                break;
            }
        }
    }

    ID2D1Brush* D2DPropertyGrid::accentBrush(D2DRenderContext& ctx, int accentIndex) const
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
