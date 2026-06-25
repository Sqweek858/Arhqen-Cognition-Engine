#include "ArhqenCognitionEngine/Ui/D2D/D2DInspectorPanel.h"

#include <algorithm>
#include <utility>

namespace am::ui
{
    void D2DInspectorPanel::setRect(UiRect rect)
    {
        rect_ = rect;

        const float pad = 16.0f;
        bodyRect_ = makeUiRect(rect_.left + pad, rect_.top + 76.0f, rect_.right - pad, rect_.top + 190.0f);
        propertiesRect_ = makeUiRect(rect_.left + pad, bodyRect_.bottom + 12.0f, rect_.right - pad, bodyRect_.bottom + 220.0f);
        relatedRect_ = makeUiRect(rect_.left + pad, propertiesRect_.bottom + 12.0f, rect_.right - pad, rect_.bottom - pad);

        propertyGrid_.setRect(propertiesRect_);
    }

    void D2DInspectorPanel::setRecord(am::core::AceUiInspectorRecord record)
    {
        record_ = std::move(record);
        propertyGrid_.setProperties(record_.properties);
    }

    void D2DInspectorPanel::render(D2DRenderContext& ctx)
    {
        D2DWidgetUtils::fillRounded(ctx, rect_, 24.0f, ctx.brushes.panel, ctx.brushes.border, 1.0f);

        D2DWidgetUtils::drawTextEx(
            ctx,
            record_.title.empty() ? L"Inspector" : record_.title,
            FontRole::BodyStrong,
            rect_.inset({18.0f, 16.0f, 18.0f, rect_.height() - 44.0f}),
            ctx.brushes.text
        );

        D2DWidgetUtils::drawTextEx(
            ctx,
            record_.subtitle.empty() ? L"Select an item from workspace or graph." : record_.subtitle,
            FontRole::Small,
            rect_.inset({18.0f, 42.0f, 18.0f, rect_.height() - 66.0f}),
            ctx.brushes.muted
        );

        D2DWidgetUtils::fillRounded(ctx, bodyRect_, 16.0f, ctx.brushes.panelDeep, ctx.brushes.borderDim, 1.0f);
        D2DWidgetUtils::drawTextEx(
            ctx,
            record_.body.empty() ? L"No selection yet. Click a concept/question/hypothesis in the workspace panel, or a node in the graph preview." : record_.body,
            FontRole::Small,
            bodyRect_.inset({14.0f, 12.0f, 14.0f, 12.0f}),
            record_.body.empty() ? ctx.brushes.muted : ctx.brushes.text
        );

        propertyGrid_.render(ctx);
        renderRelated(ctx, relatedRect_);
    }

    void D2DInspectorPanel::renderRelated(D2DRenderContext& ctx, UiRect rect)
    {
        D2DWidgetUtils::fillRounded(ctx, rect, 16.0f, ctx.brushes.panelDeep, ctx.brushes.borderDim, 1.0f);
        D2DWidgetUtils::drawTextEx(ctx, L"RELATED", FontRole::Small, rect.inset({14.0f, 9.0f, 14.0f, rect.height() - 31.0f}), ctx.brushes.accentBlue);

        if (record_.relatedItems.empty())
        {
            D2DWidgetUtils::drawTextEx(ctx, L"No related items resolved yet.", FontRole::Small, rect.inset({14.0f, 42.0f, 14.0f, 10.0f}), ctx.brushes.muted);
            return;
        }

        float y = rect.top + 38.0f;
        const std::size_t count = std::min<std::size_t>(record_.relatedItems.size(), 6);

        for (std::size_t i = 0; i < count; ++i)
        {
            const auto& item = record_.relatedItems[i];
            UiRect row{rect.left + 12.0f, y, rect.right - 12.0f, y + 42.0f};
            D2DWidgetUtils::fillRounded(ctx, row, 12.0f, ctx.brushes.panelElevated, nullptr, 0.0f);

            UiRect stripe{row.left + 4.0f, row.top + 6.0f, row.left + 8.0f, row.bottom - 6.0f};
            D2DWidgetUtils::fillRounded(ctx, stripe, 2.0f, accentBrush(ctx, item.accentIndex));

            D2DWidgetUtils::drawTextEx(ctx, item.title, FontRole::Small, row.inset({14.0f, 5.0f, 12.0f, 22.0f}), ctx.brushes.text);
            D2DWidgetUtils::drawTextEx(ctx, item.subtitle, FontRole::Small, row.inset({14.0f, 23.0f, 12.0f, 4.0f}), ctx.brushes.muted);

            y += 48.0f;
        }
    }

    ID2D1Brush* D2DInspectorPanel::accentBrush(D2DRenderContext& ctx, int accentIndex) const
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
