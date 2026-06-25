#pragma once

#include "ArhqenCognitionEngine/Ui/D2D/D2DClipboard.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DWidgetUtils.h"

#include <optional>

namespace am::ui
{
    struct TextSelectionRange
    {
        std::size_t start = 0;
        std::size_t end = 0;

        bool empty() const { return start == end; }
        std::size_t length() const { return end > start ? end - start : 0; }
    };

    class D2DTextInput
    {
    public:
        void setRect(UiRect rect);
        void setPlaceholder(std::wstring placeholder);
        void setText(std::wstring text);
        const std::wstring& text() const;
        bool empty() const;
        std::wstring takeText();

        float preferredHeightForWidth(float width) const;

        void setFocused(bool focused);
        bool focused() const;
        bool hitTest(float x, float y) const;
        bool onMouseDown(D2DRenderContext& ctx, float x, float y);
        bool onMouseWheel(D2DRenderContext& ctx, float x, float y, int wheelDelta);
        bool onMouseMove(D2DRenderContext& ctx, float x, float y);
        bool onMouseUp(D2DRenderContext& ctx, float x, float y);

        bool onChar(WPARAM wParam);
        bool onKeyDown(WPARAM wParam, bool ctrlDown, bool shiftDown);

        void selectAll();
        void clearSelection();
        bool hasSelection() const;
        TextSelectionRange selectionRange() const;
        std::wstring selectedText() const;

        bool copySelectionToClipboard(HWND owner, std::string* error);
        bool cutSelectionToClipboard(HWND owner, std::string* error);
        bool pasteFromClipboard(HWND owner, std::string* error);
        void replaceAllText(std::wstring text);

        void render(D2DRenderContext& ctx);

    private:
        void insertChar(wchar_t ch);
        void insertNewline();
        void insertText(const std::wstring& text);
        void backspace();
        void deleteForward();
        void moveLeft(bool shiftDown);
        void moveRight(bool shiftDown);
        void moveHome(bool shiftDown);
        void moveEnd(bool shiftDown);
        void deleteSelection();
        void setCaret(std::size_t caret, bool keepSelection);
        void beginSelectionIfNeeded();
        std::wstring sanitizePasteText(std::wstring text) const;
        std::wstring displayText() const;

        UiRect textRect(bool reserveScrollbar) const;
        UiRect scrollbarTrackRect() const;
        UiRect scrollbarThumbRect(D2DRenderContext& ctx, UiRect activeTextRect) const;
        bool hasScrollableOverflow(D2DRenderContext& ctx, UiRect activeTextRect) const;
        void scrollBy(D2DRenderContext& ctx, UiRect activeTextRect, float delta);
        TextLayoutOptions layoutOptions(UiRect textRect, float heightOverride = 0.0f) const;
        float contentHeight(D2DRenderContext& ctx, UiRect textRect) const;
        float maxScrollOffset(D2DRenderContext& ctx, UiRect textRect) const;
        void clampScroll(D2DRenderContext& ctx, UiRect textRect) const;
        void ensureCaretVisible(D2DRenderContext& ctx, UiRect textRect) const;
        TextHitResult caretHit(D2DRenderContext& ctx, UiRect textRect) const;
        void renderSelection(D2DRenderContext& ctx, UiRect textRect);

        UiRect rect_{};
        std::wstring text_;
        std::wstring placeholder_ = L"Type a message...";
        std::size_t caret_ = 0;
        std::size_t selectionAnchor_ = 0;
        mutable float scrollOffset_ = 0.0f;
        bool focused_ = true;
        bool scrollbarDragging_ = false;
        float scrollbarDragGrabY_ = 0.0f;
        float scrollbarDragStartOffset_ = 0.0f;
    };
}
