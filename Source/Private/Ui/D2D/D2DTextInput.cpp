#include "ArhqenCognitionEngine/Ui/D2D/D2DTextInput.h"

#include <algorithm>
#include <cmath>
#include <cwctype>
#include <utility>

namespace am::ui
{
    namespace
    {
        bool isPrintableWide(WPARAM wParam)
        {
            return wParam >= 32 && wParam != 127;
        }
    }

    void D2DTextInput::setRect(UiRect rect)
    {
        rect_ = rect;
    }

    void D2DTextInput::setPlaceholder(std::wstring placeholder)
    {
        placeholder_ = std::move(placeholder);
    }

    void D2DTextInput::setText(std::wstring text)
    {
        text_ = std::move(text);
        caret_ = text_.size();
        selectionAnchor_ = caret_;
        scrollOffset_ = 0.0f;
    }

    const std::wstring& D2DTextInput::text() const
    {
        return text_;
    }

    bool D2DTextInput::empty() const
    {
        return text_.empty();
    }

    std::wstring D2DTextInput::takeText()
    {
        std::wstring result = text_;
        text_.clear();
        caret_ = 0;
        selectionAnchor_ = 0;
        scrollOffset_ = 0.0f;
        return result;
    }

    float D2DTextInput::preferredHeightForWidth(float width) const
    {
        const float usableWidth = std::max(120.0f, width - 46.0f);
        const float charsPerLine = std::max(12.0f, usableWidth / 8.2f);

        std::size_t hardLines = 1;
        std::size_t currentLineChars = 0;
        std::size_t visualLines = 1;

        for (wchar_t ch : text_)
        {
            if (ch == L'\n')
            {
                ++hardLines;
                ++visualLines;
                currentLineChars = 0;
                continue;
            }

            ++currentLineChars;
            if (static_cast<float>(currentLineChars) >= charsPerLine)
            {
                ++visualLines;
                currentLineChars = 0;
            }
        }

        const float lines = static_cast<float>(std::max<std::size_t>(hardLines, visualLines));
        return std::clamp(42.0f + (lines - 1.0f) * 18.0f, 42.0f, 60.0f);
    }

    void D2DTextInput::setFocused(bool focused)
    {
        focused_ = focused;
        if (!focused_)
        {
            clearSelection();
        }
    }

    bool D2DTextInput::focused() const
    {
        return focused_;
    }

    bool D2DTextInput::hitTest(float x, float y) const
    {
        return rect_.contains(x, y);
    }

    bool D2DTextInput::onMouseDown(D2DRenderContext& ctx, float x, float y)
    {
        if (!hitTest(x, y))
        {
            focused_ = false;
            scrollbarDragging_ = false;
            return false;
        }

        focused_ = true;

        UiRect activeTextRect = textRect(false);
        const bool overflow = hasScrollableOverflow(ctx, activeTextRect);
        activeTextRect = textRect(overflow);
        clampScroll(ctx, activeTextRect);

        if (overflow)
        {
            const UiRect thumb = scrollbarThumbRect(ctx, activeTextRect);
            const UiRect track = scrollbarTrackRect();

            if (thumb.contains(x, y))
            {
                scrollbarDragging_ = true;
                scrollbarDragGrabY_ = y;
                scrollbarDragStartOffset_ = scrollOffset_;
                return true;
            }

            if (track.contains(x, y))
            {
                const float direction = y < thumb.top ? -activeTextRect.height() : activeTextRect.height();
                scrollBy(ctx, activeTextRect, direction);
                return true;
            }
        }

        scrollbarDragging_ = false;

        if (ctx.fontEngine)
        {
            const float localX = x - activeTextRect.left;
            const float localY = y - activeTextRect.top + scrollOffset_;
            const auto hit = ctx.fontEngine->hitTestPoint(displayText(), layoutOptions(activeTextRect, std::max(activeTextRect.height(), contentHeight(ctx, activeTextRect))), localX, localY);
            setCaret(std::min<std::size_t>(text_.size(), hit.textPosition + (hit.trailingHit ? 1 : 0)), false);
        }
        else
        {
            const float approx = std::max(0.0f, x - activeTextRect.left);
            setCaret(std::min<std::size_t>(text_.size(), static_cast<std::size_t>(approx / 9.4f)), false);
        }

        return true;
    }

