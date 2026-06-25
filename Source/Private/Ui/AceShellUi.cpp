#include "ArhqenCognitionEngine/Ui/AceShellUi.h"

#include "ArhqenCognitionEngine/Ui/Core/UiFramework.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DCyberEffects.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DCyberBackgroundField.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DCyberText.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DGlassEffects.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DBlurRuntime.h"

#include <Windowsx.h>

#include <algorithm>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <utility>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

namespace am::ui
{
    AceShellUi::~AceShellUi()
    {
        discardDeviceResources();
    }

    void AceShellUi::setSubmitHandler(SubmitHandler handler)
    {
        submitHandler_ = std::move(handler);
    }

    void AceShellUi::setSnapshotProvider(SnapshotProvider provider)
    {
        snapshotProvider_ = std::move(provider);
    }

    void AceShellUi::setSuggestionProvider(SuggestionProvider provider)
    {
        suggestionProvider_ = std::move(provider);
    }

    void AceShellUi::setInspectorProvider(InspectorProvider provider)
    {
        inspectorProvider_ = std::move(provider);
    }

    void AceShellUi::setLayoutProfilePath(std::filesystem::path path)
    {
        layoutProfilePath_ = std::move(path);
    }

    bool AceShellUi::create(HWND parent, int width, int height, std::string* error)
    {
        parent_ = parent;
        width_ = std::max(width, 760);
        height_ = std::max(height, 520);

        if (!createDeviceIndependentResources(error))
        {
            return false;
        }

        loadLayoutProfile();
        seedInitialState();
        refreshSnapshot();
        refreshSuggestions();
        layout(width_, height_);

        if (!createDeviceResources(error))
        {
            return false;
        }

        created_ = true;
        invalidate();
        return true;
    }

    void AceShellUi::layout(int width, int height)
    {
        width_ = std::max(width, 920);
        height_ = std::max(height, 620);

        const float appBarHeight = 58.0f;
        const float sidebarWidth = 272.0f;
        const float outerPad = 22.0f;
        const float contentPad = 26.0f;
        (void)contentPad;

        appTopBarRect_ = makeUiRect(0.0f, 0.0f, static_cast<float>(width_), appBarHeight);
        headerRect_ = appTopBarRect_;
        brandLogoRect_ = makeUiRect(22.0f, 13.0f, 54.0f, 45.0f);
        settingsButtonRect_ = makeUiRect(static_cast<float>(width_) - 122.0f, 12.0f, static_cast<float>(width_) - 22.0f, 46.0f);
        environmentButtonRect_ = makeUiRect(settingsButtonRect_.left - 150.0f, 12.0f, settingsButtonRect_.left - 12.0f, 46.0f);

        sidebarRect_ = makeUiRect(0.0f, appBarHeight, sidebarWidth, static_cast<float>(height_));
        newConversationRect_ = makeUiRect(22.0f, appBarHeight + 24.0f, sidebarWidth - 22.0f, appBarHeight + 66.0f);
        conversationListRect_ = makeUiRect(18.0f, newConversationRect_.bottom + 28.0f, sidebarWidth - 18.0f, static_cast<float>(height_) - 28.0f);

        const float contentLeft = sidebarWidth;
        const float contentTop = appBarHeight;
        const float contentWidth = static_cast<float>(width_) - contentLeft;
        const float contentHeight = static_cast<float>(height_) - contentTop;
        (void)contentHeight;

        const float chatMaxWidth = 860.0f;
        const float chatWidth = std::min(chatMaxWidth, std::max(520.0f, contentWidth - outerPad * 2.0f));
        const float chatLeft = contentLeft + (contentWidth - chatWidth) * 0.5f;
        const float chatRight = chatLeft + chatWidth;

        const float statusHeight = 0.0f;
        (void)statusHeight;
        const float sendSize = 46.0f;
        const float inputAvailableWidth = chatWidth - sendSize - 12.0f;
        const float inputHeight = input_.preferredHeightForWidth(inputAvailableWidth);
        const float inputBottom = static_cast<float>(height_) - 24.0f;
        const float inputTop = inputBottom - inputHeight;

        mainRect_ = makeUiRect(
            chatLeft,
            contentTop + 26.0f,
            chatRight,
            static_cast<float>(height_) - 18.0f
        );

        toolbarRect_ = makeUiRect(0.0f, 0.0f, 0.0f, 0.0f);

        conversationRect_ = makeUiRect(
            chatLeft,
            contentTop + 32.0f,
            chatRight,
            inputTop - 18.0f
        );

        inputRect_ = makeUiRect(
            chatLeft,
            inputTop,
            chatRight - sendSize - 12.0f,
            inputBottom
        );

        sendCircleRect_ = makeUiRect(
            chatRight - sendSize,
            inputBottom - sendSize,
            chatRight,
            inputBottom
        );

        sendButtonRect_ = sendCircleRect_;

        // ACE-CLEAN0: no visible status bar. It looked cheap and clipped text for no useful reason.
        statusRect_ = makeUiRect(0.0f, 0.0f, 0.0f, 0.0f);

        // ACE-CLEAN0 minimal shell: advanced workspace regions are intentionally not visible.
        tabRect_ = makeUiRect(0.0f, 0.0f, 0.0f, 0.0f);
        workspaceRect_ = makeUiRect(0.0f, 0.0f, 0.0f, 0.0f);
        inspectorRect_ = makeUiRect(0.0f, 0.0f, 0.0f, 0.0f);
        rightSplitterRect_ = makeUiRect(0.0f, 0.0f, 0.0f, 0.0f);
        inspectorSplitterRect_ = makeUiRect(0.0f, 0.0f, 0.0f, 0.0f);
        layoutOverlayRect_ = makeUiRect(0.0f, 0.0f, 0.0f, 0.0f);

        autocompleteRect_ = makeUiRect(
            inputRect_.left,
            inputRect_.top - 190.0f,
            std::min(inputRect_.right, inputRect_.left + 460.0f),
            inputRect_.top - 10.0f
        );

        const float modalWidth = std::clamp(static_cast<float>(width_) * 0.50f, 480.0f, 760.0f);
        const float modalHeight = std::clamp(static_cast<float>(height_) * 0.50f, 320.0f, 520.0f);
        settingsModalRect_ = makeUiRect(
            (static_cast<float>(width_) - modalWidth) * 0.5f,
            (static_cast<float>(height_) - modalHeight) * 0.5f,
            (static_cast<float>(width_) + modalWidth) * 0.5f,
            (static_cast<float>(height_) + modalHeight) * 0.5f
        );

        settingsModalCloseRect_ = makeUiRect(
            settingsModalRect_.right - 46.0f,
            settingsModalRect_.top + 12.0f,
            settingsModalRect_.right - 14.0f,
            settingsModalRect_.top + 44.0f
        );

        // ACE-UI1R3: centered dashboard, not sidebar-relative modal math.
        // Keep enough room for the sidebar visually, but center the actual
        // Environment control surface in the app viewport.
        const float envWidth = std::clamp(static_cast<float>(width_) * 0.74f, 980.0f, static_cast<float>(width_) - 72.0f);
        const float envHeight = std::clamp(static_cast<float>(height_) * 0.76f, 620.0f, static_cast<float>(height_) - appTopBarRect_.bottom - 54.0f);
        const float envLeft = (static_cast<float>(width_) - envWidth) * 0.5f;
        const float envTop = appTopBarRect_.bottom + (static_cast<float>(height_) - appTopBarRect_.bottom - envHeight) * 0.5f;
        environmentModalRect_ = makeUiRect(
            std::max(24.0f, envLeft),
            std::max(appTopBarRect_.bottom + 18.0f, envTop),
            std::min(static_cast<float>(width_) - 24.0f, envLeft + envWidth),
            std::min(static_cast<float>(height_) - 24.0f, envTop + envHeight)
        );
        environmentModalCloseRect_ = makeUiRect(
            environmentModalRect_.right - 46.0f,
            environmentModalRect_.top + 12.0f,
            environmentModalRect_.right - 14.0f,
            environmentModalRect_.top + 44.0f
        );

        const float paletteWidth = std::min(680.0f, static_cast<float>(width_) - 140.0f);
        const float paletteLeft = (static_cast<float>(width_) - paletteWidth) * 0.5f;
        commandPalette_.setRect(makeUiRect(paletteLeft, 88.0f, paletteLeft + paletteWidth, 520.0f));

        const float diagnosticsWidth = std::min(700.0f, static_cast<float>(width_) - 140.0f);
        const float diagnosticsLeft = (static_cast<float>(width_) - diagnosticsWidth) * 0.5f;
        diagnosticsRect_ = makeUiRect(diagnosticsLeft, 92.0f, diagnosticsLeft + diagnosticsWidth, 620.0f);

        toastRect_ = makeUiRect(
            static_cast<float>(width_) - 400.0f,
            76.0f,
            static_cast<float>(width_) - 22.0f,
            static_cast<float>(height_) - 22.0f
        );

        const float helpWidth = std::min(820.0f, static_cast<float>(width_) - 140.0f);
        const float helpLeft = (static_cast<float>(width_) - helpWidth) * 0.5f;
        shortcutHelpRect_ = makeUiRect(helpLeft, 92.0f, helpLeft + helpWidth, 620.0f);

        conversationRowRects_.clear();
        conversationRenameRects_.clear();

        float rowY = conversationListRect_.top + 28.0f;
        const float rowHeight = 38.0f;
        const float rowGap = 8.0f;

        for (std::size_t i = 0; i < conversations_.size() && rowY + rowHeight <= conversationListRect_.bottom; ++i)
        {
            UiRect row = makeUiRect(conversationListRect_.left + 8.0f, rowY, conversationListRect_.right - 8.0f, rowY + rowHeight);
            conversationRowRects_.push_back(row);
            conversationRenameRects_.push_back(makeUiRect(row.right - 62.0f, row.top + 7.0f, row.right - 8.0f, row.bottom - 7.0f));
            rowY += rowHeight + rowGap;
        }

        messageList_.setRect(conversationRect_);
        input_.setRect(inputRect_);
        sendButton_.setRect(sendCircleRect_);
        sendButton_.setLabel(L"");
        sendButton_.setEnabled(true);
        statusBar_.setRect(statusRect_);
        autocomplete_.setRect(autocompleteRect_);
        diagnostics_.setRect(diagnosticsRect_);
        toastCenter_.setRect(toastRect_);
        shortcutHelp_.setRect(shortcutHelpRect_);
        layoutOverlay_.setRect(layoutOverlayRect_);

        sidebar_.setRect(sidebarRect_);
        workspaceTabs_.setRect(tabRect_);
        workspacePanel_.setRect(workspaceRect_);
        inspectorPanel_.setRect(inspectorRect_);
        workspaceSplitter_.setEnabled(false);
        inspectorSplitter_.setEnabled(false);

        if (renderTarget_)
        {
            renderTarget_->Resize(D2D1::SizeU(static_cast<UINT32>(width_), static_cast<UINT32>(height_)));
            createGradients(nullptr);
        }

        invalidate();
    }

