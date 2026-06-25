#include "ArhqenCognitionEngine/Ui/D2D/D2DAutocompletePopup.h"

#include <algorithm>
#include <utility>

namespace am::ui
{
    void D2DAutocompletePopup::setRect(UiRect rect)
    {
        rect_ = rect;
    }

    void D2DAutocompletePopup::setSuggestions(std::vector<am::core::AceSuggestion> suggestions)
    {
        suggestions_ = std::move(suggestions);
        selected_ = std::min(selected_, suggestions_.empty() ? 0 : suggestions_.size() - 1);
        active_ = !suggestions_.empty();
    }

    void D2DAutocompletePopup::open()
    {
        active_ = !suggestions_.empty();
    }

    void D2DAutocompletePopup::close()
    {
        active_ = false;
        mousePressed_ = false;
        acceptedText_.reset();
    }

    bool D2DAutocompletePopup::active() const
    {
        return active_ && !suggestions_.empty();
    }

    bool D2DAutocompletePopup::onKeyDown(WPARAM key)
    {
        if (!active())
        {
            return false;
        }

        if (key == VK_ESCAPE)
        {
            close();
            return true;
        }

        if (key == VK_UP)
        {
            selected_ = selected_ == 0 ? suggestions_.size() - 1 : selected_ - 1;
            return true;
        }

        if (key == VK_DOWN)
        {
            selected_ = (selected_ + 1) % suggestions_.size();
            return true;
        }

        if (key == VK_TAB)
        {
            acceptedText_ = suggestions_[selected_].insertText;
            active_ = false;
            return true;
        }

        return false;
    }

    bool D2DAutocompletePopup::onMouseMove(float x, float y)
    {
        if (!active() || !rect_.contains(x, y))
        {
            return false;
        }

        const float rowHeight = 46.0f;
        const std::size_t index = static_cast<std::size_t>((y - rect_.top - 12.0f) / rowHeight);

        if (index < suggestions_.size() && index != selected_)
        {
            selected_ = index;
            return true;
        }

        return false;
    }

    bool D2DAutocompletePopup::onMouseDown(float x, float y)
    {
        if (!active())
        {
            return false;
        }

        mousePressed_ = rect_.contains(x, y);
        if (mousePressed_)
        {
            onMouseMove(x, y);
            return true;
        }

        return false;
    }

    bool D2DAutocompletePopup::onMouseUp(float x, float y)
    {
        if (!active() || !mousePressed_)
        {
            mousePressed_ = false;
            return false;
        }

        mousePressed_ = false;

        if (rect_.contains(x, y) && selected_ < suggestions_.size())
        {
            acceptedText_ = suggestions_[selected_].insertText;
            active_ = false;
            return true;
        }

        return false;
    }

    std::optional<std::wstring> D2DAutocompletePopup::takeAcceptedText()
    {
        auto result = acceptedText_;
        acceptedText_.reset();
        return result;
    }

    void D2DAutocompletePopup::render(D2DRenderContext& ctx)
    {
        if (!active())
        {
            return;
        }

        D2DWidgetUtils::fillRounded(ctx, rect_, 16.0f, ctx.brushes.panelElevated, ctx.brushes.accentBlue, 1.2f);
        D2DWidgetUtils::drawTextEx(ctx, L"AUTOCOMPLETE", FontRole::Small, rect_.inset({14.0f, 8.0f, 14.0f, rect_.height() - 28.0f}), ctx.brushes.accentBlue);

        float y = rect_.top + 34.0f;
        const std::size_t count = std::min<std::size_t>(suggestions_.size(), 5);

        for (std::size_t i = 0; i < count; ++i)
        {
            const auto& item = suggestions_[i];
            UiRect row{rect_.left + 10.0f, y, rect_.right - 10.0f, y + 40.0f};

            if (i == selected_)
            {
                D2DWidgetUtils::fillRounded(ctx, row, 12.0f, ctx.brushes.panelSoft, ctx.brushes.accentBlue, 1.0f);
            }

            UiRect stripe{row.left + 4.0f, row.top + 6.0f, row.left + 8.0f, row.bottom - 6.0f};
            D2DWidgetUtils::fillRounded(ctx, stripe, 2.0f, accentBrush(ctx, item.accentIndex));
            D2DWidgetUtils::drawTextEx(ctx, item.label, FontRole::Small, row.inset({16.0f, 5.0f, 14.0f, 20.0f}), ctx.brushes.text);
            D2DWidgetUtils::drawTextEx(ctx, item.detail, FontRole::Small, row.inset({16.0f, 22.0f, 14.0f, 3.0f}), ctx.brushes.muted);

            y += 46.0f;
        }
    }

    ID2D1Brush* D2DAutocompletePopup::accentBrush(D2DRenderContext& ctx, int accentIndex) const
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