    bool D2DTextInput::onMouseWheel(D2DRenderContext& ctx, float x, float y, int wheelDelta)
    {
        if (!hitTest(x, y))
        {
            return false;
        }

        UiRect activeTextRect = textRect(false);
        const bool overflow = hasScrollableOverflow(ctx, activeTextRect);
        activeTextRect = textRect(overflow);

        if (!overflow)
        {
            return true;
        }

        scrollBy(ctx, activeTextRect, -static_cast<float>(wheelDelta) * 0.32f);
        return true;
    }


    bool D2DTextInput::onMouseMove(D2DRenderContext& ctx, float x, float y)
    {
        (void)x;

        if (!scrollbarDragging_)
        {
            return false;
        }

        UiRect activeTextRect = textRect(false);
        const bool overflow = hasScrollableOverflow(ctx, activeTextRect);
        activeTextRect = textRect(overflow);

        if (!overflow)
        {
            scrollbarDragging_ = false;
            scrollOffset_ = 0.0f;
            return true;
        }

        const UiRect track = scrollbarTrackRect();
        const UiRect thumb = scrollbarThumbRect(ctx, activeTextRect);
        const float usableTrack = std::max(1.0f, track.height() - thumb.height());
        const float maxOffset = maxScrollOffset(ctx, activeTextRect);
        const float deltaY = y - scrollbarDragGrabY_;

        scrollOffset_ = std::clamp(scrollbarDragStartOffset_ + (deltaY / usableTrack) * maxOffset, 0.0f, maxOffset);
        return true;
    }

    bool D2DTextInput::onMouseUp(D2DRenderContext& ctx, float x, float y)
    {
        (void)ctx;
        (void)x;
        (void)y;

        if (!scrollbarDragging_)
        {
            return false;
        }

        scrollbarDragging_ = false;
        return true;
    }

    bool D2DTextInput::onChar(WPARAM wParam)
    {
        if (!focused_)
        {
            return false;
        }

        if (wParam == VK_BACK || wParam == VK_RETURN || wParam == VK_TAB)
        {
            return false;
        }

        if (isPrintableWide(wParam))
        {
            insertChar(static_cast<wchar_t>(wParam));
            return true;
        }

        return false;
    }

    bool D2DTextInput::onKeyDown(WPARAM wParam, bool ctrlDown, bool shiftDown)
    {
        if (!focused_)
        {
            return false;
        }

        if (ctrlDown)
        {
            switch (wParam)
            {
            case 'A':
                selectAll();
                return true;
            default:
                break;
            }
        }

        switch (wParam)
        {
        case VK_RETURN:
            if (shiftDown)
            {
                insertNewline();
                return true;
            }
            return false;
        case VK_BACK:
            backspace();
            return true;
        case VK_DELETE:
            deleteForward();
            return true;
        case VK_LEFT:
            moveLeft(shiftDown);
            return true;
        case VK_RIGHT:
            moveRight(shiftDown);
            return true;
        case VK_HOME:
            moveHome(shiftDown);
            return true;
        case VK_END:
            moveEnd(shiftDown);
            return true;
        case VK_UP:
            scrollOffset_ = std::max(0.0f, scrollOffset_ - 22.0f);
            return true;
        case VK_DOWN:
            scrollOffset_ += 22.0f;
            return true;
        case VK_PRIOR:
            scrollOffset_ = std::max(0.0f, scrollOffset_ - std::max(24.0f, rect_.height() - 16.0f));
            return true;
        case VK_NEXT:
            scrollOffset_ += std::max(24.0f, rect_.height() - 16.0f);
            return true;
        default:
            return false;
        }
    }

    void D2DTextInput::selectAll()
    {
        caret_ = text_.size();
        selectionAnchor_ = 0;
    }

    void D2DTextInput::clearSelection()
    {
        selectionAnchor_ = caret_;
    }

    bool D2DTextInput::hasSelection() const
    {
        return selectionAnchor_ != caret_;
    }

    TextSelectionRange D2DTextInput::selectionRange() const
    {
        return {std::min(selectionAnchor_, caret_), std::max(selectionAnchor_, caret_)};
    }

    std::wstring D2DTextInput::selectedText() const
    {
        const auto range = selectionRange();
        if (range.empty() || range.end > text_.size())
        {
            return {};
        }

        return text_.substr(range.start, range.length());
    }

    bool D2DTextInput::copySelectionToClipboard(HWND owner, std::string* error)
    {
        if (!hasSelection())
        {
            return false;
        }

        return D2DClipboard::writeText(owner, selectedText(), error);
    }

