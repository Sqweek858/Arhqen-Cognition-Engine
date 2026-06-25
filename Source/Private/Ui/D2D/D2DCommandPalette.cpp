#include "ArhqenCognitionEngine/Ui/D2D/D2DCommandPalette.h"

#include <cwctype>

namespace am::ui
{
    namespace
    {
        bool printable(WPARAM wParam)
        {
            return wParam >= 32 && wParam != 127;
        }

        std::wstring lower(std::wstring text)
        {
            for (auto& ch : text)
            {
                ch = static_cast<wchar_t>(std::towlower(static_cast<wint_t>(ch)));
            }
            return text;
        }
    }

    void D2DCommandPalette::setRect(UiRect rect)
    {
        rect_ = rect;
        inputRect_ = rect_.inset({22.0f, 22.0f, 22.0f, rect_.height() - 74.0f});
        listRect_ = rect_.inset({18.0f, 88.0f, 18.0f, 18.0f});
    }

    void D2DCommandPalette::setItems(std::vector<D2DCommandPaletteItem> items)
    {
        items_ = std::move(items);
        selected_ = 0;
    }

    void D2DCommandPalette::open()
    {
        active_ = true;
        query_.clear();
        selected_ = 0;
        pendingCommand_.reset();
    }

    void D2DCommandPalette::close()
    {
        active_ = false;
        query_.clear();
        selected_ = 0;
        mousePressed_ = false;
    }

    void D2DCommandPalette::toggle()
    {
        if (active_)
        {
            close();
        }
        else
        {
            open();
        }
    }

    bool D2DCommandPalette::active() const
    {
        return active_;
    }

    bool D2DCommandPalette::onChar(WPARAM wParam)
    {
        if (!active_)
        {
            return false;
        }

        if (wParam == VK_BACK || wParam == VK_RETURN || wParam == VK_ESCAPE)
        {
            return false;
        }

        if (printable(wParam))
        {
            query_.push_back(static_cast<wchar_t>(wParam));
            selected_ = 0;
            return true;
        }

        return false;
    }

    bool D2DCommandPalette::onKeyDown(WPARAM wParam)
    {
        if (!active_)
        {
            return false;
        }

        switch (wParam)
        {
        case VK_ESCAPE:
            close();
            return true;
        case VK_BACK:
            if (!query_.empty())
            {
                query_.pop_back();
                selected_ = 0;
            }
            return true;
        case VK_UP:
            moveSelection(-1);
            return true;
        case VK_DOWN:
            moveSelection(1);
            return true;
        case VK_RETURN:
            acceptSelection();
            return true;
        default:
            return false;
        }
    }

    bool D2DCommandPalette::onMouseMove(float x, float y)
    {
        if (!active_ || !listRect_.contains(x, y))
        {
            return false;
        }

        const auto filtered = filteredIndexes();
        if (filtered.empty())
        {
            return false;
        }

        const float rowHeight = 56.0f;
        const std::size_t hovered = static_cast<std::size_t>((y - listRect_.top) / rowHeight);

        if (hovered < filtered.size() && hovered != selected_)
        {
            selected_ = hovered;
            return true;
        }

        return false;
    }

    bool D2DCommandPalette::onMouseDown(float x, float y)
    {
        if (!active_)
        {
            return false;
        }

        mousePressed_ = rect_.contains(x, y);
        if (!mousePressed_)
        {
            close();
            return true;
        }

        onMouseMove(x, y);
        return true;
    }

    bool D2DCommandPalette::onMouseUp(float x, float y)
    {
        if (!active_ || !mousePressed_)
        {
            mousePressed_ = false;
            return false;
        }

        mousePressed_ = false;

        if (listRect_.contains(x, y))
        {
            onMouseMove(x, y);
            acceptSelection();
            return true;
        }

        return false;
    }

    std::optional<std::wstring> D2DCommandPalette::takePendingCommand()
    {
        auto result = pendingCommand_;
        pendingCommand_.reset();
        return result;
    }