    LRESULT AceShellUi::handleWindowMessage(UINT message, WPARAM wParam, LPARAM lParam, bool* handled)
    {
        if (handled)
        {
            *handled = false;
        }

        switch (message)
        {
        case WM_PAINT:
            if (handled) { *handled = true; }
            render();
            ValidateRect(parent_, nullptr);
            return 0;

        case WM_ERASEBKGND:
            if (handled) { *handled = true; }
            return 1;

        case WM_SIZE:
            handleResize(LOWORD(lParam), HIWORD(lParam));
            return 0;

        case WM_MOUSEWHEEL:
        {
            if (!commandPalette_.active())
            {
                const int wheel = GET_WHEEL_DELTA_WPARAM(wParam);
                POINT p;
                GetCursorPos(&p);
                ScreenToClient(parent_, &p);

                if (environmentOpen_ && environmentModalRect_.contains(static_cast<float>(p.x), static_cast<float>(p.y)))
                {
                    handleAquariumWheel(static_cast<float>(p.x), static_cast<float>(p.y), wheel);
                    invalidate();
                    if (handled) { *handled = true; }
                    return 0;
                }

                auto ctx = makeContext();
                if (input_.hitTest(static_cast<float>(p.x), static_cast<float>(p.y)))
                {
                    input_.onMouseWheel(ctx, static_cast<float>(p.x), static_cast<float>(p.y), wheel);
                }
                else
                {
                    messageList_.scrollBy(static_cast<float>(-wheel) * 0.42f);
                }

                invalidate();
            }
            if (handled) { *handled = true; }
            return 0;
        }

        case WM_LBUTTONDOWN:
        {
            const float x = static_cast<float>(GET_X_LPARAM(lParam));
            const float y = static_cast<float>(GET_Y_LPARAM(lParam));

            if (settingsOpen_)
            {
                if (settingsModalCloseRect_.contains(x, y))
                {
                    settingsOpen_ = false;
                    invalidate();
                    if (handled) { *handled = true; }
                    return 0;
                }

                if (settingsModalRect_.contains(x, y))
                {
                    if (handled) { *handled = true; }
                    return 0;
                }
            }

            if (environmentOpen_)
            {
                if (environmentModalCloseRect_.contains(x, y))
                {
                    environmentOpen_ = false;
                    invalidate();
                    if (handled) { *handled = true; }
                    return 0;
                }

                if (environmentModalRect_.contains(x, y))
                {
                    if (beginAquariumScrollbarDrag(x, y))
                    {
                        mouseCaptured_ = true;
                        SetCapture(parent_);
                        invalidate();
                        if (handled) { *handled = true; }
                        return 0;
                    }

                    if (handleAquariumPanelClick(x, y))
                    {
                        invalidate();
                    }

                    if (handled) { *handled = true; }
                    return 0;
                }
            }

            if (environmentButtonRect_.contains(x, y))
            {
                environmentOpen_ = true;
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }

            if (settingsButtonRect_.contains(x, y))
            {
                settingsOpen_ = true;
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }

            if (newConversationRect_.contains(x, y))
            {
                startNewConversation();
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }

            for (std::size_t i = 0; i < conversationRowRects_.size(); ++i)
            {
                if (i < conversationRenameRects_.size() && conversationRenameRects_[i].contains(x, y))
                {
                    prefillRenameConversation(i);
                    invalidate();
                    if (handled) { *handled = true; }
                    return 0;
                }

                if (conversationRowRects_[i].contains(x, y))
                {
                    selectConversation(i);
                    if (handled) { *handled = true; }
                    return 0;
                }
            }

            if (shortcutHelp_.visible())
            {
                shortcutHelp_.onMouseDown(x, y);
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }

            if (toastCenter_.onMouseDown(x, y))
            {
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }

            if (diagnostics_.visible())
            {
                diagnostics_.onMouseDown(x, y);
                toolbar_.setToggled(L"diagnostics", diagnostics_.visible());
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }

            if (commandPalette_.active())
            {
                commandPalette_.onMouseDown(x, y);
                focus_.set(D2DFocusTarget::CommandPalette);
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }

            if (autocomplete_.active() && autocomplete_.onMouseDown(x, y))
            {
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }

            auto ctx = makeContext();

            if (conversationRect_.contains(x, y))
            {
                if (auto messageSelection = messageList_.hitTest(ctx, x, y))
                {
                    inspectLocalMessage(messageSelection->id);
                    focus_.set(D2DFocusTarget::MessageList);
                    invalidate();
                    if (handled) { *handled = true; }
                    return 0;
                }
            }

            if (input_.onMouseDown(ctx, x, y))
            {
                focus_.set(D2DFocusTarget::TextInput);
                mouseCaptured_ = true;
                SetCapture(parent_);
            }
            else if (sendButton_.rect().contains(x, y))
            {
                focus_.set(D2DFocusTarget::SendButton);
            }
            else if (conversationRect_.contains(x, y))
            {
                focus_.set(D2DFocusTarget::MessageList);
            }

            if (sendButton_.onMouseDown(x, y))
            {
                mouseCaptured_ = true;
                SetCapture(parent_);
            }

            invalidate();
            if (handled) { *handled = true; }
            return 0;
        }

        case WM_LBUTTONUP:
        {
            const float x = static_cast<float>(GET_X_LPARAM(lParam));
            const float y = static_cast<float>(GET_Y_LPARAM(lParam));

            if (aquariumDraggingScroll_)
            {
                endAquariumScrollbarDrag();
                if (mouseCaptured_)
                {
                    mouseCaptured_ = false;
                    ReleaseCapture();
                }
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }

            if (commandPalette_.active())
            {
                commandPalette_.onMouseUp(x, y);
                if (auto command = commandPalette_.takePendingCommand())
                {
                    executeCommand(*command);
                }
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }

            if (autocomplete_.active() && autocomplete_.onMouseUp(x, y))
            {
                acceptAutocomplete();
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }

            {
                auto ctx = makeContext();
                if (input_.onMouseUp(ctx, x, y))
                {
                    if (mouseCaptured_)
                    {
                        mouseCaptured_ = false;
                        ReleaseCapture();
                    }

                    invalidate();
                    if (handled) { *handled = true; }
                    return 0;
                }
            }

            if (sendButton_.onMouseUp(x, y))
            {
                sendCurrentInput();
            }

            if (mouseCaptured_)
            {
                mouseCaptured_ = false;
                ReleaseCapture();
            }

            invalidate();
            if (handled) { *handled = true; }
            return 0;
        }

        case WM_MOUSEMOVE:
        {
            const float x = static_cast<float>(GET_X_LPARAM(lParam));
            const float y = static_cast<float>(GET_Y_LPARAM(lParam));

            if (aquariumDraggingScroll_)
            {
                updateAquariumScrollbarDrag(x, y);
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }

            bool changed = false;

            mouseX_ = x;
            mouseY_ = y;
            cyberBackground_.setMouse(x, y);

            const bool oldSettingsHovered = settingsHovered_;
            const bool oldEnvironmentHovered = environmentHovered_;
            const bool oldNewHovered = newConversationHovered_;
            const std::size_t oldHoveredConversation = hoveredConversationIndex_;

            settingsHovered_ = settingsButtonRect_.contains(x, y);
            environmentHovered_ = environmentButtonRect_.contains(x, y);
            newConversationHovered_ = newConversationRect_.contains(x, y);
            hoveredConversationIndex_ = kNoHoveredConversation;

            for (std::size_t i = 0; i < conversationRowRects_.size(); ++i)
            {
                if (conversationRowRects_[i].contains(x, y))
                {
                    hoveredConversationIndex_ = i;
                    break;
                }
            }

            changed = changed || oldSettingsHovered != settingsHovered_ || oldEnvironmentHovered != environmentHovered_ || oldNewHovered != newConversationHovered_ || oldHoveredConversation != hoveredConversationIndex_;

            if (commandPalette_.active())
            {
                changed = commandPalette_.onMouseMove(x, y);
            }
            else if (autocomplete_.active())
            {
                changed = autocomplete_.onMouseMove(x, y);
            }
            else
            {
                auto ctx = makeContext();
                if (input_.onMouseMove(ctx, x, y))
                {
                    changed = true;
                }
                else
                {
                    changed = sendButton_.onMouseMove(x, y);
                }
            }

            if (changed)
            {
                invalidate();
            }

            if (handled) { *handled = true; }
            return 0;
        }

        case WM_SETCURSOR:
            updateCursor();
            if (handled) { *handled = true; }
            return TRUE;

        case WM_CHAR:
            if (commandPalette_.active())
            {
                if (commandPalette_.onChar(wParam))
                {
                    invalidate();
                    if (handled) { *handled = true; }
                    return 0;
                }
            }
            else if (input_.onChar(wParam))
            {
                refreshSuggestions();
                layout(width_, height_);
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }
            break;

        case WM_KEYDOWN:
        {
            const auto keyboard = D2DKeyboardState::current();

            if (keyboard.ctrl && wParam == VK_UP)
            {
                if (auto previous = commandHistory_.previous())
                {
                    input_.replaceAllText(*previous);
                    refreshSuggestions();
                    statusBar_.setText(L"Command history: previous.");
                    layout(width_, height_);
                    invalidate();
                    if (handled) { *handled = true; }
                    return 0;
                }
            }

            if (keyboard.ctrl && wParam == VK_DOWN)
            {
                if (auto next = commandHistory_.next())
                {
                    input_.replaceAllText(*next);
                    refreshSuggestions();
                    statusBar_.setText(L"Command history: next.");
                    layout(width_, height_);
                    invalidate();
                    if (handled) { *handled = true; }
                    return 0;
                }
            }

            if (wParam == VK_F1)
            {
                shortcutHelp_.toggle();
                showToast(L"Help", shortcutHelp_.visible() ? L"Shortcut help opened." : L"Shortcut help closed.", D2DToastKind::Info);
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }

            if (shortcutHelp_.visible() && shortcutHelp_.onKeyDown(wParam))
            {
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }

            if (wParam == VK_F12)
            {
                diagnostics_.toggle();
                toolbar_.setToggled(L"diagnostics", diagnostics_.visible());
                refreshDiagnostics();
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }

            if (diagnostics_.visible() && diagnostics_.onKeyDown(wParam))
            {
                toolbar_.setToggled(L"diagnostics", diagnostics_.visible());
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }

            if (D2DKeyboard::isCtrlChord(wParam, L'K') || D2DKeyboard::isCtrlChord(wParam, L'P'))
            {
                openCommandPalette();
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }

            if (commandPalette_.active())
            {
                if (commandPalette_.onKeyDown(wParam))
                {
                    if (auto command = commandPalette_.takePendingCommand())
                    {
                        executeCommand(*command);
                    }

                    invalidate();
                    if (handled) { *handled = true; }
                    return 0;
                }
            }

            if (autocomplete_.active() && autocomplete_.onKeyDown(wParam))
            {
                acceptAutocomplete();
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }

            if (D2DKeyboard::isCtrlChord(wParam, L'C'))
            {
                std::string error;
                if (input_.copySelectionToClipboard(parent_, &error))
                {
                    statusBar_.setText(L"Copied selection to clipboard.");
                }
                else
                {
                    statusBar_.setText(L"Nothing selected to copy.");
                }
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }

            if (D2DKeyboard::isCtrlChord(wParam, L'X'))
            {
                std::string error;
                if (input_.cutSelectionToClipboard(parent_, &error))
                {
                    statusBar_.setText(L"Cut selection to clipboard.");
                }
                else
                {
                    statusBar_.setText(L"Nothing selected to cut.");
                }
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }

            if (D2DKeyboard::isCtrlChord(wParam, L'V'))
            {
                std::string error;
                if (input_.pasteFromClipboard(parent_, &error))
                {
                    refreshSuggestions();
                    statusBar_.setText(L"Pasted clipboard text into input.");
                }
                else
                {
                    statusBar_.setText(L"Paste failed or clipboard had no text.");
                }
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }

            if (wParam == VK_TAB)
            {
                if (keyboard.shift)
                {
                    focus_.focusPrevious();
                }
                else
                {
                    focus_.focusNext();
                }

                input_.setFocused(focus_.is(D2DFocusTarget::TextInput));
                statusBar_.setText(std::wstring(L"Focus: ") + focus_.name());
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }

            if (wParam == VK_RETURN && !keyboard.shift)
            {
                if (focus_.is(D2DFocusTarget::SendButton))
                {
                    sendCurrentInput();
                }
                else if (input_.focused())
                {
                    sendCurrentInput();
                }

                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }

            if (input_.onKeyDown(wParam, keyboard.ctrl, keyboard.shift))
            {
                refreshSuggestions();
                layout(width_, height_);
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }
            break;
        }

        default:
            break;
        }

        return 0;
    }

    bool AceShellUi::created() const
    {
        return created_;
    }

    void AceShellUi::tick(float dtSeconds)
    {
        messageList_.update(dtSeconds);
        toastCenter_.update(dtSeconds);
        cyberBackground_.update(dtSeconds);
        cyberTextTime_ += std::clamp(dtSeconds, 0.0f, 0.10f);

        if (aquariumControllerReady_)
        {
            aquariumController_.Tick(dtSeconds);
        }

        if (messageList_.animating())
        {
            invalidate();
        }

        if (diagnostics_.visible())
        {
            refreshDiagnostics();
        }

        if (toastCenter_.hasActiveToasts())
        {
            invalidate();
        }

        // ACE-CLEAN0: background, text pulse and hover glow are intentionally alive.
        invalidate();
    }

    bool AceShellUi::createDeviceIndependentResources(std::string* error)
    {
        HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, IID_PPV_ARGS(&d2dFactory_));
        if (FAILED(hr))
        {
            if (error) { *error = hresultToString("D2D1CreateFactory", hr); }
            return false;
        }