    bool D2DTextInput::cutSelectionToClipboard(HWND owner, std::string* error)
    {
        if (!copySelectionToClipboard(owner, error))
        {
            return false;
        }

        deleteSelection();
        return true;
    }

    bool D2DTextInput::pasteFromClipboard(HWND owner, std::string* error)
    {
        const auto text = D2DClipboard::readText(owner, error);
        if (!text.has_value())
        {
            return false;
        }

        insertText(sanitizePasteText(*text));
        return true;
    }

    void D2DTextInput::replaceAllText(std::wstring text)
    {
        setText(std::move(text));
        focused_ = true;
    }

    void D2DTextInput::render(D2DRenderContext& ctx)
    {
        const float strokeWidth = focused_ ? 2.0f : 1.0f;

        UiRect activeTextRect = textRect(false);
        const bool hasOverflow = hasScrollableOverflow(ctx, activeTextRect);
        activeTextRect = textRect(hasOverflow);
        const float measuredContentHeight = contentHeight(ctx, activeTextRect);

        // Important: do not call ensureCaretVisible() every paint.
        // That was snapping scrollOffset_ back to the caret and made the scrollbar look decorative.
        clampScroll(ctx, activeTextRect);

        D2DWidgetUtils::fillRounded(
            ctx,
            rect_,
            16.0f,
            focused_ ? ctx.brushes.inputFocused : ctx.brushes.input,
            focused_ ? ctx.brushes.accent : ctx.brushes.border,
            strokeWidth
        );

        const bool placeholderMode = text_.empty() && !focused_;

        ctx.target->PushAxisAlignedClip(activeTextRect.d2d(), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

        if (!placeholderMode)
        {
            renderSelection(ctx, activeTextRect);
        }

        UiRect drawRect = activeTextRect;
        drawRect.top -= scrollOffset_;
        drawRect.bottom = drawRect.top + std::max(activeTextRect.height(), measuredContentHeight + 4.0f);

        D2DWidgetUtils::drawTextEx(
            ctx,
            displayText(),
            FontRole::Body,
            drawRect,
            placeholderMode ? ctx.brushes.muted : ctx.brushes.text
        );

        if (focused_)
        {
            const auto hit = caretHit(ctx, activeTextRect);
            const float x = std::clamp(activeTextRect.left + hit.caretX, activeTextRect.left, activeTextRect.right - 2.0f);
            const float caretTop = activeTextRect.top + hit.caretY - scrollOffset_;
            const float caretBottom = caretTop + std::max(18.0f, hit.caretHeight);

            if (caretBottom >= activeTextRect.top && caretTop <= activeTextRect.bottom)
            {
                UiRect caretRect = makeUiRect(
                    x,
                    std::max(activeTextRect.top, caretTop),
                    x + 2.0f,
                    std::min(activeTextRect.bottom, caretBottom)
                );
                D2DWidgetUtils::fillRounded(ctx, caretRect, 1.0f, ctx.brushes.accent);
            }
        }

        ctx.target->PopAxisAlignedClip();

        if (hasOverflow)
        {
            const UiRect track = scrollbarTrackRect();
            const UiRect thumb = scrollbarThumbRect(ctx, activeTextRect);

            D2DWidgetUtils::fillRounded(ctx, track, 3.0f, ctx.brushes.panelDeep);
            D2DWidgetUtils::fillRounded(ctx, thumb, 3.0f, scrollbarDragging_ ? ctx.brushes.accent : ctx.brushes.border);
        }
    }

    void D2DTextInput::insertChar(wchar_t ch)
    {
        insertText(std::wstring(1, ch));
    }

    void D2DTextInput::insertNewline()
    {
        insertText(L"\n");
    }

    void D2DTextInput::insertText(const std::wstring& text)
    {
        if (text.empty())
        {
            return;
        }

        if (hasSelection())
        {
            deleteSelection();
        }

        caret_ = clampIndex(caret_, 0, text_.size());
        text_.insert(caret_, text);
        caret_ += text.size();
        selectionAnchor_ = caret_;
    }

    void D2DTextInput::backspace()
    {
        if (hasSelection())
        {
            deleteSelection();
            return;
        }

        if (caret_ == 0 || text_.empty())
        {
            return;
        }

        text_.erase(text_.begin() + static_cast<std::ptrdiff_t>(caret_ - 1));
        --caret_;
        selectionAnchor_ = caret_;
    }

    void D2DTextInput::deleteForward()
    {
        if (hasSelection())
        {
            deleteSelection();
            return;
        }

        if (caret_ >= text_.size())
        {
            return;
        }

        text_.erase(text_.begin() + static_cast<std::ptrdiff_t>(caret_));
        selectionAnchor_ = caret_;
    }

    void D2DTextInput::moveLeft(bool shiftDown)
    {
        if (caret_ == 0)
        {
            return;
        }

        beginSelectionIfNeeded();
        --caret_;

        if (!shiftDown)
        {
            selectionAnchor_ = caret_;
        }
    }

    void D2DTextInput::moveRight(bool shiftDown)
    {
        if (caret_ >= text_.size())
        {
            return;
        }

        beginSelectionIfNeeded();
        ++caret_;

        if (!shiftDown)
        {
            selectionAnchor_ = caret_;
        }
    }

    void D2DTextInput::moveHome(bool shiftDown)
    {
        beginSelectionIfNeeded();
        caret_ = 0;

        if (!shiftDown)
        {
            selectionAnchor_ = caret_;
        }
        scrollOffset_ = 0.0f;
    }

    void D2DTextInput::moveEnd(bool shiftDown)
    {
        beginSelectionIfNeeded();
        caret_ = text_.size();

        if (!shiftDown)
        {
            selectionAnchor_ = caret_;
        }
    }

    void D2DTextInput::deleteSelection()
    {
        const auto range = selectionRange();
        if (range.empty() || range.end > text_.size())
        {
            return;
        }

        text_.erase(range.start, range.length());
        caret_ = range.start;
        selectionAnchor_ = caret_;
    }

    void D2DTextInput::setCaret(std::size_t caret, bool keepSelection)
    {
        caret_ = clampIndex(caret, 0, text_.size());

        if (!keepSelection)
        {
            selectionAnchor_ = caret_;
        }
    }

    void D2DTextInput::beginSelectionIfNeeded()
    {
        if (!hasSelection())
        {
            selectionAnchor_ = caret_;
        }
    }

    std::wstring D2DTextInput::sanitizePasteText(std::wstring text) const
    {
        for (auto& ch : text)
        {
            if (ch == L'\r' || ch == L'\n' || ch == L'\t')
            {
                ch = L' ';
            }
        }

        return text;
    }

    std::wstring D2DTextInput::displayText() const
    {
        if (!text_.empty())
        {
            return text_;
        }

        return focused_ ? L"" : placeholder_;
    }

    UiRect D2DTextInput::textRect(bool reserveScrollbar) const
    {
        return rect_.inset({18.0f, 8.0f, reserveScrollbar ? 28.0f : 18.0f, 8.0f});
    }

    TextLayoutOptions D2DTextInput::layoutOptions(UiRect textRect, float heightOverride) const
    {
        TextLayoutOptions options;
        options.role = FontRole::Body;
        options.width = std::max(1.0f, textRect.width());
        options.height = std::max(1.0f, heightOverride > 0.0f ? heightOverride : textRect.height());
        options.horizontal = DWRITE_TEXT_ALIGNMENT_LEADING;
        options.vertical = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
        options.wrapping = DWRITE_WORD_WRAPPING_WRAP;
        return options;
    }

    float D2DTextInput::contentHeight(D2DRenderContext& ctx, UiRect textRect) const
    {
        const std::wstring text = displayText();
        if (text.empty())
        {
            return 20.0f;
        }

        const float charsPerLine = std::max(12.0f, textRect.width() / 8.2f);
        std::size_t visualLines = 1;
        std::size_t currentLineChars = 0;

        for (wchar_t ch : text)
        {
            if (ch == L'\n')
            {
                ++visualLines;
                currentLineChars = 0;
                continue;
            }

            ++currentLineChars;
            if (static_cast<float>(currentLineChars) >= charsPerLine)
            {
                ++visualLines;
                currentLineChars = 0;
            }
        }

        const float estimatedHeight = static_cast<float>(visualLines) * 22.0f;

        if (ctx.fontEngine)
        {
            const auto metrics = ctx.fontEngine->measure(text, layoutOptions(textRect, 4096.0f));
            return std::max(20.0f, std::max(metrics.height + 2.0f, estimatedHeight));
        }

        return std::max(20.0f, estimatedHeight);
    }

    float D2DTextInput::maxScrollOffset(D2DRenderContext& ctx, UiRect textRect) const
    {
        return std::max(0.0f, contentHeight(ctx, textRect) - textRect.height());
    }

    void D2DTextInput::clampScroll(D2DRenderContext& ctx, UiRect textRect) const
    {
        scrollOffset_ = std::clamp(scrollOffset_, 0.0f, maxScrollOffset(ctx, textRect));
    }

    void D2DTextInput::ensureCaretVisible(D2DRenderContext& ctx, UiRect textRect) const
    {
        if (!ctx.fontEngine || text_.empty())
        {
            return;
        }

        const auto hit = caretHit(ctx, textRect);
        const float caretTop = hit.caretY;
        const float caretBottom = hit.caretY + std::max(18.0f, hit.caretHeight);

        if (caretTop < scrollOffset_)
        {
            scrollOffset_ = caretTop;
        }
        else if (caretBottom > scrollOffset_ + textRect.height())
        {
            scrollOffset_ = caretBottom - textRect.height();
        }
    }

    TextHitResult D2DTextInput::caretHit(D2DRenderContext& ctx, UiRect textRect) const
    {
        if (ctx.fontEngine)
        {
            return ctx.fontEngine->hitTestTextPosition(
                displayText(),
                layoutOptions(textRect, std::max(textRect.height(), contentHeight(ctx, textRect))),
                static_cast<std::uint32_t>(caret_),
                false
            );
        }

        TextHitResult fallback;
        fallback.caretX = static_cast<float>(caret_) * 9.4f;
        fallback.caretY = 0.0f;
        fallback.caretHeight = 20.0f;
        return fallback;
    }


    UiRect D2DTextInput::scrollbarTrackRect() const
    {
        return makeUiRect(rect_.right - 12.0f, rect_.top + 8.0f, rect_.right - 6.0f, rect_.bottom - 8.0f);
    }

    UiRect D2DTextInput::scrollbarThumbRect(D2DRenderContext& ctx, UiRect activeTextRect) const
    {
        const UiRect track = scrollbarTrackRect();
        const float trackHeight = std::max(1.0f, track.height());
        const float measuredContentHeight = std::max(activeTextRect.height(), contentHeight(ctx, activeTextRect));
        const float maxOffset = std::max(0.0f, measuredContentHeight - activeTextRect.height());
        const float thumbHeight = std::clamp(trackHeight * (activeTextRect.height() / measuredContentHeight), 18.0f, trackHeight);
        const float normalized = maxOffset <= 0.0f ? 0.0f : scrollOffset_ / maxOffset;
        const float thumbTop = track.top + (trackHeight - thumbHeight) * normalized;

        return makeUiRect(track.left, thumbTop, track.right, thumbTop + thumbHeight);
    }

    bool D2DTextInput::hasScrollableOverflow(D2DRenderContext& ctx, UiRect activeTextRect) const
    {
        return contentHeight(ctx, activeTextRect) > activeTextRect.height() + 0.5f;
    }

    void D2DTextInput::scrollBy(D2DRenderContext& ctx, UiRect activeTextRect, float delta)
    {
        scrollOffset_ = std::clamp(scrollOffset_ + delta, 0.0f, maxScrollOffset(ctx, activeTextRect));
    }

    void D2DTextInput::renderSelection(D2DRenderContext& ctx, UiRect textRect)
    {
        if (!hasSelection() || !ctx.fontEngine)
        {
            return;
        }

        const auto range = selectionRange();
        const auto options = layoutOptions(textRect, std::max(textRect.height(), contentHeight(ctx, textRect)));

        const auto startHit = ctx.fontEngine->hitTestTextPosition(displayText(), options, static_cast<std::uint32_t>(range.start), false);
        const auto endHit = ctx.fontEngine->hitTestTextPosition(displayText(), options, static_cast<std::uint32_t>(range.end), false);

        const float left = textRect.left + std::min(startHit.caretX, endHit.caretX);
        const float right = textRect.left + std::max(startHit.caretX, endHit.caretX);
        const float top = textRect.top + std::min(startHit.caretY, endHit.caretY) - scrollOffset_;
        const float height = std::max(20.0f, std::max(startHit.caretHeight, endHit.caretHeight));

        UiRect selectionRect = makeUiRect(
            std::max(textRect.left, left),
            std::max(textRect.top, top),
            std::min(textRect.right, right + 2.0f),
            std::min(textRect.bottom, top + height)
        );

        if (!selectionRect.empty())
        {
            D2DWidgetUtils::fillRounded(ctx, selectionRect, 4.0f, ctx.brushes.accentBlue);
        }
    }
}
