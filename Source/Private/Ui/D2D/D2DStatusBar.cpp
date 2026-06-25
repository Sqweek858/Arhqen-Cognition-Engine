#include "ArhqenCognitionEngine/Ui/D2D/D2DStatusBar.h"

#include <utility>

namespace am::ui
{
    void D2DStatusBar::setRect(UiRect rect)
    {
        rect_ = rect;
    }

    void D2DStatusBar::setText(std::wstring text)
    {
        text_ = std::move(text);
    }

    void D2DStatusBar::render(D2DRenderContext& ctx)
    {
        D2DWidgetUtils::fillRounded(ctx, rect_, 12.0f, ctx.brushes.panelDeep, ctx.brushes.borderDim, 1.0f);

        UiRect dot{rect_.left + 14.0f, rect_.top + 9.0f, rect_.left + 24.0f, rect_.top + 19.0f};
        D2DWidgetUtils::fillRounded(ctx, dot, 5.0f, ctx.brushes.accent);

        D2DWidgetUtils::drawText(ctx, text_, ctx.fonts.small, rect_.inset({34.0f, 5.0f, 14.0f, 4.0f}), ctx.brushes.muted);
    }
}
