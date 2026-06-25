#include "ArhqenCognitionEngine/Ui/D2D/D2DShortcutHelpOverlay.h"

#include <algorithm>
#include <utility>

namespace am::ui
{
    void D2DShortcutHelpOverlay::setRect(UiRect rect)
    {
        rect_ = rect;
    }

    void D2DShortcutHelpOverlay::setItems(std::vector<D2DShortcutHelpItem> items)
    {
        items_ = std::move(items);
    }

    void D2DShortcutHelpOverlay::setVisible(bool visible)
    {
        visible_ = visible;
    }

    bool D2DShortcutHelpOverlay::visible() const
    {
        return visible_;
    }

    void D2DShortcutHelpOverlay::toggle()
    {
        visible_ = !visible_;
    }

    bool D2DShortcutHelpOverlay::onKeyDown(WPARAM key)
    {
        if (!visible_)
        {
            return false;
        }

        if (key == VK_ESCAPE || key == VK_F1)
        {
            visible_ = false;
            return true;
        }

        return false;
    }

    bool D2DShortcutHelpOverlay::onMouseDown(float x, float y)
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

    void D2DShortcutHelpOverlay::render(D2DRenderContext& ctx)
    {
        if (!visible_)
        {
            return;
        }

        D2DWidgetUtils::fillRounded(ctx, makeUiRect(0.0f, 0.0f, ctx.width, ctx.height), 0.0f, ctx.brushes.panelDeep);
        D2DWidgetUtils::fillRounded(ctx, rect_, 24.0f, ctx.brushes.panelElevated, ctx.brushes.accentBlue, 1.6f);

        D2DWidgetUtils::drawTextEx(
            ctx,
            L"Shortcut Help",
            FontRole::BodyStrong,
            rect_.inset({24.0f, 18.0f, 24.0f, rect_.height() - 50.0f}),
            ctx.brushes.text
        );

        D2DWidgetUtils::drawTextEx(
            ctx,
            L"F1/Esc closes. Astonishingly, keyboard shortcuts are useful when the UI has more than three buttons.",
            FontRole::Small,
            rect_.inset({24.0f, 48.0f, 24.0f, rect_.height() - 76.0f}),
            ctx.brushes.muted
        );

        const auto groupNames = groups();
        const float gap = 14.0f;
        const float groupWidth = (rect_.width() - 48.0f - gap * std::max(0.0f, static_cast<float>(groupNames.size()) - 1.0f)) / std::max(1.0f, static_cast<float>(groupNames.size()));
        float x = rect_.left + 24.0f;

        for (const auto& group : groupNames)
        {
            std::vector<D2DShortcutHelpItem> groupItems;
            for (const auto& item : items_)
            {
                if (item.group == group)
                {
                    groupItems.push_back(item);
                }
            }

            UiRect groupRect{x, rect_.top + 88.0f, x + groupWidth, rect_.bottom - 24.0f};
            renderGroup(ctx, groupRect, group, groupItems);
            x += groupWidth + gap;
        }
    }

    void D2DShortcutHelpOverlay::renderGroup(D2DRenderContext& ctx, UiRect rect, const std::wstring& group, const std::vector<D2DShortcutHelpItem>& items)
    {
        D2DWidgetUtils::fillRounded(ctx, rect, 18.0f, ctx.brushes.panelDeep, ctx.brushes.borderDim, 1.0f);
        D2DWidgetUtils::drawTextEx(ctx, group, FontRole::Small, rect.inset({14.0f, 11.0f, 14.0f, rect.height() - 36.0f}), ctx.brushes.accentBlue);

        float y = rect.top + 44.0f;
        const std::size_t count = std::min<std::size_t>(items.size(), 10);

        for (std::size_t i = 0; i < count; ++i)
        {
            const auto& item = items[i];
            UiRect row{rect.left + 12.0f, y, rect.right - 12.0f, y + 36.0f};
            D2DWidgetUtils::fillRounded(ctx, row, 10.0f, i % 2 == 0 ? ctx.brushes.panel : ctx.brushes.panelElevated, nullptr, 0.0f);

            D2DWidgetUtils::drawTextEx(ctx, item.key, FontRole::Small, row.inset({10.0f, 8.0f, row.width() * 0.58f, 5.0f}), ctx.brushes.accent);
            D2DWidgetUtils::drawTextEx(ctx, item.action, FontRole::Small, row.inset({row.width() * 0.36f, 8.0f, 10.0f, 5.0f}), ctx.brushes.text, DWRITE_TEXT_ALIGNMENT_TRAILING);

            y += 40.0f;
            if (y > rect.bottom - 36.0f)
            {
                break;
            }
        }
    }

    std::vector<std::wstring> D2DShortcutHelpOverlay::groups() const
    {
        std::vector<std::wstring> result;

        for (const auto& item : items_)
        {
            if (std::find(result.begin(), result.end(), item.group) == result.end())
            {
                result.push_back(item.group);
            }
        }

        return result;
    }
}