    void D2DCommandPalette::render(D2DRenderContext& ctx)
    {
        if (!active_)
        {
            return;
        }

        D2DWidgetUtils::fillRounded(ctx, makeUiRect(0.0f, 0.0f, ctx.width, ctx.height), 0.0f, ctx.brushes.panelDeep);
        D2DWidgetUtils::fillRounded(ctx, rect_, 24.0f, ctx.brushes.panelElevated, ctx.brushes.accent, 1.4f);

        D2DWidgetUtils::drawTextEx(
            ctx,
            L"Command Palette",
            FontRole::BodyStrong,
            rect_.inset({24.0f, 16.0f, 24.0f, rect_.height() - 48.0f}),
            ctx.brushes.text
        );

        D2DWidgetUtils::fillRounded(ctx, inputRect_, 14.0f, ctx.brushes.inputFocused, ctx.brushes.accent, 1.4f);

        const std::wstring queryDisplay = query_.empty() ? L"Type command name..." : query_;
        D2DWidgetUtils::drawTextEx(
            ctx,
            queryDisplay,
            FontRole::Body,
            inputRect_.inset({16.0f, 12.0f, 16.0f, 8.0f}),
            query_.empty() ? ctx.brushes.muted : ctx.brushes.text
        );

        const auto filtered = filteredIndexes();
        float y = listRect_.top;

        if (filtered.empty())
        {
            D2DWidgetUtils::drawTextEx(ctx, L"No commands match.", FontRole::Body, listRect_.inset(16.0f), ctx.brushes.muted);
            return;
        }

        const std::size_t maxRows = std::min<std::size_t>(filtered.size(), 6);

        for (std::size_t row = 0; row < maxRows; ++row)
        {
            const auto& item = items_[filtered[row]];
            UiRect rowRect{listRect_.left, y, listRect_.right, y + 52.0f};

            if (row == selected_)
            {
                D2DWidgetUtils::fillRounded(ctx, rowRect, 14.0f, ctx.brushes.panelSoft, ctx.brushes.accentBlue, 1.0f);
            }

            D2DWidgetUtils::drawTextEx(ctx, item.title, FontRole::BodyStrong, rowRect.inset({16.0f, 7.0f, 130.0f, 24.0f}), ctx.brushes.text);
            D2DWidgetUtils::drawTextEx(ctx, item.subtitle, FontRole::Small, rowRect.inset({16.0f, 30.0f, 130.0f, 4.0f}), ctx.brushes.muted);
            D2DWidgetUtils::drawTextEx(ctx, item.shortcut, FontRole::Small, rowRect.inset({rowRect.width() - 120.0f, 16.0f, 16.0f, 12.0f}), ctx.brushes.accent, DWRITE_TEXT_ALIGNMENT_TRAILING);

            y += 56.0f;
        }
    }

    std::vector<std::size_t> D2DCommandPalette::filteredIndexes() const
    {
        std::vector<std::size_t> result;

        for (std::size_t i = 0; i < items_.size(); ++i)
        {
            if (matches(items_[i]))
            {
                result.push_back(i);
            }
        }

        return result;
    }

    bool D2DCommandPalette::matches(const D2DCommandPaletteItem& item) const
    {
        if (query_.empty())
        {
            return true;
        }

        const auto needle = lower(query_);
        return lower(item.title).find(needle) != std::wstring::npos ||
            lower(item.subtitle).find(needle) != std::wstring::npos ||
            lower(item.id).find(needle) != std::wstring::npos;
    }

    void D2DCommandPalette::moveSelection(int delta)
    {
        const auto filtered = filteredIndexes();
        if (filtered.empty())
        {
            selected_ = 0;
            return;
        }

        const int count = static_cast<int>(filtered.size());
        int next = static_cast<int>(selected_) + delta;

        if (next < 0)
        {
            next = count - 1;
        }
        else if (next >= count)
        {
            next = 0;
        }

        selected_ = static_cast<std::size_t>(next);
    }

    void D2DCommandPalette::acceptSelection()
    {
        const auto filtered = filteredIndexes();
        if (filtered.empty())
        {
            return;
        }

        selected_ = std::min(selected_, filtered.size() - 1);
        pendingCommand_ = items_[filtered[selected_]].id;
        close();
    }
}