        hr = DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(IDWriteFactory),
            reinterpret_cast<IUnknown**>(dwriteFactory_.GetAddressOf())
        );

        if (FAILED(hr))
        {
            if (error) { *error = hresultToString("DWriteCreateFactory", hr); }
            return false;
        }

        if (!fontEngine_.initialize(dwriteFactory_.Get(), error))
        {
            return false;
        }

        return createTextFormats(error);
    }

    bool AceShellUi::createDeviceResources(std::string* error)
    {
        if (renderTarget_)
        {
            return true;
        }

        RECT rc{};
        GetClientRect(parent_, &rc);

        HRESULT hr = d2dFactory_->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(
                D2D1_RENDER_TARGET_TYPE_DEFAULT,
                D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_IGNORE),
                0.0f,
                0.0f
            ),
            D2D1::HwndRenderTargetProperties(
                parent_,
                D2D1::SizeU(static_cast<UINT32>(rc.right - rc.left), static_cast<UINT32>(rc.bottom - rc.top))
            ),
            &renderTarget_
        );

        if (FAILED(hr))
        {
            if (error) { *error = hresultToString("ID2D1Factory::CreateHwndRenderTarget", hr); }
            return false;
        }

        if (!createBrushes(error))
        {
            return false;
        }

        if (!createGradients(error))
        {
            return false;
        }

        return true;
    }

    bool AceShellUi::createBrushes(std::string* error)
    {
        auto createBrush = [&](const D2D1_COLOR_F& color, Microsoft::WRL::ComPtr<ID2D1SolidColorBrush>& brush) -> bool
        {
            const HRESULT hr = renderTarget_->CreateSolidColorBrush(color, &brush);
            if (FAILED(hr))
            {
                if (error) { *error = hresultToString("ID2D1RenderTarget::CreateSolidColorBrush", hr); }
                return false;
            }

            return true;
        };

        if (!createBrush(theme_.text, textBrush_)) { return false; }
        if (!createBrush(theme_.textDim, textDimBrush_)) { return false; }
        if (!createBrush(theme_.muted, mutedBrush_)) { return false; }
        if (!createBrush(theme_.panel, panelBrush_)) { return false; }
        if (!createBrush(theme_.panelDeep, panelDeepBrush_)) { return false; }
        if (!createBrush(theme_.panelSoft, panelSoftBrush_)) { return false; }
        if (!createBrush(theme_.panelElevated, panelElevatedBrush_)) { return false; }
        if (!createBrush(theme_.border, borderBrush_)) { return false; }
        if (!createBrush(theme_.borderDim, borderDimBrush_)) { return false; }
        if (!createBrush(theme_.accent, accentBrush_)) { return false; }
        if (!createBrush(theme_.accentBlue, accentBlueBrush_)) { return false; }
        if (!createBrush(theme_.accentWarm, accentWarmBrush_)) { return false; }
        if (!createBrush(theme_.danger, dangerBrush_)) { return false; }
        if (!createBrush(theme_.userBubble, userBubbleBrush_)) { return false; }
        if (!createBrush(theme_.assistantBubble, assistantBubbleBrush_)) { return false; }
        if (!createBrush(theme_.systemBubble, systemBubbleBrush_)) { return false; }
        if (!createBrush(theme_.input, inputBrush_)) { return false; }
        if (!createBrush(theme_.inputFocused, inputFocusedBrush_)) { return false; }

        return true;
    }

    bool AceShellUi::createGradients(std::string* error)
    {
        backgroundGradientBrush_.Reset();
        accentGradientBrush_.Reset();
        buttonGradientBrush_.Reset();
        buttonHoverGradientBrush_.Reset();

        Microsoft::WRL::ComPtr<ID2D1GradientStopCollection> backgroundStops;
        const D2D1_GRADIENT_STOP bgRaw[] =
        {
            {0.0f, theme_.backgroundTop},
            {0.48f, theme_.backgroundMid},
            {1.0f, theme_.backgroundBottom}
        };

        HRESULT hr = renderTarget_->CreateGradientStopCollection(bgRaw, 3, &backgroundStops);
        if (FAILED(hr))
        {
            if (error) { *error = hresultToString("CreateGradientStopCollection background", hr); }
            return false;
        }

        hr = renderTarget_->CreateLinearGradientBrush(
            D2D1::LinearGradientBrushProperties(
                D2D1::Point2F(0.0f, 0.0f),
                D2D1::Point2F(0.0f, static_cast<float>(height_))
            ),
            backgroundStops.Get(),
            &backgroundGradientBrush_
        );

        if (FAILED(hr))
        {
            if (error) { *error = hresultToString("CreateLinearGradientBrush background", hr); }
            return false;
        }

        Microsoft::WRL::ComPtr<ID2D1GradientStopCollection> accentStops;
        const D2D1_GRADIENT_STOP accentRaw[] =
        {
            {0.0f, theme_.accent},
            {1.0f, theme_.accentBlue}
        };

        hr = renderTarget_->CreateGradientStopCollection(accentRaw, 2, &accentStops);
        if (FAILED(hr))
        {
            if (error) { *error = hresultToString("CreateGradientStopCollection accent", hr); }
            return false;
        }

        hr = renderTarget_->CreateLinearGradientBrush(
            D2D1::LinearGradientBrushProperties(
                D2D1::Point2F(headerRect_.left, headerRect_.top),
                D2D1::Point2F(headerRect_.right, headerRect_.top)
            ),
            accentStops.Get(),
            &accentGradientBrush_
        );

        if (FAILED(hr))
        {
            if (error) { *error = hresultToString("CreateLinearGradientBrush accent", hr); }
            return false;
        }

        Microsoft::WRL::ComPtr<ID2D1GradientStopCollection> buttonStops;
        const D2D1_GRADIENT_STOP buttonRaw[] =
        {
            {0.0f, theme_.buttonTop},
            {1.0f, theme_.buttonBottom}
        };

        hr = renderTarget_->CreateGradientStopCollection(buttonRaw, 2, &buttonStops);
        if (FAILED(hr))
        {
            if (error) { *error = hresultToString("CreateGradientStopCollection button", hr); }
            return false;
        }

        hr = renderTarget_->CreateLinearGradientBrush(
            D2D1::LinearGradientBrushProperties(
                D2D1::Point2F(sendButtonRect_.left, sendButtonRect_.top),
                D2D1::Point2F(sendButtonRect_.left, sendButtonRect_.bottom)
            ),
            buttonStops.Get(),
            &buttonGradientBrush_
        );

        if (FAILED(hr))
        {
            if (error) { *error = hresultToString("CreateLinearGradientBrush button", hr); }
            return false;
        }

        Microsoft::WRL::ComPtr<ID2D1GradientStopCollection> hoverStops;
        const D2D1_GRADIENT_STOP hoverRaw[] =
        {
            {0.0f, theme_.buttonHoverTop},
            {1.0f, theme_.buttonHoverBottom}
        };

        hr = renderTarget_->CreateGradientStopCollection(hoverRaw, 2, &hoverStops);
        if (FAILED(hr))
        {
            if (error) { *error = hresultToString("CreateGradientStopCollection hover", hr); }
            return false;
        }

        hr = renderTarget_->CreateLinearGradientBrush(
            D2D1::LinearGradientBrushProperties(
                D2D1::Point2F(sendButtonRect_.left, sendButtonRect_.top),
                D2D1::Point2F(sendButtonRect_.left, sendButtonRect_.bottom)
            ),
            hoverStops.Get(),
            &buttonHoverGradientBrush_
        );

        if (FAILED(hr))
        {
            if (error) { *error = hresultToString("CreateLinearGradientBrush hover", hr); }
            return false;
        }

        return true;
    }

    bool AceShellUi::createTextFormats(std::string* error)
    {
        if (!createTextFormat(L"Segoe UI", 30.0f, DWRITE_FONT_WEIGHT_SEMI_BOLD, titleFormat_, error)) { return false; }
        if (!createTextFormat(L"Segoe UI", 15.0f, DWRITE_FONT_WEIGHT_NORMAL, subtitleFormat_, error)) { return false; }
        if (!createTextFormat(L"Segoe UI", 18.0f, DWRITE_FONT_WEIGHT_NORMAL, bodyFormat_, error)) { return false; }
        if (!createTextFormat(L"Segoe UI", 18.0f, DWRITE_FONT_WEIGHT_SEMI_BOLD, bodyStrongFormat_, error)) { return false; }
        if (!createTextFormat(L"Segoe UI", 14.0f, DWRITE_FONT_WEIGHT_NORMAL, smallFormat_, error)) { return false; }
        if (!createTextFormat(L"Cascadia Mono", 15.0f, DWRITE_FONT_WEIGHT_NORMAL, monoFormat_, error)) { return false; }
        if (!createTextFormat(L"Segoe UI", 18.0f, DWRITE_FONT_WEIGHT_SEMI_BOLD, buttonFormat_, error)) { return false; }

        if (buttonFormat_)
        {
            buttonFormat_->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            buttonFormat_->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }

        return true;
    }

    bool AceShellUi::createTextFormat(const wchar_t* family, float size, DWRITE_FONT_WEIGHT weight, Microsoft::WRL::ComPtr<IDWriteTextFormat>& format, std::string* error)
    {
        const HRESULT hr = dwriteFactory_->CreateTextFormat(
            family,
            nullptr,
            weight,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            size,
            L"ro-RO",
            &format
        );

        if (FAILED(hr))
        {
            if (error) { *error = hresultToString("IDWriteFactory::CreateTextFormat", hr); }
            return false;
        }

        format->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
        return true;
    }

    void AceShellUi::discardDeviceResources()
    {
        backgroundGradientBrush_.Reset();
        accentGradientBrush_.Reset();
        buttonGradientBrush_.Reset();
        buttonHoverGradientBrush_.Reset();

        textBrush_.Reset();
        textDimBrush_.Reset();
        mutedBrush_.Reset();
        panelBrush_.Reset();
        panelDeepBrush_.Reset();
        panelSoftBrush_.Reset();
        panelElevatedBrush_.Reset();
        borderBrush_.Reset();
        borderDimBrush_.Reset();
        accentBrush_.Reset();
        accentBlueBrush_.Reset();
        accentWarmBrush_.Reset();
        dangerBrush_.Reset();
        userBubbleBrush_.Reset();
        assistantBubbleBrush_.Reset();
        systemBubbleBrush_.Reset();
        inputBrush_.Reset();
        inputFocusedBrush_.Reset();

        renderTarget_.Reset();
    }

    D2DRenderContext AceShellUi::makeContext()
    {
        D2DRenderContext ctx;
        ctx.target = renderTarget_.Get();
        ctx.theme = &theme_;
        ctx.fontEngine = &fontEngine_;
        ctx.textCache = &textCache_;
        ctx.width = static_cast<float>(width_);
        ctx.height = static_cast<float>(height_);

        ctx.brushes.text = textBrush_.Get();
        ctx.brushes.textDim = textDimBrush_.Get();
        ctx.brushes.muted = mutedBrush_.Get();
        ctx.brushes.panel = panelBrush_.Get();
        ctx.brushes.panelDeep = panelDeepBrush_.Get();
        ctx.brushes.panelSoft = panelSoftBrush_.Get();
        ctx.brushes.panelElevated = panelElevatedBrush_.Get();
        ctx.brushes.border = borderBrush_.Get();
        ctx.brushes.borderDim = borderDimBrush_.Get();
        ctx.brushes.accent = accentBrush_.Get();
        ctx.brushes.accentBlue = accentBlueBrush_.Get();
        ctx.brushes.accentWarm = accentWarmBrush_.Get();
        ctx.brushes.danger = dangerBrush_.Get();
        ctx.brushes.userBubble = userBubbleBrush_.Get();
        ctx.brushes.assistantBubble = assistantBubbleBrush_.Get();
        ctx.brushes.systemBubble = systemBubbleBrush_.Get();
        ctx.brushes.input = inputBrush_.Get();
        ctx.brushes.inputFocused = inputFocusedBrush_.Get();
        ctx.brushes.backgroundGradient = backgroundGradientBrush_.Get();
        ctx.brushes.accentGradient = accentGradientBrush_.Get();
        ctx.brushes.buttonGradient = buttonGradientBrush_.Get();
        ctx.brushes.buttonHoverGradient = buttonHoverGradientBrush_.Get();

        ctx.fonts.title = titleFormat_.Get();
        ctx.fonts.subtitle = subtitleFormat_.Get();
        ctx.fonts.body = bodyFormat_.Get();
        ctx.fonts.bodyStrong = bodyStrongFormat_.Get();
        ctx.fonts.small = smallFormat_.Get();
        ctx.fonts.mono = monoFormat_.Get();
        ctx.fonts.button = buttonFormat_.Get();

        return ctx;
    }

    void AceShellUi::seedInitialState()
    {
        if (conversations_.empty())
        {
            conversations_.push_back(L"Conversation 1");
            conversationMessages_.push_back({});
            activeConversationIndex_ = 0;
            nextConversationNumber_ = 2;
        }

        std::vector<ChatMessage> messages;
        messageList_.setMessages(messages);
        if (!conversationMessages_.empty())
        {
            conversationMessages_[activeConversationIndex_] = messages;
        }

        input_.setPlaceholder(L"Command or note for Arhqen Cognition Engine...");
        input_.setText(L"");
        input_.setFocused(true);

        sendButton_.setLabel(L"");
        sendButton_.setEnabled(true);

        toolbar_.setItems({});

        commandPalette_.setItems({
            {L"backend_summary", L"Summary", L"Append backend summary to the chat.", L"/summary"},
            {L"backend_help", L"Help", L"Show supported commands.", L"/help"},
            {L"show_concepts", L"Concepts", L"Append concept list from backend.", L"/concepts"},
            {L"sample_concept", L"Insert concept command", L"Insert a parser command into the input field.", L""},
            {L"cache_stats", L"Cache stats", L"Append font/layout cache information.", L""},
            {L"seed_demo", L"Seed demo messages", L"Add messages for scroll testing.", L""},
            {L"focus_input", L"Focus input", L"Move keyboard focus back to input.", L"Tab"}
        });

        shortcutHelp_.setItems({
            {L"Ctrl+K / Ctrl+P", L"Command palette", L"Global"},
            {L"F1", L"Shortcut help", L"Global"},
            {L"F12", L"Diagnostics", L"Global"},
            {L"Esc", L"Close overlay", L"Global"},
            {L"Enter", L"Send", L"Input"},
            {L"/rename name", L"Rename active conversation", L"Workspaces"},
            {L"Mouse wheel", L"Scroll messages", L"Messages"}
        });

        focus_.set(D2DFocusTarget::TextInput);
        statusBar_.setText(L"");
    }

    void AceShellUi::render()
    {
        std::string error;
        if (!createDeviceResources(&error))
        {
            return;
        }

        PAINTSTRUCT ps{};
        BeginPaint(parent_, &ps);

        renderTarget_->BeginDraw();
        renderTarget_->SetTransform(D2D1::Matrix3x2F::Identity());

        auto ctx = makeContext();

        const float textPulse = 0.5f + 0.5f * std::sin(cyberTextTime_ * 2.10f);
        D2DCyberTextState cyberText;
        cyberText.enabled = true;
        cyberText.primaryAlpha = 0.92f + textPulse * 0.08f;
        cyberText.mutedAlpha = 0.72f + textPulse * 0.12f;
        cyberText.driftX = std::sin(cyberTextTime_ * 0.73f) * 0.22f;
        cyberText.driftY = std::cos(cyberTextTime_ * 0.61f) * 0.16f;
        cyberText.glowAlpha = 0.08f + textPulse * 0.10f;
        D2DCyberText::setState(cyberText);

        renderBackground(ctx);
        renderAppTopBar(ctx);
        renderSideNav(ctx);

        // ACE-UI1R2: while Environment is open, the chat/home layer stays behind
        // the frosted workspace overlay instead of bleeding through and colliding
        // with Logs / Episodes.
        if (!environmentOpen_)
        {
            renderMainPanel(ctx);
            messageList_.render(ctx);

            if (messageList_.size() == 0)
            {
                renderEmptyState(ctx);
            }

            input_.render(ctx);
            renderSendCircle(ctx);
            autocomplete_.render(ctx);
        }

        toastCenter_.render(ctx);
        diagnostics_.render(ctx);
        shortcutHelp_.render(ctx);
        commandPalette_.render(ctx);
        renderSettingsModal(ctx);
        renderEnvironmentPlaceholder(ctx);

        D2DCyberText::reset();

        const HRESULT hr = renderTarget_->EndDraw();

        if (hr == D2DERR_RECREATE_TARGET)
        {
            discardDeviceResources();
        }

        EndPaint(parent_, &ps);
    }

    void AceShellUi::renderBackground(D2DRenderContext& ctx)
    {
        D2DWidgetUtils::fillRect(ctx, makeUiRect(0.0f, 0.0f, ctx.width, ctx.height), ctx.brushes.backgroundGradient);

        const UiRect fieldRect = makeUiRect(0.0f, 0.0f, ctx.width, ctx.height);
        cyberBackground_.render(ctx, fieldRect);

        // ACE-CLEAN0: final lighter cyan wash. Subtle enough to keep contrast, visible enough to avoid the too-dark blanket.
        if (ctx.brushes.accentBlue)
        {
            ctx.brushes.accentBlue->SetOpacity(0.045f);
            ctx.target->FillRectangle(fieldRect.d2d(), ctx.brushes.accentBlue);
            ctx.brushes.accentBlue->SetOpacity(1.0f);
        }

        const UiCyberpunkThemeTokens tokens = makeDefaultCyberpunkThemeTokens();
        blurStatus_ = D2DBlurRuntime::evaluate(ctx, tokens.effects.blurRadiusFuture);
        D2DCyberEffects::drawBackgroundGrid(ctx, fieldRect, 42.0f, tokens.effects.backgroundGridAlpha * 1.85f);
        D2DCyberEffects::drawBackgroundGrid(ctx, fieldRect.inset(18.0f), 84.0f, tokens.effects.backgroundGridAlpha * 1.05f);
        D2DCyberEffects::drawScanlines(ctx, fieldRect, 7.0f, tokens.effects.scanlineAlpha * 0.88f);
    }

    void AceShellUi::renderHeader(D2DRenderContext& ctx)
    {
        renderAppTopBar(ctx);
    }

    void AceShellUi::renderMainPanel(D2DRenderContext& ctx)
    {
        // ACE-CLEAN0: no giant decorative chat shell. The message list and input are the shell.
        if (ctx.brushes.accent)
        {
            const float uplightPulse = 0.50f + 0.50f * std::sin(cyberTextTime_ * 1.20f);
            ctx.brushes.accent->SetOpacity(input_.focused() ? (0.075f + uplightPulse * 0.055f) : 0.035f);
            ctx.target->FillEllipse(D2D1::Ellipse(D2D1::Point2F(inputRect_.left + inputRect_.width() * 0.52f, inputRect_.top - 12.0f), inputRect_.width() * 0.48f, 74.0f), ctx.brushes.accent);
            ctx.brushes.accent->SetOpacity(1.0f);
        }

        D2DGlassMaterial inputGlass;
        inputGlass.radius = 16.0f;
        inputGlass.fillAlpha = input_.focused() ? 0.32f : 0.22f;
        inputGlass.borderAlpha = input_.focused() ? 0.72f : 0.32f;
        inputGlass.glowAlpha = input_.focused() ? 0.32f : 0.12f;
        inputGlass.shadowAlpha = 0.18f;
        inputGlass.blurFallbackAlpha = input_.focused() ? 0.14f : 0.08f;
        inputGlass.useCornerTicks = false;
        D2DGlassEffects::drawGlassPanel(ctx, inputRect_, inputGlass);

        if (input_.focused())
        {
            const float inputPulse = 0.50f + 0.50f * std::sin(cyberTextTime_ * 1.55f);
            D2DGlassEffects::drawCyanEdgeSweep(ctx, inputRect_, 16.0f, cyberTextTime_ * 1.15f, 0.16f + inputPulse * 0.18f);
        }

        D2DGlassEffects::drawDepthSeparator(ctx, makeUiRect(sidebarRect_.right, appTopBarRect_.bottom, sidebarRect_.right + 1.0f, ctx.height), 0.42f);
    }

    void AceShellUi::renderAppTopBar(D2DRenderContext& ctx)
    {
        D2DGlassMaterial topBarGlass;
        topBarGlass.radius = 0.0f;
        topBarGlass.fillAlpha = 0.24f;
        topBarGlass.borderAlpha = 0.28f;
        topBarGlass.glowAlpha = 0.09f;
        topBarGlass.shadowAlpha = 0.10f;
        topBarGlass.blurFallbackAlpha = 0.10f;
        topBarGlass.useCornerTicks = false;
        D2DGlassEffects::drawGlassPanel(ctx, appTopBarRect_, topBarGlass);
        D2DWidgetUtils::drawSoftSeparator(ctx, makeUiRect(0.0f, appTopBarRect_.bottom - 1.0f, ctx.width, appTopBarRect_.bottom));

        const float logoPulse = 0.55f + 0.45f * std::sin(cyberTextTime_ * 1.8f);
        D2DCyberEffects::drawBorderGlow(ctx, brandLogoRect_, 9.0f, 0.28f + logoPulse * 0.18f);
        D2DWidgetUtils::fillRounded(ctx, brandLogoRect_, 9.0f, ctx.brushes.accentGradient, ctx.brushes.borderDim, 1.0f);
        D2DWidgetUtils::drawTextEx(ctx, L"A", FontRole::BodyStrong, brandLogoRect_, ctx.brushes.text, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

        D2DWidgetUtils::drawTextEx(
            ctx,
            L"Arhqen Cognition Engine",
            FontRole::BodyStrong,
            makeUiRect(brandLogoRect_.right + 12.0f, appTopBarRect_.top + 14.0f, brandLogoRect_.right + 240.0f, appTopBarRect_.bottom - 12.0f),
            ctx.brushes.text
        );

        D2DCyberEffects::drawBorderGlow(ctx, environmentButtonRect_, 17.0f, environmentHovered_ ? 0.46f : 0.18f);
        D2DCyberEffects::drawCornerTicks(ctx, environmentButtonRect_, 8.0f, environmentHovered_ ? 0.56f : 0.26f);
        D2DWidgetUtils::fillRounded(ctx, environmentButtonRect_, 17.0f, ctx.brushes.panelElevated, ctx.brushes.borderDim, 1.0f);
        D2DWidgetUtils::drawTextEx(ctx, L"Environment", FontRole::Small, environmentButtonRect_, ctx.brushes.text, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

        D2DCyberEffects::drawBorderGlow(ctx, settingsButtonRect_, 17.0f, settingsHovered_ ? 0.42f : 0.16f);
        D2DCyberEffects::drawCornerTicks(ctx, settingsButtonRect_, 8.0f, settingsHovered_ ? 0.52f : 0.24f);
        D2DWidgetUtils::fillRounded(ctx, settingsButtonRect_, 17.0f, ctx.brushes.panel, ctx.brushes.borderDim, 1.0f);
        D2DWidgetUtils::drawTextEx(ctx, L"Settings", FontRole::Small, settingsButtonRect_, ctx.brushes.muted, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }

    void AceShellUi::renderSideNav(D2DRenderContext& ctx)
    {
        D2DGlassMaterial sidebarGlass;
        sidebarGlass.radius = 0.0f;
        sidebarGlass.fillAlpha = 0.26f;
        sidebarGlass.borderAlpha = 0.34f;
        sidebarGlass.glowAlpha = 0.12f;
        sidebarGlass.shadowAlpha = 0.12f;
        sidebarGlass.blurFallbackAlpha = 0.10f;
        sidebarGlass.useCornerTicks = false;
        D2DGlassEffects::drawGlassPanel(ctx, sidebarRect_, sidebarGlass);
        D2DWidgetUtils::drawSoftSeparator(ctx, makeUiRect(sidebarRect_.right - 1.0f, sidebarRect_.top, sidebarRect_.right, sidebarRect_.bottom));

        D2DGlassMaterial newChatGlass;
        newChatGlass.radius = 13.0f;
        newChatGlass.fillAlpha = newConversationHovered_ ? 0.46f : 0.30f;
        newChatGlass.borderAlpha = newConversationHovered_ ? 0.82f : 0.42f;
        newChatGlass.glowAlpha = newConversationHovered_ ? 0.40f : 0.16f;
        newChatGlass.shadowAlpha = 0.14f;
        newChatGlass.blurFallbackAlpha = 0.12f;
        newChatGlass.useCornerTicks = false;
        D2DGlassEffects::drawGlassPanel(ctx, newConversationRect_, newChatGlass);
        D2DWidgetUtils::drawTextEx(ctx, L"+  New workspace", FontRole::Small, newConversationRect_.inset({14.0f, 10.0f, 14.0f, 8.0f}), ctx.brushes.text);

        D2DWidgetUtils::drawTextEx(
            ctx,
            L"Workspaces",
            FontRole::Small,
            makeUiRect(conversationListRect_.left + 10.0f, conversationListRect_.top, conversationListRect_.right, conversationListRect_.top + 22.0f),
            ctx.brushes.muted
        );

        for (std::size_t i = 0; i < conversationRowRects_.size() && i < conversations_.size(); ++i)
        {
            const bool active = i == activeConversationIndex_;
            const UiRect row = conversationRowRects_[i];

            const bool hovered = i == hoveredConversationIndex_;
            if (active || hovered)
            {
                D2DCyberEffects::drawBorderGlow(ctx, row, 11.0f, active ? 0.34f : 0.22f);
            }

            D2DGlassMaterial rowGlass;
            rowGlass.radius = 11.0f;
            rowGlass.fillAlpha = active ? 0.42f : (hovered ? 0.34f : 0.18f);
            rowGlass.borderAlpha = active ? 0.80f : (hovered ? 0.60f : 0.20f);
            rowGlass.glowAlpha = active ? 0.30f : (hovered ? 0.20f : 0.06f);
            rowGlass.shadowAlpha = active ? 0.12f : 0.04f;
            rowGlass.blurFallbackAlpha = active ? 0.10f : 0.04f;
            rowGlass.useCornerTicks = false;
            D2DGlassEffects::drawGlassPanel(ctx, row, rowGlass);

            D2DWidgetUtils::drawTextEx(
                ctx,
                conversations_[i],
                FontRole::Small,
                row.inset({12.0f, 9.0f, 70.0f, 7.0f}),
                active ? ctx.brushes.text : ctx.brushes.muted
            );

            if (i < conversationRenameRects_.size())
            {
                const UiRect editRect = conversationRenameRects_[i];
                D2DWidgetUtils::fillRounded(ctx, editRect, 9.0f, ctx.brushes.panel, ctx.brushes.borderDim, 0.8f);
                D2DWidgetUtils::drawTextEx(ctx, L"rename", FontRole::Small, editRect, ctx.brushes.muted, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            }
        }
    }

    void AceShellUi::renderSendCircle(D2DRenderContext& ctx)
    {
        const float cx = (sendCircleRect_.left + sendCircleRect_.right) * 0.5f;
        const float cy = (sendCircleRect_.top + sendCircleRect_.bottom) * 0.5f;
        const float radius = std::min(sendCircleRect_.width(), sendCircleRect_.height()) * 0.5f;

        ID2D1Brush* fill = ctx.brushes.buttonGradient;

        if (sendButton_.pressed())
        {
            fill = ctx.brushes.accentBlue;
        }
        else if (sendButton_.hovered())
        {
            fill = ctx.brushes.buttonHoverGradient;
        }

        const float sendPulse = 0.55f + 0.45f * std::sin(cyberTextTime_ * 2.6f);
        const float sendGlow = sendButton_.hovered() ? 0.72f : (0.30f + sendPulse * 0.18f);
        D2DGlassEffects::drawBlurFallback(ctx, sendCircleRect_, radius, sendButton_.hovered() ? 0.28f : 0.16f);
        D2DCyberEffects::drawBorderGlow(ctx, sendCircleRect_, radius, sendGlow);
        ctx.target->FillEllipse(D2D1::Ellipse(D2D1::Point2F(cx, cy), radius, radius), fill);
        ctx.target->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(cx, cy), radius - 0.5f, radius - 0.5f), ctx.brushes.border, 1.0f);

        // Simple drawn send glyph: no text label, no font dependency, no Unicode lottery.
        const D2D1_POINT_2F tip = D2D1::Point2F(cx + 9.0f, cy);
        const D2D1_POINT_2F leftTop = D2D1::Point2F(cx - 8.0f, cy - 7.0f);
        const D2D1_POINT_2F leftBottom = D2D1::Point2F(cx - 8.0f, cy + 7.0f);
        const D2D1_POINT_2F mid = D2D1::Point2F(cx - 2.0f, cy);

        ctx.target->DrawLine(leftTop, tip, ctx.brushes.text, 2.0f);
        ctx.target->DrawLine(leftBottom, tip, ctx.brushes.text, 2.0f);
        ctx.target->DrawLine(leftTop, mid, ctx.brushes.text, 2.0f);
        ctx.target->DrawLine(leftBottom, mid, ctx.brushes.text, 2.0f);
    }


    void AceShellUi::renderEmptyState(D2DRenderContext& ctx)
    {
        const UiRect center = makeUiRect(
            conversationRect_.left + conversationRect_.width() * 0.5f - 222.0f,
            conversationRect_.top + conversationRect_.height() * 0.5f - 92.0f,
            conversationRect_.left + conversationRect_.width() * 0.5f + 222.0f,
            conversationRect_.top + conversationRect_.height() * 0.5f + 92.0f
        );

        const float pulse = 0.50f + 0.50f * std::sin(cyberTextTime_ * 1.45f);

        if (ctx.brushes.accentBlue)
        {
            ctx.brushes.accentBlue->SetOpacity(0.055f + pulse * 0.035f);
            ctx.target->FillEllipse(D2D1::Ellipse(D2D1::Point2F(center.left + center.width() * 0.5f, center.top + 46.0f), 138.0f, 58.0f), ctx.brushes.accentBlue);
            ctx.brushes.accentBlue->SetOpacity(1.0f);
        }

        UiRect orb = makeUiRect(center.left + center.width() * 0.5f - 30.0f, center.top + 8.0f, center.left + center.width() * 0.5f + 30.0f, center.top + 68.0f);
        D2DGlassEffects::drawBlurFallback(ctx, orb, 30.0f, 0.24f);
        D2DCyberEffects::drawBorderGlow(ctx, orb, 30.0f, 0.34f + pulse * 0.18f);

        if (ctx.brushes.accent)
        {
            ctx.brushes.accent->SetOpacity(0.68f + pulse * 0.22f);
            ctx.target->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(orb.left + 30.0f, orb.top + 30.0f), 24.0f, 18.0f), ctx.brushes.accent, 1.2f);
            ctx.brushes.accent->SetOpacity(1.0f);
        }

        D2DWidgetUtils::drawTextEx(ctx, L"Arhqen Cognition Engine", FontRole::Title, makeUiRect(center.left, center.top + 74.0f, center.right, center.top + 116.0f), ctx.brushes.text, DWRITE_TEXT_ALIGNMENT_CENTER);
        D2DWidgetUtils::drawTextEx(ctx, L"Ready for the next grounded cognition experiment.", FontRole::Body, makeUiRect(center.left, center.top + 118.0f, center.right, center.top + 146.0f), ctx.brushes.muted, DWRITE_TEXT_ALIGNMENT_CENTER);
        D2DWidgetUtils::drawTextEx(ctx, L"Use Environment for the future 3D sandbox placeholder.", FontRole::Small, makeUiRect(center.left, center.top + 150.0f, center.right, center.top + 178.0f), ctx.brushes.textDim, DWRITE_TEXT_ALIGNMENT_CENTER);
    }

    void AceShellUi::renderSettingsModal(D2DRenderContext& ctx)
    {
        if (!settingsOpen_)
        {
            return;
        }

        // ACE-UI1R9: stronger-but-still-subtle backdrop blur when a tab is open.
        D2DGlassEffects::drawGlassOverlay(ctx, makeUiRect(0.0f, 0.0f, ctx.width, ctx.height), 0.58f);
        D2DGlassEffects::drawBlurFallback(ctx, settingsModalRect_.inset(-18.0f), 28.0f, 0.18f);

        // ACE-UI1R2: no fake vignette bars. The overlay is a clean frosted
        // focus layer; the panel itself carries depth and glow.
        D2DGlassMaterial modalGlass;
        modalGlass.radius = 20.0f;
        modalGlass.fillAlpha = 0.46f;
        modalGlass.borderAlpha = 0.86f;
        modalGlass.highlightAlpha = 0.42f;
        modalGlass.glowAlpha = 0.42f;
        modalGlass.shadowAlpha = 0.48f;
        modalGlass.blurFallbackAlpha = 0.56f;
        modalGlass.useCornerTicks = true;
        D2DGlassEffects::drawGlassPanel(ctx, settingsModalRect_, modalGlass);

        const UiRect topBar = makeUiRect(settingsModalRect_.left, settingsModalRect_.top, settingsModalRect_.right, settingsModalRect_.top + 56.0f);
        D2DGlassMaterial modalTopGlass;
        modalTopGlass.radius = 20.0f;
        modalTopGlass.fillAlpha = 0.30f;
        modalTopGlass.borderAlpha = 0.36f;
        modalTopGlass.glowAlpha = 0.10f;
        modalTopGlass.shadowAlpha = 0.04f;
        modalTopGlass.blurFallbackAlpha = 0.08f;
        modalTopGlass.useCornerTicks = false;
        D2DGlassEffects::drawGlassPanel(ctx, topBar, modalTopGlass);
        D2DWidgetUtils::fillRounded(ctx, makeUiRect(settingsModalRect_.left + 18.0f, topBar.bottom - 3.0f, settingsModalRect_.left + 120.0f, topBar.bottom), 2.0f, ctx.brushes.accentGradient);

        D2DWidgetUtils::drawTextEx(
            ctx,
            L"Settings",
            FontRole::BodyStrong,
            makeUiRect(settingsModalRect_.left + 20.0f, settingsModalRect_.top + 14.0f, settingsModalRect_.right - 70.0f, settingsModalRect_.top + 44.0f),
            ctx.brushes.text
        );

        D2DWidgetUtils::fillRounded(ctx, settingsModalCloseRect_, 10.0f, ctx.brushes.panelDeep, ctx.brushes.borderDim, 1.0f);
        D2DWidgetUtils::drawTextEx(ctx, L"X", FontRole::Small, settingsModalCloseRect_, ctx.brushes.text, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

        D2DWidgetUtils::drawTextEx(
            ctx,
            L"Settings",
            FontRole::Body,
            settingsModalRect_.inset({20.0f, 78.0f, 20.0f, 78.0f}),
            ctx.brushes.text
        );

        D2DWidgetUtils::drawTextEx(
            ctx,
            L"Reserved for real options later.",
            FontRole::Small,
            settingsModalRect_.inset({20.0f, 114.0f, 20.0f, 24.0f}),
            ctx.brushes.muted
        );
    }



    std::wstring AceShellUi::widen(const std::string& text) const
    {
        return std::wstring(text.begin(), text.end());
    }

    bool AceShellUi::isAquariumButtonHovered(UiRect rect) const
    {
        return environmentOpen_ && rect.contains(mouseX_, mouseY_);
    }


    void AceShellUi::renderAquariumButton(D2DRenderContext& ctx, UiRect rect, const std::wstring& label, bool active)
    {
        const bool hovered = isAquariumButtonHovered(rect);
        const float glow = active ? 0.62f : (hovered ? 0.42f : 0.28f);
        D2DCyberEffects::drawBorderGlow(ctx, rect, 10.0f, glow);

        D2DGlassMaterial buttonGlass;
        buttonGlass.radius = 10.0f;
        buttonGlass.fillAlpha = active ? 0.42f : (hovered ? 0.34f : 0.26f);
        buttonGlass.borderAlpha = active ? 0.82f : (hovered ? 0.60f : 0.42f);
        buttonGlass.highlightAlpha = active ? 0.36f : (hovered ? 0.28f : 0.20f);
        buttonGlass.glowAlpha = active ? 0.46f : (hovered ? 0.30f : 0.20f);
        buttonGlass.shadowAlpha = hovered ? 0.12f : 0.08f;
        buttonGlass.blurFallbackAlpha = active ? 0.10f : (hovered ? 0.09f : 0.05f);
        buttonGlass.useCornerTicks = false;
        D2DGlassEffects::drawGlassPanel(ctx, rect, buttonGlass);

        D2DWidgetUtils::drawTextEx(ctx, label, FontRole::Small, rect, (active || hovered) ? ctx.brushes.text : ctx.brushes.muted, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }

    void AceShellUi::clampAquariumScroll(AquariumScrollPanel& scroll)
    {
        const float maxOffset = std::max(0.0f, scroll.contentHeight - scroll.viewportHeight);
        scroll.offset = std::clamp(scroll.offset, 0.0f, maxOffset);
    }

    void AceShellUi::renderAquariumScrollbar(D2DRenderContext& ctx, AquariumScrollPanel& scroll)
    {
        if (scroll.contentHeight <= scroll.viewportHeight + 1.0f || scroll.track.height() <= 8.0f)
        {
            scroll.thumb = {};
            return;
        }

        clampAquariumScroll(scroll);

        const float trackH = std::max(1.0f, scroll.track.height());
        const float ratio = std::clamp(scroll.viewportHeight / std::max(scroll.contentHeight, 1.0f), 0.10f, 1.0f);
        const float thumbH = std::clamp(trackH * ratio, 24.0f, trackH);
        const float maxOffset = std::max(1.0f, scroll.contentHeight - scroll.viewportHeight);
        const float maxThumbTravel = std::max(1.0f, trackH - thumbH);
        const float thumbTop = scroll.track.top + (scroll.offset / maxOffset) * maxThumbTravel;

        scroll.thumb = makeUiRect(scroll.track.left, thumbTop, scroll.track.right, thumbTop + thumbH);

        if (ctx.brushes.panelDeep)
        {
            const float oldTrack = ctx.brushes.panelDeep->GetOpacity();
            ctx.brushes.panelDeep->SetOpacity(0.42f);
            D2DWidgetUtils::fillRounded(ctx, scroll.track, 4.0f, ctx.brushes.panelDeep);
            ctx.brushes.panelDeep->SetOpacity(oldTrack);
        }

        D2DCyberEffects::drawBorderGlow(ctx, scroll.thumb, 4.0f, aquariumDraggingScroll_ == &scroll ? 0.48f : 0.24f);

        if (ctx.brushes.accent)
        {
            const float oldAccent = ctx.brushes.accent->GetOpacity();
            ctx.brushes.accent->SetOpacity(aquariumDraggingScroll_ == &scroll ? 0.84f : 0.58f);
            D2DWidgetUtils::fillRounded(ctx, scroll.thumb, 4.0f, ctx.brushes.accent);
            ctx.brushes.accent->SetOpacity(oldAccent);
        }
    }

    void AceShellUi::renderAquariumLines(D2DRenderContext& ctx, const std::wstring& title, const std::vector<std::string>& lines, UiRect rect, std::size_t maxLines)
    {
        // ACE-UI1R1: static cards stay static. Scrollbars are only rendered by
        // renderAquariumScrollableLines for explicitly long sections.
        D2DGlassMaterial sectionGlass;
        sectionGlass.radius = 13.0f;
        sectionGlass.fillAlpha = 0.38f;
        sectionGlass.borderAlpha = 0.58f;
        sectionGlass.highlightAlpha = 0.26f;
        sectionGlass.glowAlpha = 0.22f;
        sectionGlass.shadowAlpha = 0.10f;
        sectionGlass.blurFallbackAlpha = 0.12f;
        sectionGlass.useCornerTicks = false;
        D2DGlassEffects::drawGlassPanel(ctx, rect, sectionGlass);
        D2DCyberEffects::drawBorderGlow(ctx, rect, 13.0f, 0.16f);

        if (ctx.brushes.panelDeep)
        {
            const float oldDeep = ctx.brushes.panelDeep->GetOpacity();
            ctx.brushes.panelDeep->SetOpacity(0.18f);
            D2DWidgetUtils::fillRounded(ctx, rect.inset(4.0f), 10.0f, ctx.brushes.panelDeep);
            ctx.brushes.panelDeep->SetOpacity(oldDeep);
        }

        D2DWidgetUtils::drawTextEx(
            ctx,
            title,
            FontRole::Small,
            makeUiRect(rect.left + 10.0f, rect.top + 8.0f, rect.right - 10.0f, rect.top + 28.0f),
            ctx.brushes.text
        );

        float y = rect.top + 32.0f;
        const float lineHeight = 17.0f;
        std::size_t count = 0;
        for (const auto& line : lines)
        {
            if (count >= maxLines || y + lineHeight > rect.bottom - 6.0f)
            {
                break;
            }

            std::string clipped = line;
            if (clipped.size() > 112)
            {
                clipped = clipped.substr(0, 109) + "...";
            }

            D2DWidgetUtils::drawTextEx(
                ctx,
                widen(clipped),
                FontRole::Small,
                makeUiRect(rect.left + 10.0f, y, rect.right - 10.0f, y + lineHeight),
                ctx.brushes.muted
            );

            y += lineHeight;
            ++count;
        }
    }

    void AceShellUi::renderAquariumScrollableLines(D2DRenderContext& ctx, const std::wstring& title, const std::vector<std::string>& lines, UiRect rect, AquariumScrollPanel& scroll, std::size_t maxVisibleLines)
    {
        D2DGlassMaterial sectionGlass;
        sectionGlass.radius = 13.0f;
        sectionGlass.fillAlpha = 0.42f;
        sectionGlass.borderAlpha = 0.66f;
        sectionGlass.highlightAlpha = 0.22f;
        sectionGlass.glowAlpha = 0.28f;
        sectionGlass.shadowAlpha = 0.08f;
        sectionGlass.blurFallbackAlpha = 0.16f;
        sectionGlass.useCornerTicks = false;
        D2DGlassEffects::drawGlassPanel(ctx, rect, sectionGlass);
        D2DCyberEffects::drawBorderGlow(ctx, rect, 13.0f, 0.20f);

        D2DWidgetUtils::drawTextEx(
            ctx,
            title,
            FontRole::Small,
            makeUiRect(rect.left + 10.0f, rect.top + 8.0f, rect.right - 10.0f, rect.top + 28.0f),
            ctx.brushes.text
        );

        const float lineHeight = 17.0f;
        const float viewportTop = rect.top + 32.0f;
        const float viewportBottom = rect.bottom - 8.0f;
        const float scrollBarWidth = 8.0f;

        scroll.viewport = makeUiRect(rect.left + 10.0f, viewportTop, rect.right - 14.0f - scrollBarWidth, viewportBottom);
        scroll.track = makeUiRect(rect.right - 12.0f, viewportTop + 1.0f, rect.right - 6.0f, viewportBottom - 1.0f);
        scroll.viewportHeight = std::max(0.0f, scroll.viewport.height());
        scroll.lineCount = lines.size();
        scroll.contentHeight = static_cast<float>(lines.size()) * lineHeight;
        clampAquariumScroll(scroll);

        if (ctx.target)
        {
            ctx.target->PushAxisAlignedClip(scroll.viewport.d2d(), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        }

        float y = scroll.viewport.top - scroll.offset;
        std::size_t count = 0;
        for (const auto& line : lines)
        {
            if (count >= maxVisibleLines + 32)
            {
                break;
            }

            if (y + lineHeight >= scroll.viewport.top && y <= scroll.viewport.bottom)
            {
                std::string clipped = line;
                if (clipped.size() > 126)
                {
                    clipped = clipped.substr(0, 123) + "...";
                }

                D2DWidgetUtils::drawTextEx(
                    ctx,
                    widen(clipped),
                    FontRole::Small,
                    makeUiRect(scroll.viewport.left, y, scroll.viewport.right, y + lineHeight),
                    ctx.brushes.muted
                );
            }

            y += lineHeight;
            ++count;
        }

        if (ctx.target)
        {
            ctx.target->PopAxisAlignedClip();
        }

        renderAquariumScrollbar(ctx, scroll);
    }

    AceShellUi::AquariumScrollPanel* AceShellUi::activeAquariumScrollPanelAt(float x, float y)
    {
        AquariumScrollPanel* panels[] =
        {
            &aquariumContentScroll_
        };

        for (auto* panel : panels)
        {
            if (panel->contentHeight <= panel->viewportHeight + 1.0f)
            {
                continue;
            }

            if (panel->thumb.contains(x, y) || panel->track.contains(x, y) || panel->viewport.contains(x, y))
            {
                return panel;
            }
        }

        return nullptr;
    }

    bool AceShellUi::handleAquariumWheel(float x, float y, int wheelDelta)
    {
        auto* panel = activeAquariumScrollPanelAt(x, y);
        if (!panel)
        {
            return false;
        }

        panel->offset += static_cast<float>(-wheelDelta) * 0.34f;
        clampAquariumScroll(*panel);
        return true;
    }

    bool AceShellUi::beginAquariumScrollbarDrag(float x, float y)
    {
        auto* panel = activeAquariumScrollPanelAt(x, y);
        if (!panel || !panel->thumb.contains(x, y))
        {
            return false;
        }

        aquariumDraggingScroll_ = panel;
        aquariumDragStartY_ = y;
        aquariumDragStartOffset_ = panel->offset;
        return true;
    }

    bool AceShellUi::updateAquariumScrollbarDrag(float, float y)
    {
        if (!aquariumDraggingScroll_)
        {
            return false;
        }

        auto& scroll = *aquariumDraggingScroll_;
        const float trackTravel = std::max(1.0f, scroll.track.height() - scroll.thumb.height());
        const float maxOffset = std::max(0.0f, scroll.contentHeight - scroll.viewportHeight);
        const float dy = y - aquariumDragStartY_;
        scroll.offset = aquariumDragStartOffset_ + (dy / trackTravel) * maxOffset;
        clampAquariumScroll(scroll);
        return true;
    }

    void AceShellUi::endAquariumScrollbarDrag()
    {
        aquariumDraggingScroll_ = nullptr;
        aquariumDragStartY_ = 0.0f;
        aquariumDragStartOffset_ = 0.0f;
    }

    bool AceShellUi::handleAquariumPanelClick(float x, float y)
    {
        if (!aquariumControllerReady_)
        {
            return false;
        }

        if (aquariumScenarioPrevRect_.contains(x, y))
        {
            cycleAquariumScenario(-1);
            return true;
        }

        if (aquariumScenarioNextRect_.contains(x, y))
        {
            cycleAquariumScenario(1);
            return true;
        }

        if (aquariumPlannerPrevRect_.contains(x, y))
        {
            cycleAquariumPlanner(-1);
            return true;
        }

        if (aquariumPlannerNextRect_.contains(x, y))
        {
            cycleAquariumPlanner(1);
            return true;
        }

        if (aquariumResetRect_.contains(x, y))
        {
            aquariumController_.ResetScenario(aquariumController_.CurrentScenarioName(), 123);
            return true;
        }

        if (aquariumStepRect_.contains(x, y))
        {
            aquariumController_.StepOnce();
            return true;
        }

        if (aquariumRunPauseRect_.contains(x, y))
        {
            aquariumController_.SetRunning(!aquariumController_.IsRunning());
            return true;
        }

        if (aquariumDebugRect_.contains(x, y))
        {
            aquariumController_.SetDebugTruthEnabled(!aquariumController_.DebugTruthEnabled());
            return true;
        }

        if (aquariumLogsFocusRect_.contains(x, y))
        {
            aquariumLogsFocus_ = !aquariumLogsFocus_;
            return true;
        }

        if (aquariumManualForwardRect_.contains(x, y))
        {
            aquariumController_.StepManual(ace::aquarium::AceAqAction::MoveForward);
            return true;
        }

        if (aquariumManualLeftRect_.contains(x, y))
        {
            aquariumController_.StepManual(ace::aquarium::AceAqAction::TurnLeft);
            return true;
        }

        if (aquariumManualRightRect_.contains(x, y))
        {
            aquariumController_.StepManual(ace::aquarium::AceAqAction::TurnRight);
            return true;
        }

        if (aquariumManualWaitRect_.contains(x, y))
        {
            aquariumController_.StepManual(ace::aquarium::AceAqAction::Wait);
            return true;
        }

        if (aquariumManualTouchRect_.contains(x, y))
        {
            aquariumController_.StepManual(ace::aquarium::AceAqAction::TouchFront);
            return true;
        }

        if (aquariumManualConsumeRect_.contains(x, y))
        {
            aquariumController_.StepManual(ace::aquarium::AceAqAction::ConsumeFront);
            return true;
        }

        if (aquariumManualPushRect_.contains(x, y))
        {
            aquariumController_.StepManual(ace::aquarium::AceAqAction::PushFront);
            return true;
        }

        return false;
    }

    void AceShellUi::cycleAquariumScenario(int direction)
    {
        const auto names = aquariumController_.ScenarioNames();
        if (names.empty())
        {
            return;
        }

        auto it = std::find(names.begin(), names.end(), aquariumController_.CurrentScenarioName());
        std::ptrdiff_t index = it == names.end() ? 0 : std::distance(names.begin(), it);
        index += direction;

        if (index < 0)
        {
            index = static_cast<std::ptrdiff_t>(names.size()) - 1;
        }

        if (index >= static_cast<std::ptrdiff_t>(names.size()))
        {
            index = 0;
        }

        aquariumController_.ResetScenario(names[static_cast<std::size_t>(index)], 123);
    }

    void AceShellUi::cycleAquariumPlanner(int direction)
    {
        const std::vector<std::string> planners = {"random", "safe", "counterfactual"};
        auto it = std::find(planners.begin(), planners.end(), aquariumController_.CurrentPlannerName());
        std::ptrdiff_t index = it == planners.end() ? 0 : std::distance(planners.begin(), it);
        index += direction;

        if (index < 0)
        {
            index = static_cast<std::ptrdiff_t>(planners.size()) - 1;
        }

        if (index >= static_cast<std::ptrdiff_t>(planners.size()))
        {
            index = 0;
        }

        aquariumController_.SetPlanner(planners[static_cast<std::size_t>(index)]);
    }

    void AceShellUi::renderEnvironmentPlaceholder(D2DRenderContext& ctx)
    {
        renderAquariumControlPanel(ctx);
    }

    void AceShellUi::renderAquariumControlPanel(D2DRenderContext& ctx)
    {
        if (!environmentOpen_)
        {
            return;
        }

        if (!aquariumControllerReady_)
        {
            aquariumControllerReady_ = aquariumController_.Initialize();
        }

        const auto snapshot = aquariumController_.BuildSnapshot();

        // ACE-UI1R9: Environment tab now adds a visible but still moderate blur veil.
        D2DGlassEffects::drawGlassOverlay(ctx, makeUiRect(0.0f, 0.0f, ctx.width, ctx.height), 0.46f);
        D2DGlassEffects::drawBlurFallback(ctx, environmentModalRect_.inset(-26.0f), 34.0f, 0.22f);

        // ACE-UI1R2: no fake vignette bars. The overlay is a clean frosted
        // focus layer; the panel itself carries depth and glow.
        D2DGlassMaterial modalGlass;
        modalGlass.radius = 20.0f;
        modalGlass.fillAlpha = 0.82f;
        modalGlass.borderAlpha = 1.00f;
        modalGlass.highlightAlpha = 0.52f;
        modalGlass.glowAlpha = 0.82f;
        modalGlass.shadowAlpha = 0.48f;
        modalGlass.blurFallbackAlpha = 0.50f;
        modalGlass.useCornerTicks = true;
        D2DGlassEffects::drawGlassPanel(ctx, environmentModalRect_, modalGlass);
        D2DCyberEffects::drawBorderGlow(ctx, environmentModalRect_.inset(-3.0f), 26.0f, 0.62f);
        // ACE-UI1R1: strong frosted veil so the empty-state hero text behind
        // the modal no longer fights the control panel text.
        if (ctx.brushes.panelDeep)
        {
            const float oldDeep = ctx.brushes.panelDeep->GetOpacity();
            ctx.brushes.panelDeep->SetOpacity(0.54f);
            D2DWidgetUtils::fillRounded(ctx, environmentModalRect_.inset(8.0f), 18.0f, ctx.brushes.panelDeep);
            ctx.brushes.panelDeep->SetOpacity(oldDeep);
        }


        const UiRect topBar = makeUiRect(environmentModalRect_.left, environmentModalRect_.top, environmentModalRect_.right, environmentModalRect_.top + 56.0f);
        D2DGlassMaterial modalTopGlass;
        modalTopGlass.radius = 20.0f;
        modalTopGlass.fillAlpha = 0.30f;
        modalTopGlass.borderAlpha = 0.36f;
        modalTopGlass.glowAlpha = 0.12f;
        modalTopGlass.shadowAlpha = 0.04f;
        modalTopGlass.blurFallbackAlpha = 0.05f;
        modalTopGlass.useCornerTicks = false;
        D2DGlassEffects::drawGlassPanel(ctx, topBar, modalTopGlass);
        D2DWidgetUtils::fillRounded(ctx, makeUiRect(environmentModalRect_.left + 18.0f, topBar.bottom - 3.0f, environmentModalRect_.left + 262.0f, topBar.bottom), 2.0f, ctx.brushes.accentGradient);

        D2DWidgetUtils::drawTextEx(
            ctx,
            L"Cognitive Environment Control Panel",
            FontRole::BodyStrong,
            makeUiRect(environmentModalRect_.left + 20.0f, environmentModalRect_.top + 14.0f, environmentModalRect_.right - 70.0f, environmentModalRect_.top + 44.0f),
            ctx.brushes.text
        );

        D2DWidgetUtils::fillRounded(ctx, environmentModalCloseRect_, 10.0f, ctx.brushes.panelDeep, ctx.brushes.borderDim, 1.0f);
        D2DWidgetUtils::drawTextEx(ctx, L"X", FontRole::Small, environmentModalCloseRect_, ctx.brushes.text, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

        // ACE-UI1R4: subtitle gets real breathing room below the header line.
        D2DWidgetUtils::drawTextEx(
            ctx,
            L"3D viewport is not implemented yet. This panel controls the headless Aquarium C++ runtime.",
            FontRole::Small,
            makeUiRect(environmentModalRect_.left + 24.0f, environmentModalRect_.top + 62.0f, environmentModalRect_.right - 24.0f, environmentModalRect_.top + 82.0f),
            ctx.brushes.muted
        );

        // ACE-UI1R4: grouped control tabs. The command strip is readable now:
        // Scenario / Planner / Runtime on row 1, Manual Actions on row 2.
        const float aqLeft = environmentModalRect_.left + 26.0f;
        const float aqRight = environmentModalRect_.right - 26.0f;
        const float aqTop = environmentModalRect_.top + 98.0f;
        const float aqButtonH = 31.0f;
        const float groupGap = 14.0f;
        const float groupH = 72.0f;

        const float stripW = std::max(720.0f, aqRight - aqLeft);
        const float scenarioW = std::clamp(stripW * 0.23f, 220.0f, 300.0f);
        const float plannerW = std::clamp(stripW * 0.25f, 240.0f, 330.0f);
        const float runtimeW = std::max(360.0f, stripW - scenarioW - plannerW - groupGap * 2.0f);

        const UiRect scenarioGroup = makeUiRect(aqLeft, aqTop, aqLeft + scenarioW, aqTop + groupH);
        const UiRect plannerGroup = makeUiRect(scenarioGroup.right + groupGap, aqTop, scenarioGroup.right + groupGap + plannerW, aqTop + groupH);
        const UiRect runtimeGroup = makeUiRect(plannerGroup.right + groupGap, aqTop, std::min(aqRight, plannerGroup.right + groupGap + runtimeW), aqTop + groupH);

        const float groupButtonY = aqTop + 32.0f;
        aquariumScenarioPrevRect_ = makeUiRect(scenarioGroup.left + 12.0f, groupButtonY, scenarioGroup.left + 50.0f, groupButtonY + aqButtonH);
        aquariumScenarioNextRect_ = makeUiRect(scenarioGroup.left + 58.0f, groupButtonY, scenarioGroup.left + 96.0f, groupButtonY + aqButtonH);
        aquariumPlannerPrevRect_ = makeUiRect(plannerGroup.left + 12.0f, groupButtonY, plannerGroup.left + 50.0f, groupButtonY + aqButtonH);
        aquariumPlannerNextRect_ = makeUiRect(plannerGroup.left + 58.0f, groupButtonY, plannerGroup.left + 96.0f, groupButtonY + aqButtonH);

        const float runtimeButtonX = runtimeGroup.left + 12.0f;
        aquariumResetRect_ = makeUiRect(runtimeButtonX, groupButtonY, runtimeButtonX + 74.0f, groupButtonY + aqButtonH);
        aquariumStepRect_ = makeUiRect(runtimeButtonX + 84.0f, groupButtonY, runtimeButtonX + 152.0f, groupButtonY + aqButtonH);
        aquariumRunPauseRect_ = makeUiRect(runtimeButtonX + 162.0f, groupButtonY, runtimeButtonX + 250.0f, groupButtonY + aqButtonH);
        aquariumDebugRect_ = makeUiRect(runtimeButtonX + 260.0f, groupButtonY, std::min(runtimeGroup.right - 12.0f, runtimeButtonX + 386.0f), groupButtonY + aqButtonH);
        aquariumLogsFocusRect_ = makeUiRect(runtimeButtonX + 396.0f, groupButtonY, std::min(runtimeGroup.right - 12.0f, runtimeButtonX + 514.0f), groupButtonY + aqButtonH);

        const float manualTop = aqTop + groupH + 14.0f;
        const UiRect manualGroup = makeUiRect(aqLeft, manualTop, aqRight, manualTop + 70.0f);
        const float manualButtonY = manualTop + 32.0f;
        const float manualX = manualGroup.left + 12.0f;
        aquariumManualForwardRect_ = makeUiRect(manualX, manualButtonY, manualX + 88.0f, manualButtonY + aqButtonH);
        aquariumManualLeftRect_ = makeUiRect(manualX + 100.0f, manualButtonY, manualX + 188.0f, manualButtonY + aqButtonH);
        aquariumManualRightRect_ = makeUiRect(manualX + 200.0f, manualButtonY, manualX + 288.0f, manualButtonY + aqButtonH);
        aquariumManualWaitRect_ = makeUiRect(manualX + 300.0f, manualButtonY, manualX + 374.0f, manualButtonY + aqButtonH);
        aquariumManualTouchRect_ = makeUiRect(manualX + 386.0f, manualButtonY, manualX + 468.0f, manualButtonY + aqButtonH);
        aquariumManualConsumeRect_ = makeUiRect(manualX + 480.0f, manualButtonY, manualX + 584.0f, manualButtonY + aqButtonH);
        aquariumManualPushRect_ = makeUiRect(manualX + 596.0f, manualButtonY, manualX + 670.0f, manualButtonY + aqButtonH);

        auto renderControlGroup = [&](UiRect rect, const std::wstring& title)
        {
            D2DGlassMaterial groupGlass;
            groupGlass.radius = 13.0f;
            groupGlass.fillAlpha = 0.36f;
            groupGlass.borderAlpha = 0.56f;
            groupGlass.highlightAlpha = 0.22f;
            groupGlass.glowAlpha = 0.22f;
            groupGlass.shadowAlpha = 0.08f;
            groupGlass.blurFallbackAlpha = 0.14f;
            groupGlass.useCornerTicks = false;
            D2DGlassEffects::drawGlassPanel(ctx, rect, groupGlass);
            D2DCyberEffects::drawBorderGlow(ctx, rect, 13.0f, 0.14f);
            D2DWidgetUtils::drawTextEx(ctx, title, FontRole::Small, makeUiRect(rect.left + 12.0f, rect.top + 8.0f, rect.right - 12.0f, rect.top + 28.0f), ctx.brushes.text);
        };

        renderControlGroup(scenarioGroup, L"Scenario");
        renderControlGroup(plannerGroup, L"Planner");
        renderControlGroup(runtimeGroup, L"Runtime");
        renderControlGroup(manualGroup, L"Manual Actions");

        D2DWidgetUtils::drawTextEx(ctx, widen(snapshot.scenarioName), FontRole::Small, makeUiRect(aquariumScenarioNextRect_.right + 10.0f, groupButtonY + 5.0f, scenarioGroup.right - 12.0f, groupButtonY + aqButtonH), ctx.brushes.muted);
        D2DWidgetUtils::drawTextEx(ctx, widen(snapshot.plannerName), FontRole::Small, makeUiRect(aquariumPlannerNextRect_.right + 10.0f, groupButtonY + 5.0f, plannerGroup.right - 12.0f, groupButtonY + aqButtonH), ctx.brushes.muted);
        D2DWidgetUtils::drawTextEx(ctx, widen("Step " + std::to_string(snapshot.step)), FontRole::Small, makeUiRect(aquariumLogsFocusRect_.right + 10.0f, groupButtonY + 5.0f, runtimeGroup.right - 12.0f, groupButtonY + aqButtonH), ctx.brushes.muted);

        renderAquariumButton(ctx, aquariumScenarioPrevRect_, L"<", false);
        renderAquariumButton(ctx, aquariumScenarioNextRect_, L">", false);
        renderAquariumButton(ctx, aquariumPlannerPrevRect_, L"<", false);
        renderAquariumButton(ctx, aquariumPlannerNextRect_, L">", false);
        renderAquariumButton(ctx, aquariumResetRect_, L"Reset", false);
        renderAquariumButton(ctx, aquariumStepRect_, L"Step", false);
        renderAquariumButton(ctx, aquariumRunPauseRect_, snapshot.running ? L"Pause" : L"Run", snapshot.running);
        renderAquariumButton(ctx, aquariumDebugRect_, L"Debug Truth", snapshot.debugTruthEnabled);
        renderAquariumButton(ctx, aquariumLogsFocusRect_, L"Focus Logs", aquariumLogsFocus_);

        renderAquariumButton(ctx, aquariumManualForwardRect_, L"Forward", false);
        renderAquariumButton(ctx, aquariumManualLeftRect_, L"Turn L", false);
        renderAquariumButton(ctx, aquariumManualRightRect_, L"Turn R", false);
        renderAquariumButton(ctx, aquariumManualWaitRect_, L"Wait", false);
        renderAquariumButton(ctx, aquariumManualTouchRect_, L"Touch", false);
        renderAquariumButton(ctx, aquariumManualConsumeRect_, L"Consume", false);
        renderAquariumButton(ctx, aquariumManualPushRect_, L"Push", false);

        // ACE-UI1R7: real content viewport. Controls stay fixed; everything
        // below them is a clipped scrollable content flow.
        const float viewportTop = manualGroup.bottom + 16.0f;
        const float left = environmentModalRect_.left + 22.0f;
        const float right = environmentModalRect_.right - 34.0f;
        const float bottom = environmentModalRect_.bottom - 20.0f;
        const float gap = 10.0f;
        const float colW = (right - left - gap * 2.0f) / 3.0f;
        const float viewportH = std::max(120.0f, bottom - viewportTop);
        const float rowH = aquariumLogsFocus_ ? 0.0f : 96.0f;
        const float logContentH = 44.0f + static_cast<float>(std::max<std::size_t>(snapshot.logLines.size(), 1)) * 17.0f;
        const float debugContentH = snapshot.debugTruthEnabled ? (44.0f + static_cast<float>(std::max<std::size_t>(snapshot.debugTruthLines.size(), 1)) * 17.0f) : 0.0f;
        const float logEndPadding = 24.0f;
        const float logH = aquariumLogsFocus_
            ? std::max(viewportH, logContentH + debugContentH + (snapshot.debugTruthEnabled ? gap : 0.0f) + logEndPadding)
            : std::max(132.0f, logContentH + debugContentH + (snapshot.debugTruthEnabled ? gap : 0.0f) + logEndPadding);
        const float contentTop = viewportTop - aquariumContentScroll_.offset;
        const float contentHeight = aquariumLogsFocus_ ? logH : (rowH * 3.0f + gap * 3.0f + logH);
        // ACE-UI1R8: bottom must move with contentTop. Previously the top
        // scrolled but the bottom stayed anchored to viewportTop, so the
        // Logs card could lose its visible ending while scrolling.
        const float contentBottom = contentTop + contentHeight;
        aquariumContentScroll_.viewport = makeUiRect(left, viewportTop, right, bottom);
        aquariumContentScroll_.track = makeUiRect(environmentModalRect_.right - 22.0f, viewportTop + 2.0f, environmentModalRect_.right - 12.0f, bottom - 2.0f);
        aquariumContentScroll_.viewportHeight = viewportH;
        aquariumContentScroll_.contentHeight = contentHeight;
        aquariumContentScroll_.lineCount = 0;
        clampAquariumScroll(aquariumContentScroll_);

        if (ctx.target)
        {
            ctx.target->PushAxisAlignedClip(aquariumContentScroll_.viewport.d2d(), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        }

        const float top = contentTop;
        const float logTop = aquariumLogsFocus_ ? contentTop : (contentTop + rowH * 3.0f + gap * 3.0f);
        const float logBottom = contentBottom;

        // ACE-UI1R6: Focus Logs hides upper dashboard cards and turns the
        // remaining space into a real log viewer.
        if (!aquariumLogsFocus_)
        {
            renderAquariumLines(ctx, L"Body / Agent", {
                "body: H=" + std::to_string(snapshot.body.hydration).substr(0, 4) +
                " N=" + std::to_string(snapshot.body.nutrition).substr(0, 4) +
                " I=" + std::to_string(snapshot.body.integrity).substr(0, 4) +
                " T=" + std::to_string(snapshot.body.temperature).substr(0, 4),
                "homeostatic_error=" + std::to_string(snapshot.homeostaticError),
                "agent: " + snapshot.agentPositionText,
                "direction: " + snapshot.agentDirectionText,
                "last action: " + snapshot.lastActionText,
                "last result: " + snapshot.lastResultText
            }, makeUiRect(left, top, left + colW, top + rowH), 7);

            renderAquariumLines(ctx, L"Observation", snapshot.observationLines, makeUiRect(left + colW + gap, top, left + colW * 2.0f + gap, top + rowH), 6);
            renderAquariumLines(ctx, L"Planner Trace", snapshot.decisionTraceLines, makeUiRect(left + colW * 2.0f + gap * 2.0f, top, right, top + rowH), 6);

            const float row2 = top + rowH + gap;
            renderAquariumLines(ctx, L"Counterfactual", snapshot.counterfactualTraceLines, makeUiRect(left, row2, left + colW, row2 + rowH), 6);
            renderAquariumLines(ctx, L"Memory / Concepts", snapshot.protoConceptLines, makeUiRect(left + colW + gap, row2, left + colW * 2.0f + gap, row2 + rowH), 6);
            renderAquariumLines(ctx, L"Self Model", snapshot.selfModelLines, makeUiRect(left + colW * 2.0f + gap * 2.0f, row2, right, row2 + rowH), 6);

            const float row3 = row2 + rowH + gap;
            renderAquariumLines(ctx, L"Delayed / Dynamic", snapshot.delayedEffectLines, makeUiRect(left, row3, left + colW, row3 + rowH), 6);
            renderAquariumLines(ctx, L"World Events", snapshot.worldEventLines, makeUiRect(left + colW + gap, row3, left + colW * 2.0f + gap, row3 + rowH), 6);
            renderAquariumLines(ctx, L"Metrics", snapshot.metricLines, makeUiRect(left + colW * 2.0f + gap * 2.0f, row3, right, row3 + rowH), 8);

        }
        else
        {
            // ACE-UI1R6: Focus mode intentionally renders no dashboard cards
            // and no extra banner. The full remaining area belongs to Logs.
        }

        // ACE-UI1R5: Logs must never escape the Environment panel.
        const UiRect logsRect = makeUiRect(left, logTop, right, logBottom - 8.0f);
        if (snapshot.debugTruthEnabled)
        {
            const float debugH = std::min(84.0f, std::max(54.0f, logsRect.height() * 0.35f));
            const UiRect logOnlyRect = makeUiRect(logsRect.left, logsRect.top, logsRect.right, std::max(logsRect.top + 54.0f, logsRect.bottom - debugH - gap));
            const UiRect debugRect = makeUiRect(logsRect.left, std::max(logOnlyRect.bottom + gap, logsRect.bottom - debugH), logsRect.right, logsRect.bottom);
            renderAquariumLines(ctx, L"Logs / Episodes", snapshot.logLines, logOnlyRect, snapshot.logLines.size() + 2);
            renderAquariumLines(ctx, L"DEBUG TRUTH - NOT AGENT INPUT", snapshot.debugTruthLines, debugRect, snapshot.debugTruthLines.size() + 2);
        }
        else
        {
            renderAquariumLines(ctx, L"Logs / Episodes", snapshot.logLines, logsRect, snapshot.logLines.size() + 2);
        }

        if (ctx.target)
        {
            ctx.target->PopAxisAlignedClip();
        }

        renderAquariumScrollbar(ctx, aquariumContentScroll_);
    }

    void AceShellUi::sendCurrentInput()
    {
        if (input_.empty())
        {
            statusBar_.setText(L"Input is empty.");
            return;
        }

        const auto submittedText = input_.takeText();

        if (handleLocalInputCommand(submittedText))
        {
            layout(width_, height_);
            invalidate();
            return;
        }

        commandHistory_.push(submittedText);
        messageList_.addMessage(makeMessage(L"You", submittedText, true, false, ChatMessageKind::User, L""));

        if (submitHandler_)
        {
            appendSubmitResult(submitHandler_(submittedText));
        }
        else
        {
            appendAssistantMockReply();
            statusBar_.setText(L"Input captured locally. No legacy AI backend is attached.");
        }

        saveActiveConversation();
        layout(width_, height_);
        invalidate();
    }

    bool AceShellUi::handleLocalInputCommand(const std::wstring& text)
    {
        const std::wstring prefix = L"/rename ";
        if (text.rfind(prefix, 0) != 0)
        {
            return false;
        }

        std::wstring name = text.substr(prefix.size());
        const auto first = name.find_first_not_of(L" \t\r\n");
        const auto last = name.find_last_not_of(L" \t\r\n");

        if (first == std::wstring::npos || last == std::wstring::npos)
        {
            statusBar_.setText(L"Rename cancelled: empty name.");
            return true;
        }

        name = name.substr(first, last - first + 1);

        if (conversations_.empty())
        {
            conversations_.push_back(name);
            activeConversationIndex_ = 0;
            nextConversationNumber_ = 2;
        }
        else
        {
            activeConversationIndex_ = std::min(activeConversationIndex_, conversations_.size() - 1);
            conversations_[activeConversationIndex_] = name;
        }

        if (conversationMessages_.size() < conversations_.size())
        {
            conversationMessages_.resize(conversations_.size());
        }

        statusBar_.setText(L"Conversation renamed.");
        return true;
    }

    void AceShellUi::appendExternalMessage(ChatMessage message)
    {
        if (message.id == 0)
        {
            message.id = nextMessageId_++;
        }

        if (message.timestamp.empty())
        {
            message.timestamp = currentTimestamp();
        }

        message.fromUser = message.kind == ChatMessageKind::User ? true : message.fromUser;
        message.system = message.kind == ChatMessageKind::System ? true : message.system;
        messageList_.addMessage(std::move(message));
        saveActiveConversation();
    }

    void AceShellUi::appendSubmitResult(ChatSubmitResult result)
    {
        for (auto& message : result.messages)
        {
            appendExternalMessage(std::move(message));
        }

        if (!result.status.empty())
        {
            statusBar_.setText(std::move(result.status));
        }
        else
        {
            statusBar_.setText(result.ok ? L"Backend submit completed." : L"Backend submit failed.");
        }

        refreshSnapshot();
        refreshSuggestions();
        refreshDiagnostics();
    }

    void AceShellUi::appendAssistantMockReply()
    {
        messageList_.addMessage(makeMessage(
            L"Arhqen Cognition Engine",
            L"Input captured locally. No legacy AI backend is attached.",
            false,
            false,
            ChatMessageKind::Assistant,
            L"local"
        ));
        saveActiveConversation();
    }



    void AceShellUi::refreshSnapshot()
    {
        if (snapshotProvider_)
        {
            workspacePanel_.setSnapshot(snapshotProvider_());
        }
        else
        {
            am::core::AceUiSnapshot snapshot;
            snapshot.title = L"Arhqen Cognition Engine Workspace";
            snapshot.subtitle = L"Snapshot provider is not connected.";
            snapshot.metrics.push_back({L"Provider", L"missing", 3});
            workspacePanel_.setSnapshot(std::move(snapshot));
        }
    }

    void AceShellUi::refreshSuggestions()
    {
        if (!suggestionProvider_)
        {
            autocomplete_.close();
            return;
        }

        auto suggestions = suggestionProvider_(input_.text());
        autocomplete_.setSuggestions(std::move(suggestions));
    }

    void AceShellUi::acceptAutocomplete()
    {
        if (auto accepted = autocomplete_.takeAcceptedText())
        {
            input_.replaceAllText(*accepted);
            input_.setFocused(true);
            focus_.set(D2DFocusTarget::TextInput);
            refreshSuggestions();
            statusBar_.setText(L"Autocomplete accepted.");
        }
    }



    void AceShellUi::refreshInspector(am::core::AceUiSelection selection)
    {
        if (inspectorProvider_)
        {
            inspectorPanel_.setRecord(inspectorProvider_(selection));
            showToast(L"Inspector", L"Selected " + selection.label, D2DToastKind::Success);
        }
        else
        {
            am::core::AceUiInspectorRecord record;
            record.selection = selection;
            record.title = selection.label.empty() ? L"Inspector" : selection.label;
            record.subtitle = L"Inspector provider missing.";
            record.body = L"The UI selected an item, but no inspector provider is connected.";
            record.properties.push_back({L"provider", L"missing", 3});
            inspectorPanel_.setRecord(std::move(record));
        }
    }


    void AceShellUi::inspectLocalMessage(std::uint64_t messageId)
    {
        const ChatMessage* message = messageList_.findMessage(messageId);
        if (!message)
        {
            am::core::AceUiInspectorRecord record;
            record.title = L"Message missing";
            record.subtitle = L"Local UI message lookup failed.";
            record.body = L"The message was clicked, then disappeared. Naturally.";
            record.properties.push_back({L"id", std::to_wstring(messageId), 3});
            inspectorPanel_.setRecord(std::move(record));
            return;
        }

        inspectorPanel_.setRecord(D2DMessageInspector::makeRecord(*message));
        workspaceTabs_.setActive(L"inspect");
        statusBar_.setText(L"Inspecting message #" + std::to_wstring(messageId));
        showToast(L"Message inspector", L"Opened message #" + std::to_wstring(messageId), D2DToastKind::Success);
    }

    void AceShellUi::refreshDiagnostics()
    {
        D2DDiagnosticsSnapshot snapshot;
        snapshot.title = L"Arhqen Cognition Engine Diagnostics";
        snapshot.visible = diagnostics_.visible();

        snapshot.rows.push_back({L"window", std::to_wstring(width_) + L"x" + std::to_wstring(height_)});
        snapshot.rows.push_back({L"text cache", L"size=" + std::to_wstring(textCache_.size()) + L" cap=" + std::to_wstring(textCache_.capacity())});
        snapshot.rows.push_back({L"text cache hits", std::to_wstring(textCache_.hitCount())});
        snapshot.rows.push_back({L"text cache misses", std::to_wstring(textCache_.missCount())});
        snapshot.rows.push_back({L"message layout cache", L"size=" + std::to_wstring(messageList_.layoutCacheSize())});
        snapshot.rows.push_back({L"layout cache hits", std::to_wstring(messageList_.layoutCacheHits())});
        snapshot.rows.push_back({L"layout cache misses", std::to_wstring(messageList_.layoutCacheMisses())});
        snapshot.rows.push_back({L"messages", std::to_wstring(messageList_.size())});
        snapshot.rows.push_back({L"toolbar", L"active"});
        snapshot.rows.push_back({L"autocomplete", autocomplete_.active() ? L"visible" : L"hidden"});
        snapshot.rows.push_back({L"command palette", commandPalette_.active() ? L"visible" : L"hidden"});
        snapshot.rows.push_back({L"focus", focus_.name()});
        snapshot.rows.push_back({L"workspace ratio", std::to_wstring(layoutProfile_.workspaceRatio)});
        snapshot.rows.push_back({L"inspector ratio", std::to_wstring(layoutProfile_.inspectorRatio)});
        snapshot.rows.push_back({L"command history", std::to_wstring(commandHistory_.size())});
        const auto graphStats = workspacePanel_.graphStats();
        snapshot.rows.push_back({L"graph nodes", std::to_wstring(graphStats.nodeCount)});
        snapshot.rows.push_back({L"graph edges", std::to_wstring(graphStats.edgeCount)});
        snapshot.rows.push_back({L"graph zoom", std::to_wstring(graphStats.zoom)});

        snapshot.notes.push_back(L"F12 toggles this overlay.");
        snapshot.notes.push_back(L"Click a message bubble to inspect local message metadata.");
        snapshot.notes.push_back(L"Click workspace items to inspect backend records.");
        snapshot.notes.push_back(L"Toolbar actions are command-palette shortcuts with fewer human ceremonies.");

        diagnostics_.setSnapshot(std::move(snapshot));
    }


    void AceShellUi::showToast(std::wstring title, std::wstring body, D2DToastKind kind)
    {
        toastCenter_.push(std::move(title), std::move(body), kind, 4.0f);
    }


    void AceShellUi::saveLayoutProfile()
    {
        if (layoutProfilePath_.empty())
        {
            showToast(L"Layout", L"No layout path configured.", D2DToastKind::Warning);
            return;
        }

        std::string error;
        layoutProfile_.activeWorkspaceTab = workspaceTabs_.active();

        if (D2DLayoutPersistence::save(layoutProfilePath_, layoutProfile_, &error))
        {
            showToast(L"Layout saved", layoutProfilePath_.wstring(), D2DToastKind::Success);
            statusBar_.setText(L"Layout profile saved.");
        }
        else
        {
            showToast(L"Layout save failed", std::wstring(error.begin(), error.end()), D2DToastKind::Error);
            statusBar_.setText(L"Layout profile save failed.");
        }
    }

    void AceShellUi::loadLayoutProfile()
    {
        if (layoutProfilePath_.empty())
        {
            return;
        }

        std::string error;
        if (auto loaded = D2DLayoutPersistence::load(layoutProfilePath_, &error))
        {
            layoutProfile_ = *loaded;
            layoutProfile_.clamp();

            if (!layoutProfile_.activeWorkspaceTab.empty())
            {
                workspaceTabs_.setActive(layoutProfile_.activeWorkspaceTab);
            }

            refreshLayoutOverlay();
            statusBar_.setText(L"Layout profile loaded.");
        }
    }

    void AceShellUi::resetLayoutProfile()
    {
        layoutProfile_ = D2DDockLayoutProfile::defaults();
        workspaceTabs_.setActive(layoutProfile_.activeWorkspaceTab);
        layout(width_, height_);
        refreshLayoutOverlay();
        showToast(L"Layout reset", L"Default dock profile restored.", D2DToastKind::Success);
        statusBar_.setText(L"Layout profile reset.");
    }

    void AceShellUi::refreshLayoutOverlay()
    {
        layoutProfile_.activeWorkspaceTab = workspaceTabs_.active().empty() ? layoutProfile_.activeWorkspaceTab : workspaceTabs_.active();
        layoutProfile_.clamp();
        layoutOverlay_.setProfile(layoutProfile_);
    }


    void AceShellUi::startNewConversation()
    {
        saveActiveConversation();

        const std::wstring title = L"Conversation " + std::to_wstring(nextConversationNumber_++);
        conversations_.push_back(title);
        conversationMessages_.push_back({});
        activeConversationIndex_ = conversations_.empty() ? 0 : conversations_.size() - 1;

        messageList_.setMessages({});
        input_.setText(L"");
        input_.setFocused(true);
        focus_.set(D2DFocusTarget::TextInput);
        statusBar_.setText(L"New conversation.");
        layout(width_, height_);
    }

    void AceShellUi::selectConversation(std::size_t index)
    {
        if (index >= conversations_.size())
        {
            return;
        }

        saveActiveConversation();
        activeConversationIndex_ = index;

        if (conversationMessages_.size() < conversations_.size())
        {
            conversationMessages_.resize(conversations_.size());
        }

        messageList_.setMessages(conversationMessages_[activeConversationIndex_]);
        input_.setText(L"");
        input_.setFocused(true);
        focus_.set(D2DFocusTarget::TextInput);
        statusBar_.setText(L"Selected " + conversations_[index] + L".");
        layout(width_, height_);
        invalidate();
    }

    void AceShellUi::prefillRenameConversation(std::size_t index)
    {
        if (index >= conversations_.size())
        {
            return;
        }

        selectConversation(index);
        input_.setText(L"/rename " + conversations_[index]);
        input_.setFocused(true);
        focus_.set(D2DFocusTarget::TextInput);
        statusBar_.setText(L"Edit the name in the input, then press Enter.");
        layout(width_, height_);
    }

    void AceShellUi::saveActiveConversation()
    {
        if (conversations_.empty())
        {
            conversations_.push_back(L"Conversation 1");
            activeConversationIndex_ = 0;
            nextConversationNumber_ = std::max<std::uint64_t>(nextConversationNumber_, 2);
        }

        if (conversationMessages_.size() < conversations_.size())
        {
            conversationMessages_.resize(conversations_.size());
        }

        activeConversationIndex_ = std::min(activeConversationIndex_, conversations_.size() - 1);
        conversationMessages_[activeConversationIndex_] = messageList_.messages();
    }

    void AceShellUi::clearInspector()
    {
        am::core::AceUiInspectorRecord record;
        record.title = L"Inspector";
        record.subtitle = L"Select an item.";
        record.body = L"Click a workspace item to inspect its backend data.";
        record.properties.push_back({L"state", L"idle", 0});
        inspectorPanel_.setRecord(std::move(record));
    }


    void AceShellUi::executeCommand(const std::wstring& commandId)
    {
        if (commandId == L"palette")
        {
            openCommandPalette();
        }
        else if (commandId == L"summary")
        {
            if (submitHandler_)
            {
                appendSubmitResult(submitHandler_(L"/summary"));
            }
            statusBar_.setText(L"Toolbar executed: summary");
        }
        else if (commandId == L"store")
        {
            if (submitHandler_)
            {
                appendSubmitResult(submitHandler_(L"/summary"));
            }
            statusBar_.setText(L"Store summary appended to chat.");
        }
        else if (commandId == L"ledger")
        {
            refreshInspector(am::core::AceUiSelection{am::core::AceUiItemKind::Ledger, 0, L"Belief Ledger", L"toolbar"});
            statusBar_.setText(L"Toolbar inspected ledger.");
        }
        else if (commandId == L"diagnostics")
        {
            diagnostics_.toggle();
            toolbar_.setToggled(L"diagnostics", diagnostics_.visible());
            refreshDiagnostics();
            showToast(L"Diagnostics", diagnostics_.visible() ? L"Diagnostics overlay opened." : L"Diagnostics overlay closed.", D2DToastKind::Info);
            statusBar_.setText(diagnostics_.visible() ? L"Diagnostics opened." : L"Diagnostics closed.");
        }
        else if (commandId == L"shortcuts")
        {
            shortcutHelp_.toggle();
            showToast(L"Help", shortcutHelp_.visible() ? L"Shortcut help opened." : L"Shortcut help closed.", D2DToastKind::Info);
            statusBar_.setText(shortcutHelp_.visible() ? L"Shortcut help opened." : L"Shortcut help closed.");
        }
        else if (commandId == L"graph_fit")
        {
            messageList_.addMessage(makeMessage(L"Tools", L"Graph tools are hidden until there is a real environment graph view.", false, true, ChatMessageKind::System, L"graph"));
            showToast(L"Graph", L"Graph tools hidden.", D2DToastKind::Info);
            statusBar_.setText(L"Graph tool command acknowledged.");
        }
        else if (commandId == L"graph_layout")
        {
            messageList_.addMessage(makeMessage(L"Tools", L"Graph auto-layout is reserved for a future environment graph view.", false, true, ChatMessageKind::System, L"graph"));
            showToast(L"Graph", L"Graph auto-layout acknowledged.", D2DToastKind::Info);
            statusBar_.setText(L"Graph auto-layout command acknowledged.");
        }
        else if (commandId == L"layout" || commandId == L"layout_overlay")
        {
            layoutOverlay_.toggle();
            refreshLayoutOverlay();
            showToast(L"Layout", layoutOverlay_.visible() ? L"Layout overlay opened." : L"Layout overlay closed.", D2DToastKind::Info);
            statusBar_.setText(layoutOverlay_.visible() ? L"Layout overlay opened." : L"Layout overlay closed.");
        }
        else if (commandId == L"save_layout")
        {
            saveLayoutProfile();
        }
        else if (commandId == L"load_layout")
        {
            loadLayoutProfile();
            layout(width_, height_);
            showToast(L"Layout loaded", L"Saved layout profile applied.", D2DToastKind::Success);
        }
        else if (commandId == L"reset_layout")
        {
            resetLayoutProfile();
        }
        else if (commandId == L"clear_chat")
        {
            messageList_.clear();
            messageList_.addMessage(makeMessage(L"System", L"Workspace entries cleared by command palette.", false, true, ChatMessageKind::System, L"command"));
            statusBar_.setText(L"Command executed: clear workspace");
        }
        else if (commandId == L"seed_demo")
        {
            seedDemoMessages();
            statusBar_.setText(L"Command executed: seed_demo");
        }
        else if (commandId == L"focus_input")
        {
            focus_.set(D2DFocusTarget::TextInput);
            input_.setFocused(true);
            statusBar_.setText(L"Command executed: focus_input");
        }
        else if (commandId == L"insert_template")
        {
            input_.setFocused(true);
            focus_.set(D2DFocusTarget::TextInput);
            input_.setText(L"Analyze this claim: liquids boil when vapor pressure equals external pressure. Generate hypotheses, evidence needs, and possible counterexamples.");
            statusBar_.setText(L"Command executed: insert_template");
        }
        else if (commandId == L"backend_summary")
        {
            if (submitHandler_)
            {
                appendSubmitResult(submitHandler_(L"/summary"));
            }
            else
            {
                messageList_.addMessage(makeMessage(L"Warning", L"Submit handler is not connected.", false, false, ChatMessageKind::Warning, L"backend"));
            }
            statusBar_.setText(L"Command executed: backend_summary");
        }
        else if (commandId == L"backend_help")
        {
            if (submitHandler_)
            {
                appendSubmitResult(submitHandler_(L"/help"));
            }
            else
            {
                messageList_.addMessage(makeMessage(L"Warning", L"Submit handler is not connected.", false, false, ChatMessageKind::Warning, L"backend"));
            }
            statusBar_.setText(L"Command executed: backend_help");
        }
        else if (commandId == L"sample_concept")
        {
            input_.setFocused(true);
            focus_.set(D2DFocusTarget::TextInput);
            input_.setText(L"concept vapor_pressure | pressure exerted by vapor in equilibrium with liquid");
            statusBar_.setText(L"Inserted concept parser command.");
        }
        else if (commandId == L"sample_hypothesis")
        {
            input_.setFocused(true);
            focus_.set(D2DFocusTarget::TextInput);
            input_.setText(L"Liquids with higher external pressure need higher temperature to boil.");
            statusBar_.setText(L"Inserted natural-language hypothesis.");
        }
        else if (commandId == L"refresh_workspace")
        {
            refreshSnapshot();
            statusBar_.setText(L"Workspace snapshot refreshed.");
        }
        else if (commandId == L"show_concepts")
        {
            if (submitHandler_)
            {
                appendSubmitResult(submitHandler_(L"/concepts"));
                refreshSnapshot();
            }
            else
            {
                messageList_.addMessage(makeMessage(L"Warning", L"Submit handler is not connected.", false, false, ChatMessageKind::Warning, L"backend"));
            }
            statusBar_.setText(L"Command executed: show_concepts");
        }
        else if (commandId == L"inspect_store")
        {
            refreshInspector(am::core::AceUiSelection{am::core::AceUiItemKind::Store, 0, L"Cognitive Store", L"command"});
            statusBar_.setText(L"Inspector opened: store");
        }
        else if (commandId == L"inspect_ledger")
        {
            refreshInspector(am::core::AceUiSelection{am::core::AceUiItemKind::Ledger, 0, L"Belief Ledger", L"command"});
            statusBar_.setText(L"Inspector opened: ledger");
        }
        else if (commandId == L"cache_stats")
        {
            std::wstring stats =
                L"Text cache: size=" + std::to_wstring(textCache_.size()) +
                L" capacity=" + std::to_wstring(textCache_.capacity()) +
                L" hits=" + std::to_wstring(textCache_.hitCount()) +
                L" misses=" + std::to_wstring(textCache_.missCount()) +
                L" | message layout cache: size=" + std::to_wstring(messageList_.layoutCacheSize()) +
                L" hits=" + std::to_wstring(messageList_.layoutCacheHits()) +
                L" misses=" + std::to_wstring(messageList_.layoutCacheMisses());

            messageList_.addMessage(makeMessage(L"Tool", stats, false, false, ChatMessageKind::Tool, L"font cache"));
            statusBar_.setText(L"Command executed: cache_stats");
        }
        else if (commandId == L"help")
        {
            messageList_.addMessage(makeMessage(
                L"System",
                L"Shortcuts: Ctrl+K/Ctrl+P command palette, Ctrl+A select input, Ctrl+C copy, Ctrl+X cut, Ctrl+V paste, Tab focus, mouse wheel scroll, Enter send.",
                false,
                true,
                ChatMessageKind::System,
                L"help"
            ));
            statusBar_.setText(L"Command executed: help");
        }
        else
        {
            messageList_.addMessage(makeMessage(L"Warning", L"Unknown command: " + commandId, false, false, ChatMessageKind::Warning, L"command"));
            statusBar_.setText(L"Unknown command.");
        }

        invalidate();
    }

    void AceShellUi::openCommandPalette()
    {
        commandPalette_.open();
        focus_.set(D2DFocusTarget::CommandPalette);
        input_.setFocused(false);
        statusBar_.setText(L"Command palette open. Type to filter; Enter executes; Esc closes.");
    }

    void AceShellUi::seedDemoMessages()
    {
        messageList_.addMessage(makeMessage(L"System", L"ACE shell demo seed inserted for layout testing.", false, true, ChatMessageKind::System, L"demo"));
        messageList_.addMessage(makeMessage(L"Tool", L"ACE shell backend placeholder.applyEvidence(claim=boiling_rule, delta=+0.11)", false, false, ChatMessageKind::Tool, L"backend trace"));
        messageList_.addMessage(makeMessage(L"Arhqen Cognition Engine", L"This message is intentionally longer so the message list has to measure text, wrap it, estimate bubble height, and scroll like a civilized UI instead of guessing with a cursed char counter.", false, false, ChatMessageKind::Assistant, L"layout"));
        messageList_.addMessage(makeMessage(L"You", L"Good. Keep making the UI less embarrassing.", true, false, ChatMessageKind::User, L"demo user"));
    }


    void AceShellUi::handleResize(int width, int height)
    {
        layout(width, height);
    }

    void AceShellUi::invalidate()
    {
        if (parent_)
        {
            InvalidateRect(parent_, nullptr, FALSE);
        }
    }

    void AceShellUi::updateCursor()
    {
        POINT p{};
        GetCursorPos(&p);
        ScreenToClient(parent_, &p);

        if (commandPalette_.active())
        {
            SetCursor(LoadCursorW(nullptr, IDC_ARROW));
        }
        else if (input_.hitTest(static_cast<float>(p.x), static_cast<float>(p.y)))
        {
            SetCursor(LoadCursorW(nullptr, IDC_IBEAM));
        }
        else if (sendButton_.rect().contains(static_cast<float>(p.x), static_cast<float>(p.y)))
        {
            SetCursor(LoadCursorW(nullptr, IDC_HAND));
        }
        else
        {
            SetCursor(LoadCursorW(nullptr, IDC_ARROW));
        }
    }


    ChatMessage AceShellUi::makeMessage(std::wstring author, std::wstring text, bool fromUser, bool system, ChatMessageKind kind, std::wstring metadata)
    {
        ChatMessage message;
        message.author = std::move(author);
        message.text = std::move(text);
        message.fromUser = fromUser;
        message.system = system;
        message.kind = kind;
        message.id = nextMessageId_++;
        message.timestamp = currentTimestamp();
        message.metadata = std::move(metadata);
        return message;
    }

    std::wstring AceShellUi::currentTimestamp() const
    {
        std::time_t now = std::time(nullptr);
        std::tm localTime{};
        localtime_s(&localTime, &now);

        std::wostringstream out;
        out << std::setfill(L'0') << std::setw(2) << localTime.tm_hour
            << L":" << std::setw(2) << localTime.tm_min
            << L":" << std::setw(2) << localTime.tm_sec;
        return out.str();
    }


    std::string AceShellUi::hresultToString(const char* label, HRESULT hr)
    {
        std::ostringstream out;
        out << label << " failed. HRESULT=0x" << std::hex << static_cast<unsigned long>(hr);
        return out.str();
    }
}
