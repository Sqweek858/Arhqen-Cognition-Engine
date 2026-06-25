#include "ArhqenCognitionEngine/Ui/D2D/D2DLayoutOverlay.h"

namespace am::ui
{
    void D2DLayoutOverlay::setRect(UiRect rect)
    {
        rect_ = rect;
    }

    void D2DLayoutOverlay::setProfile(D2DDockLayoutProfile profile)
    {
        profile_ = profile;
        profile_.clamp();
    }

    void D2DLayoutOverlay::setVisible(bool visible)
    {
        visible_ = visible;
    }

    bool D2DLayoutOverlay::visible() const
    {
        return visible_;
    }

    void D2DLayoutOverlay::toggle()
    {
        visible_ = !visible_;
    }

    bool D2DLayoutOverlay::onKeyDown(WPARAM key)
    {
        if (!visible_)
        {
            return false;
        }

        if (key == VK_ESCAPE)
        {
            visible_ = false;
            return true;
        }

        return false;
    }

    bool D2DLayoutOverlay::onMouseDown(float x, float y)
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

    void D2DLayoutOverlay::render(D2DRenderContext& ctx)
    {
        if (!visible_)
        {
            return;
        }

        D2DWidgetUtils::fillRounded(ctx, makeUiRect(0.0f, 0.0f, ctx.width, ctx.height), 0.0f, ctx.brushes.panelDeep);
        D2DWidgetUtils::fillRounded(ctx, rect_, 24.0f, ctx.brushes.panelElevated, ctx.brushes.accent, 1.6f);

        D2DWidgetUtils::drawTextEx(
            ctx,
            L"Layout Profile",
            FontRole::BodyStrong,
            rect_.inset({22.0f, 18.0f, 22.0f, rect_.height() - 48.0f}),
            ctx.brushes.text
        );

        D2DWidgetUtils::drawTextEx(
            ctx,
            L"Drag splitters to change ratios. Save/load/reset layout from toolbar or command palette.",
            FontRole::Small,
            rect_.inset({22.0f, 48.0f, 22.0f, rect_.height() - 76.0f}),
            ctx.brushes.muted
        );

        float y = rect_.top + 88.0f;
        const float rowHeight = 36.0f;

        renderRow(ctx, makeUiRect(rect_.left + 18.0f, y, rect_.right - 18.0f, y + rowHeight), L"sidebar width", std::to_wstring(static_cast<int>(profile_.sidebarWidth)), 0);
        y += rowHeight + 8.0f;
        renderRow(ctx, makeUiRect(rect_.left + 18.0f, y, rect_.right - 18.0f, y + rowHeight), L"workspace ratio", std::to_wstring(profile_.workspaceRatio), 1);
        y += rowHeight + 8.0f;
        renderRow(ctx, makeUiRect(rect_.left + 18.0f, y, rect_.right - 18.0f, y + rowHeight), L"inspector ratio", std::to_wstring(profile_.inspectorRatio), 2);
        y += rowHeight + 8.0f;
        renderRow(ctx, makeUiRect(rect_.left + 18.0f, y, rect_.right - 18.0f, y + rowHeight), L"active tab", profile_.activeWorkspaceTab, 0);
        y += rowHeight + 8.0f;
        renderRow(ctx, makeUiRect(rect_.left + 18.0f, y, rect_.right - 18.0f, y + rowHeight), L"sidebar collapsed", profile_.sidebarCollapsed ? L"true" : L"false", 3);
        y += rowHeight + 8.0f;
        renderRow(ctx, makeUiRect(rect_.left + 18.0f, y, rect_.right - 18.0f, y + rowHeight), L"workspace collapsed", profile_.workspaceCollapsed ? L"true" : L"false", 3);
        y += rowHeight + 8.0f;
        renderRow(ctx, makeUiRect(rect_.left + 18.0f, y, rect_.right - 18.0f, y + rowHeight), L"diagnostics pinned", profile_.diagnosticsPinned ? L"true" : L"false", 1);
    }

    void D2DLayoutOverlay::renderRow(D2DRenderContext& ctx, UiRect row, const std::wstring& name, const std::wstring& value, int accent)
    {
        ID2D1Brush* stripeBrush = ctx.brushes.accent;

        if (accent == 1) { stripeBrush = ctx.brushes.accentBlue; }
        if (accent == 2) { stripeBrush = ctx.brushes.accentWarm; }
        if (accent == 3) { stripeBrush = ctx.brushes.danger; }

        D2DWidgetUtils::fillRounded(ctx, row, 12.0f, ctx.brushes.panelDeep, ctx.brushes.borderDim, 1.0f);
        D2DWidgetUtils::fillRounded(ctx, makeUiRect(row.left + 6.0f, row.top + 7.0f, row.left + 10.0f, row.bottom - 7.0f), 2.0f, stripeBrush);
        D2DWidgetUtils::drawTextEx(ctx, name, FontRole::Small, row.inset({18.0f, 8.0f, row.width() * 0.50f, 5.0f}), ctx.brushes.muted);
        D2DWidgetUtils::drawTextEx(ctx, value, FontRole::Small, row.inset({row.width() * 0.42f, 8.0f, 12.0f, 5.0f}), ctx.brushes.text, DWRITE_TEXT_ALIGNMENT_TRAILING);
    }
}
