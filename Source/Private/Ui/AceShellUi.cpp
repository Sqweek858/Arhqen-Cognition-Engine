#include "ArhqenCognitionEngine/Ui/AceShellUi.h"

#include "ArhqenCognitionEngine/Ui/Core/UiFramework.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DCyberEffects.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DCyberBackgroundField.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DCyberText.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DGlassEffects.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DBlurRuntime.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DCachedEffects.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DDisplayMetrics.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DTextLayoutFoundation.h"
#include "ArhqenCognitionEngine/Ui/Core/AceUiStyleSet.h"
#include "ArhqenCognitionEngine/AquariumRender/AceAquariumViewport.h"
#include "ArhqenCognitionEngine/AquariumRender/AceAquariumEmbeddedDx12Viewport.h"

#include <Windowsx.h>

#include <algorithm>
#include <cmath>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <utility>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

namespace
{
    D2D1_COLOR_F aceAquariumCompositeColor(float r, float g, float b, float a)
    {
        return D2D1::ColorF(
            std::clamp(r, 0.0f, 1.0f),
            std::clamp(g, 0.0f, 1.0f),
            std::clamp(b, 0.0f, 1.0f),
            std::clamp(a, 0.0f, 1.0f));
    }

    am::ui::UiRect aceRectAround(float x, float y, float width, float height)
    {
        return am::ui::makeUiRect(x - width * 0.5f, y - height * 0.5f, x + width * 0.5f, y + height * 0.5f);
    }

    am::ui::UiRect aceRectBetween(float ax, float ay, float bx, float by, float minThickness)
    {
        const float left = std::min(ax, bx);
        const float top = std::min(ay, by);
        const float right = std::max(ax, bx);
        const float bottom = std::max(ay, by);
        return am::ui::makeUiRect(left, top, std::max(left + minThickness, right), std::max(top + minThickness, bottom));
    }
}

namespace am::ui
{
    AceShellUi::~AceShellUi()
    {
        destroyAquariumResizeShieldWindow();
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
        refreshDisplayMetrics("create");
        uiStyleSet_ = AceUiStyleSet::MakeDefaultArhqen();

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

        // ACE-AQ3D2: Environment is now a full workspace, not a centered settings modal.
        environmentModalRect_ = makeUiRect(
            0.0f,
            appTopBarRect_.bottom,
            static_cast<float>(width_),
            static_cast<float>(height_)
        );
        environmentModalCloseRect_ = makeUiRect(
            environmentModalRect_.left + 18.0f,
            environmentModalRect_.top + 18.0f,
            environmentModalRect_.left + 104.0f,
            environmentModalRect_.top + 54.0f
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

        uiRetainedLayout_.Clear();
        const auto rootNode = uiRetainedLayout_.AddNode(L"root", makeUiRect(0.0f, 0.0f, static_cast<float>(width_), static_cast<float>(height_)), AceUiLayoutAxis::Vertical);
        auto* rootLayout = uiRetainedLayout_.Node(rootNode);
        if (rootLayout)
        {
            rootLayout->padding = 0.0f;
            rootLayout->gap = 0.0f;
        }
        auto topNode = uiRetainedLayout_.AddNode(L"app.topbar", appTopBarRect_, AceUiLayoutAxis::None, rootNode);
        auto sideNode = uiRetainedLayout_.AddNode(L"sidebar", sidebarRect_, AceUiLayoutAxis::Vertical, rootNode);
        auto mainNode = uiRetainedLayout_.AddNode(L"main", mainRect_, AceUiLayoutAxis::Vertical, rootNode);
        if (auto* n = uiRetainedLayout_.Node(topNode)) { n->minHeight = appTopBarRect_.height(); }
        if (auto* n = uiRetainedLayout_.Node(sideNode)) { n->minWidth = sidebarRect_.width(); n->weight = 1.0f; }
        if (auto* n = uiRetainedLayout_.Node(mainNode)) { n->weight = 1.0f; }
        uiRetainedLayout_.Arrange(rootNode, makeUiRect(0.0f, 0.0f, static_cast<float>(width_), static_cast<float>(height_)));
        ++aceUi6RetainedLayoutFrameCount_;

        if (renderTarget_)
        {
            renderTarget_->Resize(D2D1::SizeU(static_cast<UINT32>(width_), static_cast<UINT32>(height_)));
            applyPixelAlignedD2DTargetDpi();
            if (windowLiveResizeActive_)
            {
                // ACE-AQ3D8: gradient_recreation_deferred_or_documented. During
                // live resize, keep existing D2D gradient resources and rebuild
                // once on WM_EXITSIZEMOVE to avoid paint/resource churn.
                gradientsDirty_ = true;
            }
            else
            {
                createGradients(nullptr);
                gradientsDirty_ = false;
            }
        }

        invalidate();
    }

    void AceShellUi::refreshDisplayMetrics(const char* reason)
    {
        (void)reason;
        displayMetrics_ = D2DDisplayMetrics::capture(parent_);
        uiDpiScale_ = displayMetrics_.valid ? displayMetrics_.uiScale : D2DDisplayMetrics::dpiScaleForWindow(parent_);
        ++displayMetricsRefreshCount_;
    }

    float AceShellUi::dpiScale() const
    {
        return std::clamp(uiDpiScale_, 0.75f, 2.50f);
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
            ++parentPaintCount_;
            render();
            ValidateRect(parent_, nullptr);
            return 0;

        case WM_ERASEBKGND:
            if (handled) { *handled = true; }
            return 1;

        case WM_DPICHANGED:
        {
            ++dpiChangedMessageCount_;
            refreshDisplayMetrics("WM_DPICHANGED");
            if (lParam && parent_)
            {
                const RECT* suggested = reinterpret_cast<const RECT*>(lParam);
                SetWindowPos(
                    parent_,
                    nullptr,
                    suggested->left,
                    suggested->top,
                    suggested->right - suggested->left,
                    suggested->bottom - suggested->top,
                    SWP_NOZORDER | SWP_NOACTIVATE
                );
            }
            RECT client{};
            if (parent_ && GetClientRect(parent_, &client))
            {
                handleResize(static_cast<int>(client.right - client.left), static_cast<int>(client.bottom - client.top));
            }
            invalidate();
            if (handled) { *handled = true; }
            return 0;
        }

        case WM_DISPLAYCHANGE:
            ++displayChangeMessageCount_;
            refreshDisplayMetrics("WM_DISPLAYCHANGE");
            layout(width_, height_);
            if (handled) { *handled = true; }
            return 0;

        case WM_NCLBUTTONDOWN:
            // ACE-AQ3D12: arm the resize shield before the modal sizing loop
            // starts. WM_ENTERSIZEMOVE/WM_SIZING can arrive after the first
            // compositor update, which is too late for the DX12 child area.
            if (wParam == HTLEFT || wParam == HTRIGHT || wParam == HTTOP || wParam == HTBOTTOM ||
                wParam == HTTOPLEFT || wParam == HTTOPRIGHT || wParam == HTBOTTOMLEFT || wParam == HTBOTTOMRIGHT)
            {
                beginWindowLiveResize();
            }
            break;

        case WM_SIZING:
            beginWindowLiveResize();
            if (!aquariumUseSingleHwndCompositeViewport_ && handleFrozenNativeResizeSizing(lParam))
            {
                if (handled) { *handled = true; }
                return TRUE;
            }
            // ACE-AQ3D14: the single-HWND composite viewport can live-resize with
            // the parent D2D target. Do not freeze the native rect anymore; that
            // workaround only existed for the legacy child-HWND swapchain path.
            break;

        case WM_ENTERSIZEMOVE:
            beginWindowLiveResize();
            if (handled) { *handled = true; }
            return 0;

        case WM_EXITSIZEMOVE:
            endWindowLiveResize();
            if (handled) { *handled = true; }
            return 0;

        case WM_SIZE:
            if (wParam != SIZE_MINIMIZED)
            {
                handleResize(LOWORD(lParam), HIWORD(lParam));
            }
            if (handled) { *handled = true; }
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
                    invalidateAquariumChrome();
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
                    if (aquarium3DModeActive_)
                    {
                        aquarium3DModeActive_ = false;
                        aquariumEmbeddedViewportVisible_ = false;
                        aquariumEmbeddedDx12Viewport_.Hide();
                        aquariumEmbeddedViewportStatus_ = L"DX12 Environment ready";
                    }
                    else
                    {
                        environmentOpen_ = false;
                        aquariumEmbeddedViewportVisible_ = false;
                        aquariumEmbeddedDx12Viewport_.Hide();
                    }
                    invalidate();
                    if (handled) { *handled = true; }
                    return 0;
                }

                if (environmentModalRect_.contains(x, y))
                {
                    if (beginAquariumPanelResize(x, y))
                    {
                        mouseCaptured_ = true;
                        SetCapture(parent_);
                        invalidateAquariumChrome();
                        if (handled) { *handled = true; }
                        return 0;
                    }

                    if (beginAquariumScrollbarDrag(x, y))
                    {
                        mouseCaptured_ = true;
                        SetCapture(parent_);
                        invalidateAquariumChrome();
                        if (handled) { *handled = true; }
                        return 0;
                    }

                    if (handleAquariumPanelClick(x, y))
                    {
                        // ACE-AQ3D7: button press updates only the Environment chrome;
                        // it must not full-invalidate the parent or touch the child DX12 HWND.
                        invalidateAquariumChrome();
                    }

                    if (handled) { *handled = true; }
                    return 0;
                }
            }

            if (environmentButtonRect_.contains(x, y))
            {
                environmentOpen_ = true;
                aquarium3DModeActive_ = false;
                aquariumEmbeddedViewportVisible_ = false;
                aquariumContentScroll_.offset = 0.0f;
                aquariumEmbeddedViewportStatus_ = L"DX12 Environment ready";
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

            if (aquariumPanelResizeTarget_ != AquariumPanelResizeTarget::None)
            {
                endAquariumPanelResize();
                if (mouseCaptured_)
                {
                    mouseCaptured_ = false;
                    ReleaseCapture();
                }
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }

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

            if (aquariumPanelResizeTarget_ != AquariumPanelResizeTarget::None)
            {
                updateAquariumPanelResize(x, y);
                invalidateAquariumChrome();
                if (handled) { *handled = true; }
                return 0;
            }

            if (aquariumDraggingScroll_)
            {
                updateAquariumScrollbarDrag(x, y);
                invalidateAquariumChrome();
                if (handled) { *handled = true; }
                return 0;
            }

            bool changed = false;

            mouseX_ = x;
            mouseY_ = y;
            cyberBackground_.setMouse(x, y);

            if (environmentOpen_ && environmentModalRect_.contains(x, y))
            {
                // ACE-AQ3D6: hover/click stability. While the Environment UI is
                // active, do not run hover logic for the chat shell underneath it.
                // A hover-only move changes exactly one hot-id visual state and
                // does not structurally rebuild the 3D UI layout.
                const int oldHot = aquariumHoverHotId_;
                aquariumHoverHotId_ = aquariumHotIdAt(x, y);
                if (oldHot != aquariumHoverHotId_)
                {
                    ++aquariumHoverStateChangeCount_;
                    ++hoverInvalidationCount_;
                    invalidateRect(inflateRect(aquariumHotRectById(oldHot), 10.0f));
                    invalidateRect(inflateRect(aquariumHotRectById(aquariumHoverHotId_), 10.0f));
                }
                if (handled) { *handled = true; }
                return 0;
            }

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

            if (wParam == VK_F9)
            {
                uiDebugOverlay_.Toggle();
                ++aceUi9DebugOverlayToggleCount_;
                refreshUiDebugOverlay();
                showToast(L"UI Debug", uiDebugOverlay_.Visible() ? L"UI debug overlay opened." : L"UI debug overlay closed.", D2DToastKind::Info);
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

            if (aquariumUseSingleHwndCompositeViewport_ && environmentOpen_ && aquarium3DModeActive_)
            {
                // ACE-AQ3D14: Slate-style single-HWND viewport composition.
                // The 3D workspace no longer drives a child DX12 HWND in the
                // main path, so hover/click/resize cannot race D2D parent paint
                // against a flip-model child swapchain. The scene is composed by
                // the D2D shell from the retained Aquarium render model.
                if (aquariumEmbeddedDx12Viewport_.IsVisible())
                {
                    aquariumEmbeddedDx12Viewport_.Hide();
                    ++aquariumLegacyChildSuppressedCount_;
                }
                aquariumEmbeddedViewportSyncNeeded_ = false;
                aquariumEmbeddedDx12Viewport_.SetResizeApplySuspended(true);

                if (aquariumController_.IsRunning() || windowLiveResizeActive_)
                {
                    invalidateRect(aquariumEmbeddedViewportRect_.empty() ? mainRect_ : aquariumEmbeddedViewportRect_);
                }
            }
            else if (windowLiveResizeActive_)
            {
                // ACE-AQ3D8: freeze the embedded DX12 child surface during live
                // window resize. WM_SIZE spam updates pending dimensions only;
                // the final child HWND sync and resize apply happen after
                // WM_EXITSIZEMOVE. This avoids resize flicker while dragging.
                aquariumEmbeddedDx12Viewport_.SetResizeApplySuspended(true);
            }
            else
            {
                syncAquariumEmbeddedViewportWindow();

                if (environmentOpen_ && aquarium3DModeActive_ && aquariumEmbeddedViewportVisible_ && aquariumEmbeddedDx12Viewport_.IsVisible())
                {
                    std::string viewportError;
                    if (!aquariumEmbeddedDx12Viewport_.RenderFrame(aquariumController_, aquariumSceneAdapter_, aquariumController_.DebugTruthEnabled(), &viewportError))
                    {
                        aquariumEmbeddedViewportStatus_ = L"DX12 Environment renderer unavailable; check logs.";
                    }
                    else if (aquariumEmbeddedDx12Viewport_.IsVisible())
                    {
                        aquariumEmbeddedViewportStatus_ = L"DX12 Environment Mode active";
                    }
                }
            }
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

        // ACE-AQ3D5: no_flicker_idle_marker. In 3D environment mode the DX12
        // child surface renders independently, so the D2D shell is not invalidated
        // every idle tick. This keeps panel rects and the viewport rect stable and
        // prevents UI/DX12 clear order flicker while paused.
        if (environmentOpen_ && aquarium3DModeActive_)
        {
            if (!windowLiveResizeActive_ && aquariumControllerReady_ && aquariumController_.IsRunning())
            {
                invalidate();
            }
        }
        else
        {
            // ACE-CLEAN0: background, text pulse and hover glow are intentionally alive.
            invalidate();
        }
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
                96.0f,
                96.0f
            ),
            D2D1::HwndRenderTargetProperties(
                parent_,
                D2D1::SizeU(static_cast<UINT32>(rc.right - rc.left), static_cast<UINT32>(rc.bottom - rc.top)),
                D2D1_PRESENT_OPTIONS_RETAIN_CONTENTS
            ),
            &renderTarget_
        );

        if (FAILED(hr))
        {
            if (error) { *error = hresultToString("ID2D1Factory::CreateHwndRenderTarget", hr); }
            return false;
        }

        applyPixelAlignedD2DTargetDpi();

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
        if (!createTextFormat(L"Segoe UI", 32.0f, DWRITE_FONT_WEIGHT_SEMI_BOLD, titleFormat_, error)) { return false; }
        if (!createTextFormat(L"Segoe UI", 16.0f, DWRITE_FONT_WEIGHT_NORMAL, subtitleFormat_, error)) { return false; }
        if (!createTextFormat(L"Segoe UI", 19.0f, DWRITE_FONT_WEIGHT_NORMAL, bodyFormat_, error)) { return false; }
        if (!createTextFormat(L"Segoe UI", 19.0f, DWRITE_FONT_WEIGHT_SEMI_BOLD, bodyStrongFormat_, error)) { return false; }
        if (!createTextFormat(L"Segoe UI", 15.0f, DWRITE_FONT_WEIGHT_NORMAL, smallFormat_, error)) { return false; }
        if (!createTextFormat(L"Cascadia Mono", 16.0f, DWRITE_FONT_WEIGHT_NORMAL, monoFormat_, error)) { return false; }
        if (!createTextFormat(L"Segoe UI", 19.0f, DWRITE_FONT_WEIGHT_SEMI_BOLD, buttonFormat_, error)) { return false; }

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
        D2DCachedEffects::reset();
    }

    void AceShellUi::applyPixelAlignedD2DTargetDpi()
    {
        if (!renderTarget_)
        {
            return;
        }

        // ACE-UI3F: keep the HwndRenderTarget in explicit 96-DPI DIPs so the
        // existing Arhqen UI layout remains pixel-space. UI3 still captures
        // monitor DPI for placement/scaling diagnostics, but D2D must not
        // implicitly scale every coordinate on high-DPI monitors. That was
        // the bug that pushed panels/text far outside their intended rects.
        renderTarget_->SetDpi(96.0f, 96.0f);
        ++d2dPixelDpiFixApplyCount_;
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
        ctx.dpiScale = dpiScale();
        ctx.dpiX = displayMetrics_.nearestMonitor.dpiX;
        ctx.dpiY = displayMetrics_.nearestMonitor.dpiY;

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
            {L"ui_debug", L"UI debug overlay", L"Toggle retained-layout/draw/dirty rect debug overlay.", L"F9"},
            {L"ui_stats", L"UI subsystem stats", L"Append UI5-UI11 subsystem counters.", L""},
            {L"seed_demo", L"Seed demo messages", L"Add messages for scroll testing.", L""},
            {L"focus_input", L"Focus input", L"Move keyboard focus back to input.", L"Tab"}
        });

        shortcutHelp_.setItems({
            {L"Ctrl+K / Ctrl+P", L"Command palette", L"Global"},
            {L"F1", L"Shortcut help", L"Global"},
            {L"F12", L"Diagnostics", L"Global"},
            {L"F9", L"UI debug overlay", L"Development"},
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
        uiDrawCommands_.BeginFrame();
        ++aceUi7DrawCommandFrameCount_;
        uiDrawCommands_.RoundedRect(mainRect_, 18.0f, 0);

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
        if (!environmentOpen_)
        {
            renderAppTopBar(ctx);
            renderSideNav(ctx);
        }

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
        renderUiDebugOverlay(ctx);

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

    int AceShellUi::aquariumHotIdAt(float x, float y) const
    {
        // ACE-AQ3D6: hover_state_change_count is based on stable hit ids, not on
        // rebuilding every control tree during simple mouse motion. The order is
        // deterministic and covers only live Aquarium UI rects, so underlying chat
        // shell controls cannot flicker the 3D mode on hover.
        const UiRect rects[] = {
            aquariumDetailsToggleRect_,
            aquariumLogsToggleRect_,
            aquariumDebugRect_,
            environmentModalCloseRect_,
            aquariumScenarioPrevRect_,
            aquariumScenarioNextRect_,
            aquariumPlannerPrevRect_,
            aquariumPlannerNextRect_,
            aquariumResetRect_,
            aquariumStepRect_,
            aquariumRunPauseRect_,
            aquariumCameraResetRect_,
            aquariumDetailsDebugRect_,
            aquariumManualForwardRect_,
            aquariumManualLeftRect_,
            aquariumManualRightRect_,
            aquariumManualWaitRect_,
            aquariumManualTouchRect_,
            aquariumManualConsumeRect_,
            aquariumManualPushRect_,
            aquariumLeftResizeHandleRect_,
            aquariumRightResizeHandleRect_
        };

        for (int i = 0; i < static_cast<int>(sizeof(rects) / sizeof(rects[0])); ++i)
        {
            if (rects[i].contains(x, y))
            {
                return i;
            }
        }

        if (aquarium3DLogsScroll_.thumb.contains(x, y) || aquarium3DLogsScroll_.track.contains(x, y))
        {
            return 100;
        }

        if (aquariumLogScroll_.thumb.contains(x, y) || aquariumLogScroll_.track.contains(x, y))
        {
            return 101;
        }

        return -1;
    }

    UiRect AceShellUi::aquariumHotRectById(int hotId) const
    {
        const UiRect rects[] = {
            aquariumDetailsToggleRect_,
            aquariumLogsToggleRect_,
            aquariumDebugRect_,
            environmentModalCloseRect_,
            aquariumScenarioPrevRect_,
            aquariumScenarioNextRect_,
            aquariumPlannerPrevRect_,
            aquariumPlannerNextRect_,
            aquariumResetRect_,
            aquariumStepRect_,
            aquariumRunPauseRect_,
            aquariumCameraResetRect_,
            aquariumDetailsDebugRect_,
            aquariumManualForwardRect_,
            aquariumManualLeftRect_,
            aquariumManualRightRect_,
            aquariumManualWaitRect_,
            aquariumManualTouchRect_,
            aquariumManualConsumeRect_,
            aquariumManualPushRect_,
            aquariumLeftResizeHandleRect_,
            aquariumRightResizeHandleRect_
        };

        if (hotId >= 0 && hotId < static_cast<int>(sizeof(rects) / sizeof(rects[0])))
        {
            return rects[hotId];
        }

        if (hotId == 100)
        {
            return aquarium3DLogsScroll_.track.empty() ? aquariumRightLogsPanelRect_ : aquarium3DLogsScroll_.track;
        }

        if (hotId == 101)
        {
            return aquariumLogScroll_.track;
        }

        return am::ui::makeUiRect(0, 0, 0, 0);
    }



    void AceShellUi::renderAquariumDx12ViewportSurface(D2DRenderContext& ctx, UiRect rect, bool debugTruthEnabled)
    {
        (void)debugTruthEnabled;

        if (aquariumUseSingleHwndCompositeViewport_)
        {
            aquariumEmbeddedViewportRect_ = rect;
            aquariumPendingViewportRect_ = rect;
            aquariumPendingViewportValid_ = true;
            aquariumEmbeddedViewportVisible_ = false;
            aquariumEmbeddedViewportSyncNeeded_ = false;
            renderAquariumSlateCompositeViewport(ctx, rect, debugTruthEnabled);
            aquariumEmbeddedViewportStatus_ = L"Single-HWND composited viewport active";
            return;
        }

        // ACE-AQ3D7: child_hwnd_sync_not_called_from_render_path. Render only
        // publishes the desired stable child HWND rect. Child HWND placement and
        // visibility sync happens later, outside WM_PAINT.
        const bool viewportRectChanged =
            !aquariumPendingViewportValid_ ||
            std::fabs(aquariumPendingViewportRect_.left - rect.left) > 0.5f ||
            std::fabs(aquariumPendingViewportRect_.top - rect.top) > 0.5f ||
            std::fabs(aquariumPendingViewportRect_.right - rect.right) > 0.5f ||
            std::fabs(aquariumPendingViewportRect_.bottom - rect.bottom) > 0.5f;
        aquariumEmbeddedViewportRect_ = rect;
        aquariumPendingViewportRect_ = rect;
        aquariumPendingViewportValid_ = true;
        aquariumEmbeddedViewportSyncNeeded_ = aquariumEmbeddedViewportSyncNeeded_ || viewportRectChanged || !aquariumEmbeddedDx12Viewport_.IsVisible();

        const float left = aquariumPendingViewportRect_.left;
        const float top = aquariumPendingViewportRect_.top;
        const float right = aquariumPendingViewportRect_.right;
        const float bottom = aquariumPendingViewportRect_.bottom;
        if (!aquariumEmbeddedViewportVisible_ || !environmentOpen_ || !aquarium3DModeActive_ || right <= left + 64.0f || bottom <= top + 64.0f)
        {
            aquariumEmbeddedViewportVisible_ = false;
            aquariumEmbeddedViewportStatus_ = L"DX12 Environment pending hidden";
            return;
        }

        // ACE-AQ3D9: during live resize the flip-model child HWND is hidden and
        // the D2D parent paints a stable proxy in the same rect. This keeps the
        // parent render target up to date while avoiding child swapchain flicker.
        if (windowLiveResizeActive_ || aquariumViewportHiddenForLiveResize_)
        {
            renderAquariumResizeProxyViewport(ctx, rect);
            aquariumEmbeddedViewportStatus_ = L"Resizing viewport proxy active";
            return;
        }

        aquariumEmbeddedViewportStatus_ = L"DX12 Environment Mode active";
    }

    void AceShellUi::renderAquariumResizeProxyViewport(D2DRenderContext& ctx, UiRect rect)
    {
        if (rect.empty())
        {
            return;
        }

        ++liveResizeProxyPaintCount_;

        D2DWidgetUtils::fillRect(ctx, rect, ctx.brushes.panelDeep);

        if (ctx.target)
        {
            ctx.target->PushAxisAlignedClip(rect.d2d(), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

            if (ctx.brushes.borderDim)
            {
                ctx.brushes.borderDim->SetOpacity(0.34f);
                const float grid = 32.0f;
                for (float x = rect.left; x <= rect.right; x += grid)
                {
                    ctx.target->DrawLine(D2D1::Point2F(x, rect.top), D2D1::Point2F(x, rect.bottom), ctx.brushes.borderDim, 1.0f);
                }
                for (float y = rect.top; y <= rect.bottom; y += grid)
                {
                    ctx.target->DrawLine(D2D1::Point2F(rect.left, y), D2D1::Point2F(rect.right, y), ctx.brushes.borderDim, 1.0f);
                }
                ctx.brushes.borderDim->SetOpacity(1.0f);
            }

            if (ctx.brushes.accent)
            {
                const float cx = rect.left + rect.width() * 0.5f;
                const float cy = rect.top + rect.height() * 0.5f;
                ctx.target->FillEllipse(D2D1::Ellipse(D2D1::Point2F(cx, cy), 7.0f, 7.0f), ctx.brushes.accent);
                ctx.target->DrawLine(D2D1::Point2F(cx, cy), D2D1::Point2F(cx + 24.0f, cy - 12.0f), ctx.brushes.accent, 2.0f);
            }

            ctx.target->PopAxisAlignedClip();
        }

        const UiRect label = makeUiRect(rect.left + 18.0f, rect.top + 16.0f, rect.right - 18.0f, rect.top + 44.0f);
        D2DWidgetUtils::drawTextEx(ctx, L"Resizing viewport...", FontRole::Small, label, ctx.brushes.muted);
    }

    void AceShellUi::renderAquariumSlateCompositeViewport(D2DRenderContext& ctx, UiRect rect, bool debugTruthEnabled)
    {
        if (rect.empty() || !ctx.target)
        {
            return;
        }

        ++aquariumCompositeFrameCount_;
        if (windowLiveResizeActive_)
        {
            ++aquariumCompositeCachedResizeFrameCount_;
        }

        aquariumCompositeViewportLastRect_ = rect;
        aquariumCompositeViewportFrameValid_ = true;

        if (aquariumEmbeddedDx12Viewport_.IsVisible())
        {
            aquariumEmbeddedDx12Viewport_.Hide();
            ++aquariumLegacyChildSuppressedCount_;
        }

        D2DWidgetUtils::fillRect(ctx, rect, ctx.brushes.panelDeep);
        ctx.target->PushAxisAlignedClip(rect.d2d(), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

        auto makeBrush = [&](float r, float g, float b, float a) -> Microsoft::WRL::ComPtr<ID2D1SolidColorBrush>
        {
            Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
            ctx.target->CreateSolidColorBrush(aceAquariumCompositeColor(r, g, b, a), brush.GetAddressOf());
            return brush;
        };

        auto fill = [&](UiRect r, float cr, float cg, float cb, float ca)
        {
            if (r.empty())
            {
                return;
            }
            auto brush = makeBrush(cr, cg, cb, ca);
            if (brush)
            {
                ctx.target->FillRectangle(r.d2d(), brush.Get());
            }
        };

        auto line = [&](float ax, float ay, float bx, float by, float thickness, float cr, float cg, float cb, float ca)
        {
            auto brush = makeBrush(cr, cg, cb, ca);
            if (brush)
            {
                ctx.target->DrawLine(D2D1::Point2F(ax, ay), D2D1::Point2F(bx, by), brush.Get(), thickness);
            }
        };

        fill(rect, 0.008f, 0.018f, 0.040f, 1.0f);
        fill(makeUiRect(rect.left, rect.top, rect.right, std::min(rect.bottom, rect.top + 28.0f)),
             debugTruthEnabled ? 0.52f : 0.020f,
             debugTruthEnabled ? 0.20f : 0.060f,
             debugTruthEnabled ? 0.06f : 0.110f,
             0.92f);

        if (!aquariumControllerReady_)
        {
            ctx.target->PopAxisAlignedClip();
            D2DWidgetUtils::drawTextEx(ctx, L"Aquarium runtime not ready", FontRole::Small, rect.inset(18.0f), ctx.brushes.muted);
            return;
        }

        const float surfaceW = std::max(1.0f, rect.width());
        const float surfaceH = std::max(1.0f, rect.height());
        const auto primitives = aquariumSceneAdapter_.BuildPrimitives(aquariumController_, debugTruthEnabled);

        float maxX = 1.0f;
        float maxY = 1.0f;
        for (const auto& primitive : primitives)
        {
            maxX = std::max(maxX, primitive.X + std::max(0.0f, primitive.SizeX));
            maxY = std::max(maxY, primitive.Y + std::max(0.0f, primitive.SizeY));
        }

        auto camera = aquariumCamera_;
        camera.Zoom = std::clamp(
            std::min(
                surfaceW / std::max(maxX + maxY + 2.0f, 1.0f),
                (surfaceH - 44.0f) / std::max((maxX + maxY) * 0.62f + 3.0f, 1.0f)
            ) * 2.86f,
            26.0f,
            108.0f);
        camera.OriginY = std::max(28.0f, surfaceH * 0.105f);

        const auto model = aquariumViewport_.BuildViewportModel(primitives, camera, surfaceW, surfaceH);
        const float tileW = std::clamp(camera.Zoom * 0.86f, 28.0f, 76.0f);
        const float tileH = std::clamp(camera.Zoom * 0.42f, 14.0f, 38.0f);
        const float blockW = std::clamp(camera.Zoom * 0.72f, 28.0f, 70.0f);
        const float blockH = std::clamp(camera.Zoom * 1.04f, 36.0f, 96.0f);
        const float agentW = std::clamp(camera.Zoom * 0.92f, 40.0f, 86.0f);
        const float agentH = std::clamp(camera.Zoom * 1.22f, 52.0f, 116.0f);

        auto ox = [&](float x) { return rect.left + x; };
        auto oy = [&](float y) { return rect.top + y; };

        for (const auto& primitive : model)
        {
            const float pr = primitive.R;
            const float pg = primitive.G;
            const float pb = primitive.Bc;
            const float pa = primitive.Aalpha;
            const float ax = ox(primitive.A.X);
            const float ay = oy(primitive.A.Y);
            const float bx = ox(primitive.B.X);
            const float by = oy(primitive.B.Y);

            switch (primitive.Kind)
            {
            case ace::aquarium_render::AceAqRenderPrimitiveKind::GridLine:
                line(ax, ay, bx, by, 1.0f, pr, pg, pb, pa);
                break;

            case ace::aquarium_render::AceAqRenderPrimitiveKind::Tile:
                fill(aceRectAround(ax, ay, tileW, tileH), pr, pg, pb, pa);
                break;

            case ace::aquarium_render::AceAqRenderPrimitiveKind::Block:
                fill(aceRectAround(ax, ay, blockW, blockH), pr, pg, pb, pa);
                fill(makeUiRect(ax - blockW * 0.35f, ay - blockH * 0.56f, ax + blockW * 0.35f, ay - blockH * 0.42f),
                     pr + 0.12f, pg + 0.12f, pb + 0.12f, pa);
                break;

            case ace::aquarium_render::AceAqRenderPrimitiveKind::Agent:
                fill(aceRectAround(ax, ay, agentW * 1.42f, agentH * 1.16f), 0.08f, 0.50f, 0.72f, 0.22f);
                fill(aceRectAround(ax, ay, agentW, agentH), 0.10f, 0.95f, 1.0f, 0.92f);
                break;

            case ace::aquarium_render::AceAqRenderPrimitiveKind::DirectionArrow:
                fill(aceRectBetween(ax, ay, bx, by, std::max(4.0f, camera.Zoom * 0.08f)), 1.0f, 0.90f, 0.20f, 0.92f);
                fill(aceRectAround(bx, by, std::max(8.0f, camera.Zoom * 0.16f), std::max(8.0f, camera.Zoom * 0.16f)), 1.0f, 0.90f, 0.20f, 0.92f);
                break;

            case ace::aquarium_render::AceAqRenderPrimitiveKind::Highlight:
                fill(aceRectAround(ax, ay, tileW * 1.16f, tileH * 1.55f), 1.0f, 0.82f, 0.20f, 0.34f);
                break;

            case ace::aquarium_render::AceAqRenderPrimitiveKind::DebugLabel:
                if (debugTruthEnabled)
                {
                    fill(aceRectAround(ax, ay, 18.0f, 6.0f), 1.0f, 0.55f, 0.18f, 0.90f);
                }
                break;
            }
        }

        ctx.target->PopAxisAlignedClip();

        const UiRect label = makeUiRect(rect.left + 16.0f, rect.top + 8.0f, rect.right - 16.0f, rect.top + 30.0f);
        D2DWidgetUtils::drawTextEx(ctx,
            debugTruthEnabled ? L"DEBUG TRUTH - NOT AGENT INPUT | Single-HWND composite viewport" : L"Single-HWND composite viewport",
            FontRole::Small,
            label,
            debugTruthEnabled ? ctx.brushes.accentWarm : ctx.brushes.muted,
            DWRITE_TEXT_ALIGNMENT_LEADING);
    }

    void AceShellUi::syncAquariumEmbeddedViewportWindow()
    {
        if (aquariumUseSingleHwndCompositeViewport_)
        {
            // ACE-AQ3D14: main 3D path is Slate-style single-HWND composition.
            // The legacy child HWND is suppressed so resize cannot flicker through
            // a separate DWM surface.
            if (aquariumEmbeddedDx12Viewport_.IsVisible())
            {
                aquariumEmbeddedDx12Viewport_.Hide();
                ++aquariumLegacyChildSuppressedCount_;
            }
            aquariumEmbeddedViewportVisible_ = false;
            aquariumEmbeddedViewportSyncNeeded_ = false;
            return;
        }

        // ACE-AQ3D7: stable child HWND sync. This is the only place that may
        // update embedded DX12 child visibility/placement; hover/click paints
        // update dirty D2D rects only and do not resize/recreate the child.
        if (windowLiveResizeActive_)
        {
            // ACE-AQ3D8: live_resize_defers_child_hwnd_sync and
            // sync_viewport_window_guarded_during_live_resize. Do not Show/Hide,
            // SetWindowPos, MoveWindow, or indirectly resize DX12 while the user
            // is dragging the top-level window border. The final rect is applied
            // once after WM_EXITSIZEMOVE.
            aquariumEmbeddedViewportSyncNeeded_ = true;
            return;
        }

        const bool wantVisible = environmentOpen_ && aquarium3DModeActive_ && aquariumEmbeddedViewportVisible_ && aquariumPendingViewportValid_;
        if (!wantVisible)
        {
            aquariumEmbeddedDx12Viewport_.Hide();
            aquariumViewportHiddenForLiveResize_ = false;
            aquariumViewportWasVisibleBeforeLiveResize_ = false;
            return;
        }

        const UiRect rect = aquariumPendingViewportRect_;
        const float left = rect.left;
        const float top = rect.top;
        const float right = rect.right;
        const float bottom = rect.bottom;
        if (right <= left + 64.0f || bottom <= top + 64.0f)
        {
            aquariumEmbeddedDx12Viewport_.Hide();
            return;
        }

        const int x = static_cast<int>(std::round(left));
        const int y = static_cast<int>(std::round(top));
        const int w = static_cast<int>(std::round(right - left));
        const int h = static_cast<int>(std::round(bottom - top));

        if (!aquariumEmbeddedViewportSyncNeeded_ &&
            aquariumEmbeddedDx12Viewport_.IsVisible() &&
            aquariumEmbeddedDx12Viewport_.X() == x &&
            aquariumEmbeddedDx12Viewport_.Y() == y &&
            aquariumEmbeddedDx12Viewport_.Width() == w &&
            aquariumEmbeddedDx12Viewport_.Height() == h)
        {
            return;
        }

        std::string viewportError;
        ++childSyncCount_;
        if (!aquariumEmbeddedDx12Viewport_.Show(parent_, x, y, w, h, &viewportError))
        {
            aquariumEmbeddedViewportStatus_ = L"DX12 Environment renderer unavailable; check logs.";
            aquariumEmbeddedDx12Viewport_.Hide();
            return;
        }

        if (aquariumViewportHiddenForLiveResize_)
        {
            ++viewportShowAfterLiveResizeCount_;
        }
        aquariumViewportHiddenForLiveResize_ = false;
        aquariumViewportWasVisibleBeforeLiveResize_ = false;
        aquariumEmbeddedViewportSyncNeeded_ = false;
        aquariumEmbeddedViewportStatus_ = L"DX12 Environment Mode active";
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

    void AceShellUi::renderAquariumMiniButton(D2DRenderContext& ctx, UiRect rect, const std::wstring& label, bool active)
    {
        const bool hovered = isAquariumButtonHovered(rect);
        D2DGlassMaterial buttonGlass;
        buttonGlass.radius = 6.0f;
        buttonGlass.fillAlpha = active ? 0.36f : (hovered ? 0.28f : 0.18f);
        buttonGlass.borderAlpha = active ? 0.72f : (hovered ? 0.52f : 0.32f);
        buttonGlass.highlightAlpha = active ? 0.22f : 0.14f;
        buttonGlass.glowAlpha = active ? 0.26f : (hovered ? 0.18f : 0.08f);
        buttonGlass.shadowAlpha = 0.04f;
        buttonGlass.blurFallbackAlpha = 0.08f;
        buttonGlass.useCornerTicks = false;
        D2DGlassEffects::drawGlassPanel(ctx, rect, buttonGlass);
        if (hovered || active)
        {
            D2DCyberEffects::drawBorderGlow(ctx, rect, 6.0f, active ? 0.34f : 0.20f);
        }

        D2DWidgetUtils::drawTextEx(ctx, label, FontRole::Small, rect, (active || hovered) ? ctx.brushes.text : ctx.brushes.muted, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }

    void AceShellUi::renderAquariumResizeHandle(D2DRenderContext& ctx, UiRect rect, bool leftPanelHandle)
    {
        if (rect.empty())
        {
            return;
        }

        const bool active = (leftPanelHandle && aquariumPanelResizeTarget_ == AquariumPanelResizeTarget::LeftDetails) ||
            (!leftPanelHandle && aquariumPanelResizeTarget_ == AquariumPanelResizeTarget::RightLogs);
        const bool hovered = rect.contains(mouseX_, mouseY_);
        D2DWidgetUtils::fillRounded(ctx, rect, 5.0f, ctx.brushes.panelDeep, hovered || active ? ctx.brushes.accent : ctx.brushes.borderDim, 1.0f);

        const float oldOpacity = ctx.brushes.accent ? ctx.brushes.accent->GetOpacity() : 1.0f;
        if (ctx.brushes.accent)
        {
            ctx.brushes.accent->SetOpacity(active ? 0.88f : (hovered ? 0.62f : 0.36f));
            const float x0 = leftPanelHandle ? rect.left + 5.0f : rect.right - 8.0f;
            const float x1 = leftPanelHandle ? rect.left + 8.0f : rect.right - 5.0f;
            for (int i = 0; i < 3; ++i)
            {
                const float y = rect.bottom - 5.0f - static_cast<float>(i) * 4.0f;
                const UiRect mark = leftPanelHandle
                    ? makeUiRect(x0 + static_cast<float>(i) * 3.0f, y, rect.right - 4.0f, y + 1.5f)
                    : makeUiRect(rect.left + 4.0f, y, x1 - static_cast<float>(i) * 3.0f, y + 1.5f);
                D2DWidgetUtils::fillRounded(ctx, mark, 1.0f, ctx.brushes.accent);
            }
            ctx.brushes.accent->SetOpacity(oldOpacity);
        }
    }

    void AceShellUi::clampAquariumScroll(AquariumScrollPanel& scroll)
    {
        scroll.maxScroll = std::max(0.0f, scroll.contentHeight - scroll.viewportHeight);
        scroll.offset = std::clamp(scroll.offset, 0.0f, scroll.maxScroll);
        const float lineHeight = 19.0f;
        scroll.visibleLogStart = static_cast<std::size_t>(std::max(0.0f, std::floor(scroll.offset / lineHeight)));
        const float visibleBottom = scroll.offset + scroll.viewportHeight;
        scroll.visibleLogEnd = std::min(scroll.lineCount, static_cast<std::size_t>(std::ceil(visibleBottom / lineHeight)) + 1U);
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
        if (rect.empty() || rect.height() < 18.0f || rect.width() < 24.0f)
        {
            return;
        }

        // ACE-AQ3D6: section draw is clipped to its computed section_rect. Static
        // cards stay static; scrollbars are only rendered by renderAquariumScrollableLines
        // for explicitly long sections. This keeps panel_content_clipped_to_content_rect
        // true even when the panel is resized small.
        const bool pushedClip = ctx.target != nullptr;
        if (pushedClip)
        {
            ctx.target->PushAxisAlignedClip(rect.d2d(), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        }

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
        const float lineHeight = 19.0f;
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

        if (pushedClip)
        {
            ctx.target->PopAxisAlignedClip();
        }
    }

    void AceShellUi::renderAquariumScrollableLines(D2DRenderContext& ctx, const std::wstring& title, const std::vector<std::string>& lines, UiRect rect, AquariumScrollPanel& scroll, std::size_t maxVisibleLines)
    {
        if (rect.empty() || rect.height() < 36.0f || rect.width() < 40.0f)
        {
            scroll.viewport = {};
            scroll.track = {};
            scroll.thumb = {};
            scroll.viewportHeight = 0.0f;
            scroll.contentHeight = 0.0f;
            scroll.maxScroll = 0.0f;
            scroll.visibleLogStart = 0;
            scroll.visibleLogEnd = 0;
            return;
        }

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

        const float lineHeight = 19.0f;
        const float viewportTop = rect.top + 36.0f;
        const float viewportBottom = rect.bottom - 8.0f;
        const float scrollBarWidth = 8.0f;

        scroll.viewport = makeUiRect(rect.left + 10.0f, viewportTop, rect.right - 14.0f - scrollBarWidth, viewportBottom);
        scroll.track = makeUiRect(rect.right - 12.0f, viewportTop + 1.0f, rect.right - 6.0f, viewportBottom - 1.0f);
        const float previousMaxScroll = scroll.maxScroll;
        const bool wasAtBottom = scroll.offset >= previousMaxScroll - 2.0f;
        scroll.viewportHeight = std::max(0.0f, scroll.viewport.height());
        scroll.lineCount = lines.size();
        scroll.contentHeight = static_cast<float>(lines.size()) * lineHeight;
        scroll.maxScroll = std::max(0.0f, scroll.contentHeight - scroll.viewportHeight);
        if (scroll.autoScrollWhenAtBottom && (!scroll.userScrolled || wasAtBottom))
        {
            scroll.offset = scroll.maxScroll;
            scroll.userScrolled = false;
        }
        clampAquariumScroll(scroll);

        if (ctx.target)
        {
            ctx.target->PushAxisAlignedClip(scroll.viewport.d2d(), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        }

        (void)maxVisibleLines;
        float y = scroll.viewport.top - scroll.offset;
        for (const auto& line : lines)
        {
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
            &aquariumLogScroll_,
            &aquarium3DLogsScroll_,
            &aquarium3DDetailsScroll_,
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
        panel->userScrolled = true;
        clampAquariumScroll(*panel);
        panel->autoScrollWhenAtBottom = panel->offset >= panel->maxScroll - 2.0f;
        if (panel->autoScrollWhenAtBottom)
        {
            panel->userScrolled = false;
        }
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
        panel->userScrolled = true;
        panel->autoScrollWhenAtBottom = false;
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
        scroll.userScrolled = true;
        clampAquariumScroll(scroll);
        scroll.autoScrollWhenAtBottom = scroll.offset >= scroll.maxScroll - 2.0f;
        if (scroll.autoScrollWhenAtBottom)
        {
            scroll.userScrolled = false;
        }
        return true;
    }

    void AceShellUi::endAquariumScrollbarDrag()
    {
        aquariumDraggingScroll_ = nullptr;
        aquariumDragStartY_ = 0.0f;
        aquariumDragStartOffset_ = 0.0f;
    }

    bool AceShellUi::beginAquariumPanelResize(float x, float y)
    {
        if (!environmentOpen_ || !aquarium3DModeActive_)
        {
            return false;
        }

        if (aquariumLeftResizeHandleRect_.contains(x, y))
        {
            aquariumPanelResizeTarget_ = AquariumPanelResizeTarget::LeftDetails;
        }
        else if (aquariumRightResizeHandleRect_.contains(x, y))
        {
            aquariumPanelResizeTarget_ = AquariumPanelResizeTarget::RightLogs;
        }
        else
        {
            return false;
        }

        aquariumPanelResizeStartX_ = x;
        aquariumPanelResizeStartY_ = y;
        aquariumPanelResizeStartState_ = aquarium3DPanelState_;
        return true;
    }

    bool AceShellUi::updateAquariumPanelResize(float x, float y)
    {
        if (aquariumPanelResizeTarget_ == AquariumPanelResizeTarget::None)
        {
            return false;
        }

        if (aquariumPanelResizeTarget_ == AquariumPanelResizeTarget::LeftDetails)
        {
            const float requestedWidth = aquariumPanelResizeStartState_.detailsWidth + (x - aquariumPanelResizeStartX_);
            const float requestedHeight = aquariumPanelResizeStartState_.detailsHeight + (y - aquariumPanelResizeStartY_);
            environment3DMode_.ResizeLeftPanel(aquarium3DPanelState_, requestedWidth, requestedHeight, static_cast<float>(width_), static_cast<float>(height_));
            return true;
        }

        if (aquariumPanelResizeTarget_ == AquariumPanelResizeTarget::RightLogs)
        {
            const float requestedWidth = aquariumPanelResizeStartState_.logsWidth - (x - aquariumPanelResizeStartX_);
            const float requestedHeight = aquariumPanelResizeStartState_.logsHeight + (y - aquariumPanelResizeStartY_);
            environment3DMode_.ResizeRightPanel(aquarium3DPanelState_, requestedWidth, requestedHeight, static_cast<float>(width_), static_cast<float>(height_));
            return true;
        }

        return false;
    }

    void AceShellUi::endAquariumPanelResize()
    {
        aquariumPanelResizeTarget_ = AquariumPanelResizeTarget::None;
        aquariumPanelResizeStartX_ = 0.0f;
        aquariumPanelResizeStartY_ = 0.0f;
    }

    bool AceShellUi::handleAquariumPanelClick(float x, float y)
    {
        if (!aquariumControllerReady_)
        {
            return false;
        }

        if (aquariumDetailsToggleRect_.contains(x, y))
        {
            aquariumDetailsPanelVisible_ = !aquariumDetailsPanelVisible_;
            aquarium3DPanelState_.detailsVisible = aquariumDetailsPanelVisible_;
            return true;
        }

        if (aquariumLogsToggleRect_.contains(x, y))
        {
            aquariumLogsPanelVisible_ = !aquariumLogsPanelVisible_;
            aquarium3DPanelState_.logsVisible = aquariumLogsPanelVisible_;
            return true;
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

        if (aquariumDebugRect_.contains(x, y) || aquariumDetailsDebugRect_.contains(x, y))
        {
            aquariumController_.SetDebugTruthEnabled(!aquariumController_.DebugTruthEnabled());
            return true;
        }

        if (aquariumOpen3DRect_.contains(x, y))
        {
            aquarium3DModeActive_ = true;
            aquariumEmbeddedViewportVisible_ = true;
            aquariumContentScroll_.offset = 0.0f;
            aquariumEmbeddedViewportStatus_ = L"DX12 Environment Mode active";
            showToast(L"3D Environment", L"DX12 environment mode active.", D2DToastKind::Success);
            return true;
        }

        if (aquariumCameraResetRect_.contains(x, y))
        {
            aquariumEmbeddedViewportVisible_ = true;
            aquariumContentScroll_.offset = 0.0f;
            aquariumEmbeddedViewportStatus_ = L"DX12 Environment Mode active";
            showToast(L"Camera", L"Environment camera reset.", D2DToastKind::Info);
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
            aquarium3DModeActive_ = false;
            aquariumEmbeddedViewportVisible_ = false;
            aquariumPendingViewportValid_ = false;
            aquariumEmbeddedViewportSyncNeeded_ = true;
            return;
        }

        if (!aquariumControllerReady_)
        {
            aquariumControllerReady_ = aquariumController_.Initialize();
        }

        if (aquarium3DModeActive_)
        {
            renderAquariumFullScreen3DMode(ctx);
        }
        else
        {
            renderAquariumControlPanelHome(ctx);
        }
    }

    void AceShellUi::renderAquariumControlPanelHome(D2DRenderContext& ctx)
    {
        aquariumEmbeddedViewportVisible_ = false;
        aquariumPendingViewportValid_ = false;
        aquariumEmbeddedViewportSyncNeeded_ = true;

        const float modalW = std::min(1080.0f, ctx.width - 72.0f);
        const float modalH = std::min(720.0f, ctx.height - 96.0f);
        const float left = (ctx.width - modalW) * 0.5f;
        const float top = (ctx.height - modalH) * 0.5f;
        const UiRect panel = makeUiRect(left, top, left + modalW, top + modalH);
        environmentModalRect_ = panel;
        environmentModalCloseRect_ = makeUiRect(panel.right - 48.0f, panel.top + 16.0f, panel.right - 18.0f, panel.top + 46.0f);

        aquariumDetailsToggleRect_ = makeUiRect(0, 0, 0, 0);
        aquariumLogsToggleRect_ = makeUiRect(0, 0, 0, 0);
        aquariumLeftPanelRect_ = makeUiRect(0, 0, 0, 0);
        aquariumRightLogsPanelRect_ = makeUiRect(0, 0, 0, 0);
        aquariumLeftResizeHandleRect_ = makeUiRect(0, 0, 0, 0);
        aquariumRightResizeHandleRect_ = makeUiRect(0, 0, 0, 0);
        aquariumDebugRect_ = makeUiRect(0, 0, 0, 0);
        aquariumCameraResetRect_ = makeUiRect(0, 0, 0, 0);
        aquariumDetailsDebugRect_ = makeUiRect(0, 0, 0, 0);

        D2DGlassEffects::drawGlassOverlay(ctx, makeUiRect(0.0f, 0.0f, ctx.width, ctx.height), 0.42f);

        D2DGlassMaterial glass;
        glass.radius = 22.0f;
        glass.fillAlpha = 0.64f;
        glass.borderAlpha = 0.58f;
        glass.highlightAlpha = 0.20f;
        glass.glowAlpha = 0.20f;
        glass.shadowAlpha = 0.10f;
        glass.blurFallbackAlpha = 0.12f;
        glass.useCornerTicks = true;
        D2DGlassEffects::drawGlassPanel(ctx, panel, glass);
        D2DCyberEffects::drawBorderGlow(ctx, panel, 22.0f, 0.20f);

        renderAquariumMiniButton(ctx, environmentModalCloseRect_, L"X", false);

        D2DWidgetUtils::drawTextEx(
            ctx,
            L"Environment Control Panel",
            FontRole::Title,
            makeUiRect(panel.left + 28.0f, panel.top + 18.0f, panel.right - 64.0f, panel.top + 56.0f),
            ctx.brushes.text
        );

        const auto snapshot = aquariumController_.BuildSnapshot();
        const float margin = 28.0f;
        const float gap = 16.0f;
        const float contentLeft = panel.left + margin;
        const float contentRight = panel.right - margin;
        const float contentTop = panel.top + 72.0f;
        const float columnW = (contentRight - contentLeft - gap) * 0.5f;

        const UiRect runtimeCard = makeUiRect(contentLeft, contentTop, contentLeft + columnW, contentTop + 118.0f);
        renderAquariumLines(ctx, L"Runtime", {
            "scenario: " + snapshot.scenarioName,
            "planner: " + snapshot.plannerName,
            "step: " + std::to_string(snapshot.step),
            "status: " + std::string(snapshot.running ? "running" : "paused")
        }, runtimeCard, 5);

        const UiRect scenarioCard = makeUiRect(runtimeCard.right + gap, contentTop, contentRight, contentTop + 118.0f);
        renderAquariumLines(ctx, L"Scenario / Planner", {
            "Scenario: " + snapshot.scenarioName,
            "Planner:  " + snapshot.plannerName
        }, scenarioCard, 3);
        aquariumScenarioPrevRect_ = makeUiRect(scenarioCard.left + 18.0f, scenarioCard.bottom - 44.0f, scenarioCard.left + 56.0f, scenarioCard.bottom - 12.0f);
        aquariumScenarioNextRect_ = makeUiRect(aquariumScenarioPrevRect_.right + 8.0f, aquariumScenarioPrevRect_.top, aquariumScenarioPrevRect_.right + 46.0f, aquariumScenarioPrevRect_.bottom);
        aquariumPlannerPrevRect_ = makeUiRect(scenarioCard.left + 160.0f, aquariumScenarioPrevRect_.top, scenarioCard.left + 198.0f, aquariumScenarioPrevRect_.bottom);
        aquariumPlannerNextRect_ = makeUiRect(aquariumPlannerPrevRect_.right + 8.0f, aquariumScenarioPrevRect_.top, aquariumPlannerPrevRect_.right + 46.0f, aquariumScenarioPrevRect_.bottom);
        renderAquariumButton(ctx, aquariumScenarioPrevRect_, L"<", false);
        renderAquariumButton(ctx, aquariumScenarioNextRect_, L">", false);
        renderAquariumButton(ctx, aquariumPlannerPrevRect_, L"<", false);
        renderAquariumButton(ctx, aquariumPlannerNextRect_, L">", false);

        const UiRect mainActions = makeUiRect(contentLeft, runtimeCard.bottom + gap, contentRight, runtimeCard.bottom + gap + 86.0f);
        renderAquariumLines(ctx, L"Main Actions", {}, mainActions, 0);
        aquariumOpen3DRect_ = makeUiRect(mainActions.left + 18.0f, mainActions.top + 38.0f, mainActions.left + 284.0f, mainActions.bottom - 14.0f);
        aquariumResetRect_ = makeUiRect(aquariumOpen3DRect_.right + 16.0f, aquariumOpen3DRect_.top, aquariumOpen3DRect_.right + 106.0f, aquariumOpen3DRect_.bottom);
        aquariumStepRect_ = makeUiRect(aquariumResetRect_.right + 10.0f, aquariumOpen3DRect_.top, aquariumResetRect_.right + 90.0f, aquariumOpen3DRect_.bottom);
        aquariumRunPauseRect_ = makeUiRect(aquariumStepRect_.right + 10.0f, aquariumOpen3DRect_.top, aquariumStepRect_.right + 112.0f, aquariumOpen3DRect_.bottom);
        renderAquariumButton(ctx, aquariumOpen3DRect_, L"Enter 3D Environment >", true);
        renderAquariumButton(ctx, aquariumResetRect_, L"Reset", false);
        renderAquariumButton(ctx, aquariumStepRect_, L"Step", false);
        renderAquariumButton(ctx, aquariumRunPauseRect_, snapshot.running ? L"Pause" : L"Run", snapshot.running);

        const UiRect manual = makeUiRect(contentLeft, mainActions.bottom + gap, contentRight, mainActions.bottom + gap + 86.0f);
        renderAquariumLines(ctx, L"Manual Actions", {}, manual, 0);
        float bx = manual.left + 18.0f;
        const float by = manual.top + 38.0f;
        const float bh = manual.bottom - 14.0f;
        aquariumManualForwardRect_ = makeUiRect(bx, by, bx + 94.0f, bh); bx += 104.0f;
        aquariumManualLeftRect_ = makeUiRect(bx, by, bx + 78.0f, bh); bx += 88.0f;
        aquariumManualRightRect_ = makeUiRect(bx, by, bx + 82.0f, bh); bx += 92.0f;
        aquariumManualWaitRect_ = makeUiRect(bx, by, bx + 68.0f, bh); bx += 78.0f;
        aquariumManualTouchRect_ = makeUiRect(bx, by, bx + 78.0f, bh); bx += 88.0f;
        aquariumManualConsumeRect_ = makeUiRect(bx, by, bx + 96.0f, bh); bx += 106.0f;
        aquariumManualPushRect_ = makeUiRect(bx, by, bx + 70.0f, bh);
        renderAquariumButton(ctx, aquariumManualForwardRect_, L"Forward", false);
        renderAquariumButton(ctx, aquariumManualLeftRect_, L"Turn L", false);
        renderAquariumButton(ctx, aquariumManualRightRect_, L"Turn R", false);
        renderAquariumButton(ctx, aquariumManualWaitRect_, L"Wait", false);
        renderAquariumButton(ctx, aquariumManualTouchRect_, L"Touch", false);
        renderAquariumButton(ctx, aquariumManualConsumeRect_, L"Consume", false);
        renderAquariumButton(ctx, aquariumManualPushRect_, L"Push", false);

        const UiRect logs = makeUiRect(contentLeft, manual.bottom + gap, contentRight, panel.bottom - margin);
        renderAquariumScrollableLines(ctx, L"Logs / Episodes", snapshot.logLines, logs, aquariumLogScroll_, 64);
    }

    void AceShellUi::renderAquariumFullScreen3DMode(D2DRenderContext& ctx)
    {
        const auto snapshot = aquariumController_.BuildSnapshot();
        aquarium3DPanelState_.detailsVisible = aquariumDetailsPanelVisible_;
        aquarium3DPanelState_.logsVisible = aquariumLogsPanelVisible_;
        environment3DMode_.ClampPanelState(aquarium3DPanelState_, ctx.width, ctx.height);
        const auto layout = environment3DMode_.Compute(ctx.width, ctx.height, aquarium3DPanelState_);
        environment3DMode_.MarkRepaint();
        ++aquariumUiLayoutPassCount_;
        ++aquariumUiRepaintCount_;

        auto toUiRect = [](const ace::aquarium_ui::AceEnvironment3DModeRect& r)
        {
            return am::ui::makeUiRect(r.left, r.top, r.right, r.bottom);
        };

        const UiRect mode = toUiRect(layout.mode);
        const UiRect topbar = toUiRect(layout.topOverlay);
        const UiRect viewportSurface = toUiRect(layout.dx12Surface);
        const UiRect viewportBackground = toUiRect(layout.viewport);
        environmentModalRect_ = mode;
        environmentModalCloseRect_ = toUiRect(layout.closeButton);
        aquariumDetailsToggleRect_ = toUiRect(layout.detailsToggle);
        aquariumLogsToggleRect_ = toUiRect(layout.logsToggle);
        aquariumDebugRect_ = toUiRect(layout.topbarDebugTruth);
        aquariumLeftPanelRect_ = toUiRect(layout.leftPanel);
        aquariumRightLogsPanelRect_ = toUiRect(layout.rightLogsPanel);
        aquariumLeftResizeHandleRect_ = toUiRect(layout.leftResizeHandle);
        aquariumRightResizeHandleRect_ = toUiRect(layout.rightResizeHandle);
        aquariumLogsFocusRect_ = aquariumRightLogsPanelRect_;
        aquariumLastViewportRect_ = viewportSurface;
        aquariumLastLeftPanelRect_ = aquariumLeftPanelRect_;
        aquariumLastRightPanelRect_ = aquariumRightLogsPanelRect_;
        aquariumOpen3DRect_ = makeUiRect(0, 0, 0, 0);
        aquariumEmbeddedViewportVisible_ = true;
        aquariumContentScroll_.viewport = viewportSurface;

        // ACE-AQ3D6: deterministic 3D mode layout pass. Topbar, left panel,
        // right logs panel, content clips and resize handles are all computed
        // from AceEnvironment3DModeLayout before anything is painted. Hover/click
        // visual state does not structurally rebuild this layout.
        D2DWidgetUtils::fillRect(ctx, mode, ctx.brushes.panelDeep);
        D2DGlassEffects::drawGlassOverlay(ctx, mode, 0.10f);

        // The viewport remains the dominant background. The hosted DX12 child
        // surface is kept stable in dx12Surface so panels are not covered by
        // child-window z-order and the surface is not recreated during idle.
        D2DWidgetUtils::fillRounded(ctx, viewportBackground, 0.0f, ctx.brushes.panelDeep);
        renderAquariumDx12ViewportSurface(ctx, viewportSurface, snapshot.debugTruthEnabled);
        aquariumTelemetryWidgets_.RenderOverlay(ctx, viewportSurface, snapshot);

        D2DGlassMaterial topGlass;
        topGlass.radius = 0.0f;
        topGlass.fillAlpha = 0.52f;
        topGlass.borderAlpha = 0.34f;
        topGlass.highlightAlpha = 0.14f;
        topGlass.glowAlpha = 0.08f;
        topGlass.shadowAlpha = 0.04f;
        topGlass.blurFallbackAlpha = 0.10f;
        topGlass.useCornerTicks = false;
        D2DGlassEffects::drawGlassPanel(ctx, topbar, topGlass);

        // ACE-AQ3D6: TopbarLayout left/center/right clusters exist and do not overlap.
        renderAquariumMiniButton(ctx, aquariumDetailsToggleRect_, aquariumDetailsPanelVisible_ ? L"Hide Details" : L"Details", aquariumDetailsPanelVisible_);
        renderAquariumMiniButton(ctx, aquariumLogsToggleRect_, aquariumLogsPanelVisible_ ? L"Hide Logs" : L"Logs", aquariumLogsPanelVisible_);
        renderAquariumMiniButton(ctx, aquariumDebugRect_, snapshot.debugTruthEnabled ? L"Truth ON" : L"Truth", snapshot.debugTruthEnabled);
        renderAquariumMiniButton(ctx, environmentModalCloseRect_, L"X", false);

        const UiRect statusRect = toUiRect(layout.topbarStatusClip);
        if (!statusRect.empty())
        {
            if (ctx.target)
            {
                ctx.target->PushAxisAlignedClip(statusRect.d2d(), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
            }
            D2DWidgetUtils::drawTextEx(
                ctx,
                widen(snapshot.scenarioName + " | " + snapshot.plannerName + " | step=" + std::to_string(snapshot.step)),
                FontRole::Small,
                statusRect,
                ctx.brushes.muted,
                DWRITE_TEXT_ALIGNMENT_CENTER,
                DWRITE_PARAGRAPH_ALIGNMENT_CENTER
            );
            if (ctx.target)
            {
                ctx.target->PopAxisAlignedClip();
            }
        }

        if (snapshot.debugTruthEnabled)
        {
            const UiRect warningRect = toUiRect(layout.topbarWarningClip);
            if (!warningRect.empty())
            {
                if (ctx.target)
                {
                    ctx.target->PushAxisAlignedClip(warningRect.d2d(), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
                }
                D2DWidgetUtils::drawTextEx(
                    ctx,
                    L"DEBUG TRUTH - NOT AGENT INPUT",
                    FontRole::Small,
                    warningRect,
                    ctx.brushes.accentWarm,
                    DWRITE_TEXT_ALIGNMENT_TRAILING,
                    DWRITE_PARAGRAPH_ALIGNMENT_CENTER
                );
                if (ctx.target)
                {
                    ctx.target->PopAxisAlignedClip();
                }
            }
        }

        auto renderPanelShell = [&](UiRect rect, const std::wstring& title)
        {
            if (rect.empty())
            {
                return;
            }

            D2DGlassMaterial panelGlass;
            panelGlass.radius = 14.0f;
            panelGlass.fillAlpha = 0.48f;
            panelGlass.borderAlpha = 0.56f;
            panelGlass.highlightAlpha = 0.18f;
            panelGlass.glowAlpha = 0.16f;
            panelGlass.shadowAlpha = 0.08f;
            panelGlass.blurFallbackAlpha = 0.14f;
            panelGlass.useCornerTicks = false;
            D2DGlassEffects::drawGlassPanel(ctx, rect, panelGlass);
            D2DCyberEffects::drawBorderGlow(ctx, rect, 14.0f, 0.14f);
            D2DWidgetUtils::drawTextEx(ctx, title, FontRole::BodyStrong, makeUiRect(rect.left + 14.0f, rect.top + 8.0f, rect.right - 14.0f, rect.top + 34.0f), ctx.brushes.text);
        };

        auto clearLeftControlRects = [&]()
        {
            aquariumScenarioPrevRect_ = makeUiRect(0, 0, 0, 0);
            aquariumScenarioNextRect_ = makeUiRect(0, 0, 0, 0);
            aquariumPlannerPrevRect_ = makeUiRect(0, 0, 0, 0);
            aquariumPlannerNextRect_ = makeUiRect(0, 0, 0, 0);
            aquariumResetRect_ = makeUiRect(0, 0, 0, 0);
            aquariumStepRect_ = makeUiRect(0, 0, 0, 0);
            aquariumRunPauseRect_ = makeUiRect(0, 0, 0, 0);
            aquariumCameraResetRect_ = makeUiRect(0, 0, 0, 0);
            aquariumDetailsDebugRect_ = makeUiRect(0, 0, 0, 0);
            aquariumManualForwardRect_ = makeUiRect(0, 0, 0, 0);
            aquariumManualLeftRect_ = makeUiRect(0, 0, 0, 0);
            aquariumManualRightRect_ = makeUiRect(0, 0, 0, 0);
            aquariumManualWaitRect_ = makeUiRect(0, 0, 0, 0);
            aquariumManualTouchRect_ = makeUiRect(0, 0, 0, 0);
            aquariumManualConsumeRect_ = makeUiRect(0, 0, 0, 0);
            aquariumManualPushRect_ = makeUiRect(0, 0, 0, 0);
        };

        if (aquariumDetailsPanelVisible_ && !aquariumLeftPanelRect_.empty())
        {
            renderPanelShell(aquariumLeftPanelRect_, L"Details / Controls");
            const UiRect contentClip = toUiRect(layout.leftContentClip);
            aquariumScenarioPrevRect_ = toUiRect(layout.scenarioPrev);
            aquariumScenarioNextRect_ = toUiRect(layout.scenarioNext);
            aquariumPlannerPrevRect_ = toUiRect(layout.plannerPrev);
            aquariumPlannerNextRect_ = toUiRect(layout.plannerNext);
            aquariumResetRect_ = toUiRect(layout.reset);
            aquariumStepRect_ = toUiRect(layout.step);
            aquariumRunPauseRect_ = toUiRect(layout.runPause);
            aquariumCameraResetRect_ = toUiRect(layout.cameraReset);
            aquariumDetailsDebugRect_ = toUiRect(layout.debugTruth);
            aquariumManualForwardRect_ = toUiRect(layout.manualForward);
            aquariumManualLeftRect_ = toUiRect(layout.manualLeft);
            aquariumManualRightRect_ = toUiRect(layout.manualRight);
            aquariumManualWaitRect_ = toUiRect(layout.manualWait);
            aquariumManualTouchRect_ = toUiRect(layout.manualTouch);
            aquariumManualConsumeRect_ = toUiRect(layout.manualConsume);
            aquariumManualPushRect_ = toUiRect(layout.manualPush);

            // ACE-AQ3D6: panel_content_clipped_to_content_rect. Sections may be
            // clipped, but they never draw into the resize handle or into each other.
            if (ctx.target && !contentClip.empty())
            {
                ctx.target->PushAxisAlignedClip(contentClip.d2d(), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
            }

            renderAquariumLines(ctx, L"Runtime", {
                "scenario: " + snapshot.scenarioName,
                "planner: " + snapshot.plannerName,
                "step: " + std::to_string(snapshot.step),
                "state: " + std::string(snapshot.running ? "running" : "paused")
            }, toUiRect(layout.leftRuntimeSection), 5);

            const UiRect scenarioSection = toUiRect(layout.leftScenarioPlannerSection);
            renderAquariumLines(ctx, L"Scenario / Planner", {}, scenarioSection, 0);
            renderAquariumMiniButton(ctx, aquariumScenarioPrevRect_, L"<", false);
            renderAquariumMiniButton(ctx, aquariumScenarioNextRect_, L">", false);
            renderAquariumMiniButton(ctx, aquariumPlannerPrevRect_, L"<", false);
            renderAquariumMiniButton(ctx, aquariumPlannerNextRect_, L">", false);

            renderAquariumLines(ctx, L"Main Controls", {}, toUiRect(layout.leftMainControlsSection), 0);
            renderAquariumMiniButton(ctx, aquariumResetRect_, L"Reset", false);
            renderAquariumMiniButton(ctx, aquariumStepRect_, L"Step", false);
            renderAquariumMiniButton(ctx, aquariumRunPauseRect_, snapshot.running ? L"Pause" : L"Run", snapshot.running);
            renderAquariumMiniButton(ctx, aquariumCameraResetRect_, L"Camera Reset", false);
            renderAquariumMiniButton(ctx, aquariumDetailsDebugRect_, L"Debug Truth", snapshot.debugTruthEnabled);

            // ACE-AQ3D6: Manual Actions grid is deterministic and non-overlapping.
            renderAquariumLines(ctx, L"Manual Actions", {}, toUiRect(layout.leftManualActionsSection), 0);
            renderAquariumMiniButton(ctx, aquariumManualForwardRect_, L"Forward", false);
            renderAquariumMiniButton(ctx, aquariumManualLeftRect_, L"Turn L", false);
            renderAquariumMiniButton(ctx, aquariumManualRightRect_, L"Turn R", false);
            renderAquariumMiniButton(ctx, aquariumManualWaitRect_, L"Wait", false);
            renderAquariumMiniButton(ctx, aquariumManualTouchRect_, L"Touch", false);
            renderAquariumMiniButton(ctx, aquariumManualConsumeRect_, L"Consume", false);
            renderAquariumMiniButton(ctx, aquariumManualPushRect_, L"Push", false);

            renderAquariumLines(ctx, L"Inspector", {
                "Body / Agent",
                "Observation",
                "Planner Trace",
                "Counterfactual",
                "Self Model",
                "Delayed / Dynamic",
                "Metrics"
            }, toUiRect(layout.leftInspectorSection), 9);

            if (ctx.target && !contentClip.empty())
            {
                ctx.target->PopAxisAlignedClip();
            }

            renderAquariumResizeHandle(ctx, aquariumLeftResizeHandleRect_, true);
        }
        else
        {
            clearLeftControlRects();
        }

        if (aquariumLogsPanelVisible_ && !aquariumRightLogsPanelRect_.empty())
        {
            renderPanelShell(aquariumRightLogsPanelRect_, L"Logs / Episodes");
            const UiRect logsBody = toUiRect(layout.rightLogsContent);
            if (ctx.target && !logsBody.empty())
            {
                ctx.target->PushAxisAlignedClip(logsBody.d2d(), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
            }
            renderAquariumScrollableLines(ctx, L"Logs", snapshot.logLines, logsBody, aquarium3DLogsScroll_, 128);
            if (ctx.target && !logsBody.empty())
            {
                ctx.target->PopAxisAlignedClip();
            }
            renderAquariumResizeHandle(ctx, aquariumRightResizeHandleRect_, false);
        }
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
        snapshot.rows.push_back({L"AQ3D7 parent paints", std::to_wstring(parentPaintCount_)});
        snapshot.rows.push_back({L"AQ3D7 full invalidations", std::to_wstring(fullInvalidationCount_)});
        snapshot.rows.push_back({L"AQ3D7 partial invalidations", std::to_wstring(partialInvalidationCount_)});
        snapshot.rows.push_back({L"AQ3D7 hover invalidations", std::to_wstring(hoverInvalidationCount_)});
        snapshot.rows.push_back({L"AQ3D7 child sync", std::to_wstring(childSyncCount_)});
        snapshot.rows.push_back({L"AQ3D7 viewport init", std::to_wstring(aquariumEmbeddedDx12Viewport_.InitCount())});
        snapshot.rows.push_back({L"AQ3D7 viewport resize", std::to_wstring(aquariumEmbeddedDx12Viewport_.ResizeCount())});
        snapshot.rows.push_back({L"AQ3D7 renderer recreate", std::to_wstring(aquariumEmbeddedDx12Viewport_.RendererRecreateCount())});
        snapshot.rows.push_back({L"AQ3D14 single-HWND composite", aquariumUseSingleHwndCompositeViewport_ ? L"enabled" : L"disabled"});
        snapshot.rows.push_back({L"AQ3D14 composite frames", std::to_wstring(aquariumCompositeFrameCount_)});
        snapshot.rows.push_back({L"AQ3D14 resize cached frames", std::to_wstring(aquariumCompositeCachedResizeFrameCount_)});
        snapshot.rows.push_back({L"AQ3D14 legacy child suppressed", std::to_wstring(aquariumLegacyChildSuppressedCount_)});
        const auto textStats = D2DTextLayoutFoundation::Stats();
        const auto drawStats = uiDrawCommands_.Stats();
        const auto invalidationStats = uiInvalidation_.Snapshot();
        const auto retainedStats = uiRetainedLayout_.Stats();
        const auto styleStats = uiStyleSet_.Stats();
        const auto telemetryStats = aquariumTelemetryWidgets_.Stats();
        snapshot.rows.push_back({L"ACE-UI5 text draws", std::to_wstring(textStats.drawCount) + L" ellipsis=" + std::to_wstring(textStats.ellipsisCount)});
        snapshot.rows.push_back({L"ACE-UI6 retained nodes", std::to_wstring(retainedStats.nodeCount) + L" passes=" + std::to_wstring(retainedStats.arrangePasses)});
        snapshot.rows.push_back({L"ACE-UI7 draw commands", std::to_wstring(drawStats.commandCount) + L" max=" + std::to_wstring(drawStats.maxCommandCount)});
        snapshot.rows.push_back({L"ACE-UI8 dirty marks", std::to_wstring(invalidationStats.markCount) + L" rects=" + std::to_wstring(invalidationStats.rects.size())});
        snapshot.rows.push_back({L"ACE-UI9 overlay", uiDebugOverlay_.Visible() ? L"visible" : L"hidden"});
        snapshot.rows.push_back({L"ACE-UI11 styles", std::to_wstring(styleStats.panelStyleCount) + L" panels / " + std::to_wstring(styleStats.textStyleCount) + L" text"});
        snapshot.rows.push_back({L"ACE-AQUI1 telemetry", std::to_wstring(telemetryStats.renderCount) + L" renders"});

        snapshot.notes.push_back(L"F12 toggles this overlay.");
        snapshot.notes.push_back(L"Click a message bubble to inspect local message metadata.");
        snapshot.notes.push_back(L"Click workspace items to inspect backend records.");
        snapshot.notes.push_back(L"Toolbar actions are command-palette shortcuts with fewer human ceremonies.");

        diagnostics_.setSnapshot(std::move(snapshot));
    }


    void AceShellUi::refreshUiDebugOverlay()
    {
        D2DUiDebugOverlaySnapshot snapshot;
        snapshot.visible = uiDebugOverlay_.Visible();
        snapshot.title = L"Arhqen UI Debug / Slate-inspired";
        snapshot.layoutStats = uiRetainedLayout_.Stats();
        snapshot.dirty = uiInvalidation_.Snapshot();
        snapshot.drawStats = uiDrawCommands_.Stats();
        snapshot.textStats = D2DTextLayoutFoundation::Stats();
        snapshot.paintCount = parentPaintCount_;
        snapshot.repaintCount = aquariumUiRepaintCount_;
        snapshot.hoverId = aquariumHoverHotId_ >= 0 ? L"aquarium-hot-" + std::to_wstring(aquariumHoverHotId_) : L"shell";
        snapshot.focusId = focus_.name();
        snapshot.rects.push_back({L"app.topbar", appTopBarRect_, false});
        snapshot.rects.push_back({L"sidebar", sidebarRect_, false});
        snapshot.rects.push_back({L"main", mainRect_, false});
        if (environmentOpen_)
        {
            snapshot.rects.push_back({L"environment", environmentModalRect_, true});
            snapshot.rects.push_back({L"aq.viewport", aquariumEmbeddedViewportRect_, true});
            snapshot.rects.push_back({L"aq.left", aquariumLeftPanelRect_, false});
            snapshot.rects.push_back({L"aq.logs", aquariumRightLogsPanelRect_, false});
        }
        uiDebugOverlay_.SetSnapshot(std::move(snapshot));
    }

    void AceShellUi::renderUiDebugOverlay(D2DRenderContext& ctx)
    {
        if (!uiDebugOverlay_.Visible())
        {
            return;
        }

        refreshUiDebugOverlay();
        uiDebugOverlay_.Render(ctx);
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
        else if (commandId == L"ui_debug")
        {
            uiDebugOverlay_.Toggle();
            ++aceUi9DebugOverlayToggleCount_;
            refreshUiDebugOverlay();
            showToast(L"UI Debug", uiDebugOverlay_.Visible() ? L"UI debug overlay opened." : L"UI debug overlay closed.", D2DToastKind::Info);
            statusBar_.setText(uiDebugOverlay_.Visible() ? L"UI debug overlay opened." : L"UI debug overlay closed.");
        }
        else if (commandId == L"ui_stats")
        {
            const auto textStats = D2DTextLayoutFoundation::Stats();
            const auto drawStats = uiDrawCommands_.Stats();
            const auto invalidationStats = uiInvalidation_.Snapshot();
            const auto retainedStats = uiRetainedLayout_.Stats();
            const auto styleStats = uiStyleSet_.Stats();
            const auto telemetryStats = aquariumTelemetryWidgets_.Stats();
            std::wstring stats =
                L"UI5 text: draw=" + std::to_wstring(textStats.drawCount) +
                L" ellipsis=" + std::to_wstring(textStats.ellipsisCount) +
                L" | UI6 retained: nodes=" + std::to_wstring(retainedStats.nodeCount) +
                L" passes=" + std::to_wstring(retainedStats.arrangePasses) +
                L" | UI7 draw: commands=" + std::to_wstring(drawStats.commandCount) +
                L" max=" + std::to_wstring(drawStats.maxCommandCount) +
                L" | UI8 dirty: marks=" + std::to_wstring(invalidationStats.markCount) +
                L" rects=" + std::to_wstring(invalidationStats.rects.size()) +
                L" | UI11 styles: panels=" + std::to_wstring(styleStats.panelStyleCount) +
                L" text=" + std::to_wstring(styleStats.textStyleCount) +
                L" | AQUI1 telemetry=" + std::to_wstring(telemetryStats.renderCount);
            messageList_.addMessage(makeMessage(L"Tool", stats, false, false, ChatMessageKind::Tool, L"ui stats"));
            statusBar_.setText(L"Command executed: ui_stats");
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

            const auto effectStats = D2DCachedEffects::stats();
            stats += L" | effect cache: entries=" + std::to_wstring(effectStats.cachedEntryCount) +
                L" hits=" + std::to_wstring(effectStats.hitCount) +
                L" misses=" + std::to_wstring(effectStats.missCount) +
                L" | display: dpi=" + std::to_wstring(displayMetrics_.nearestMonitor.dpiX) +
                L" scale=" + std::to_wstring(static_cast<int>(dpiScale() * 100.0f)) + L"%" +
                L" | d2d pixel-dpi fix=" + std::to_wstring(d2dPixelDpiFixApplyCount_);

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
        if (width <= 0 || height <= 0)
        {
            return;
        }

        refreshDisplayMetrics("resize");

        if (windowLiveResizeActive_)
        {
            // ACE-AQ3D9: live resize still updates the D2D parent target/layout so
            // the window paints at the current client size, but the embedded DX12
            // child HWND remains hidden/frozen until WM_EXITSIZEMOVE.
            pendingLiveResizeWidth_ = width;
            pendingLiveResizeHeight_ = height;
            pendingResizeAfterLiveDrag_ = true;
            aquariumEmbeddedViewportSyncNeeded_ = true;
            ++liveResizeDeferredSizeCount_;
            layoutForLiveResize(width, height);
            return;
        }

        layout(width, height);
    }

    void AceShellUi::layoutForLiveResize(int width, int height)
    {
        if (width <= 0 || height <= 0)
        {
            return;
        }

        ++d2dResizeDuringLiveResizeCount_;
        gradientsDirty_ = true;
        layout(width, height);

        if (environmentOpen_ && aquarium3DModeActive_)
        {
            aquarium3DPanelState_.detailsVisible = aquariumDetailsPanelVisible_;
            aquarium3DPanelState_.logsVisible = aquariumLogsPanelVisible_;
            environment3DMode_.ClampPanelState(aquarium3DPanelState_, static_cast<float>(width), static_cast<float>(height));
            const auto liveLayout = environment3DMode_.Compute(static_cast<float>(width), static_cast<float>(height), aquarium3DPanelState_);
            aquariumEmbeddedViewportRect_ = makeUiRect(liveLayout.dx12Surface.left, liveLayout.dx12Surface.top, liveLayout.dx12Surface.right, liveLayout.dx12Surface.bottom);
            aquariumPendingViewportRect_ = aquariumEmbeddedViewportRect_;
            aquariumPendingViewportValid_ = true;
            if (!aquariumUseSingleHwndCompositeViewport_)
            {
                updateAquariumResizeShieldWindow(aquariumEmbeddedViewportRect_);
            }
        }

        forceLiveResizeProxyRepaintNow();
    }

    void AceShellUi::beginWindowLiveResize()
    {
        if (windowLiveResizeActive_)
        {
            return;
        }

        windowLiveResizeActive_ = true;
        pendingResizeAfterLiveDrag_ = false;
        pendingLiveResizeWidth_ = width_;
        pendingLiveResizeHeight_ = height_;
        gradientsDirty_ = true;
        aquariumEmbeddedViewportSyncNeeded_ = true;
        captureFrozenNativeResizeRect();
        aquariumEmbeddedDx12Viewport_.SetResizeApplySuspended(true);

        aquariumViewportWasVisibleBeforeLiveResize_ = environmentOpen_ && aquarium3DModeActive_ && aquariumEmbeddedDx12Viewport_.IsVisible();
        aquariumViewportHiddenForLiveResize_ = false;
        if (aquariumViewportWasVisibleBeforeLiveResize_)
        {
            aquariumEmbeddedDx12Viewport_.HideForLiveResize();
            aquariumViewportHiddenForLiveResize_ = true;
            ++viewportHideForLiveResizeCount_;
        }

        if (environmentOpen_ && aquarium3DModeActive_ && !aquariumUseSingleHwndCompositeViewport_)
        {
            updateAquariumResizeShieldWindow(aquariumEmbeddedViewportRect_);
        }
        else if (aquariumUseSingleHwndCompositeViewport_)
        {
            hideAquariumResizeShieldWindow();
        }

        ++liveResizeEnterCount_;
    }

    bool AceShellUi::shouldFreezeNativeLiveResize() const
    {
        // ACE-AQ3D14: native resize freezing is disabled for the single-HWND
        // composite viewport path. It remains only as a legacy emergency path
        // for the old child-HWND swapchain renderer.
        return !aquariumUseSingleHwndCompositeViewport_ && environmentOpen_ && aquarium3DModeActive_;
    }

    void AceShellUi::captureFrozenNativeResizeRect()
    {
        if (!parent_ || !shouldFreezeNativeLiveResize())
        {
            return;
        }

        RECT rect{};
        if (!GetWindowRect(parent_, &rect))
        {
            return;
        }

        frozenNativeLiveResizeRect_ = rect;
        frozenNativeLiveResizeRectValid_ = true;
        frozenNativeLiveResizeActive_ = true;
        pendingNativeLiveResizeRectValid_ = false;
        ++nativeResizeFreezeEnterCount_;
    }

    bool AceShellUi::handleFrozenNativeResizeSizing(LPARAM lParam)
    {
        if (!parent_ || !shouldFreezeNativeLiveResize() || !lParam)
        {
            return false;
        }

        if (!frozenNativeLiveResizeRectValid_)
        {
            captureFrozenNativeResizeRect();
        }

        if (!frozenNativeLiveResizeRectValid_)
        {
            return false;
        }

        RECT* proposed = reinterpret_cast<RECT*>(lParam);
        pendingNativeLiveResizeRect_ = *proposed;
        pendingNativeLiveResizeRectValid_ = true;
        ++nativeResizeFrozenSizingMessageCount_;

        // Do not let USER32 continuously change the actual top-level client area
        // while the DX12 child swapchain exists. The proposed RECT is committed
        // once in applyFrozenNativeResizeCommit().
        *proposed = frozenNativeLiveResizeRect_;
        ++nativeResizePreventedClientResizeCount_;

        pendingResizeAfterLiveDrag_ = true;
        aquariumEmbeddedViewportSyncNeeded_ = true;
        updateAquariumResizeShieldWindow(aquariumEmbeddedViewportRect_);
        return true;
    }

    void AceShellUi::applyFrozenNativeResizeCommit()
    {
        if (!parent_ || !frozenNativeLiveResizeActive_)
        {
            frozenNativeLiveResizeRectValid_ = false;
            pendingNativeLiveResizeRectValid_ = false;
            return;
        }

        frozenNativeLiveResizeActive_ = false;
        frozenNativeLiveResizeRectValid_ = false;

        if (!pendingNativeLiveResizeRectValid_)
        {
            return;
        }

        const RECT finalRect = pendingNativeLiveResizeRect_;
        pendingNativeLiveResizeRectValid_ = false;

        const int finalWidth = std::max<int>(1, static_cast<int>(finalRect.right - finalRect.left));
        const int finalHeight = std::max<int>(1, static_cast<int>(finalRect.bottom - finalRect.top));
        ++nativeResizeFinalCommitCount_;

        SetWindowPos(
            parent_,
            nullptr,
            finalRect.left,
            finalRect.top,
            finalWidth,
            finalHeight,
            SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER
        );

        RECT client{};
        if (GetClientRect(parent_, &client))
        {
            width_ = std::max<int>(1, static_cast<int>(client.right - client.left));
            height_ = std::max<int>(1, static_cast<int>(client.bottom - client.top));
            pendingLiveResizeWidth_ = width_;
            pendingLiveResizeHeight_ = height_;
            pendingResizeAfterLiveDrag_ = true;
        }
    }

    void AceShellUi::endWindowLiveResize()
    {
        if (!windowLiveResizeActive_)
        {
            return;
        }

        windowLiveResizeActive_ = false;
        ++liveResizeExitCount_;

        aquariumEmbeddedDx12Viewport_.SetResizeApplySuspended(false);
        applyFrozenNativeResizeCommit();
        applyDeferredLiveResize();
    }

    void AceShellUi::applyDeferredLiveResize()
    {
        if (!pendingResizeAfterLiveDrag_)
        {
            // Still force a final paint/sync point so maximize/restore and native
            // resize edge cases leave the viewport in the correct state. If the
            // child HWND was hidden for resize, keep the D2D proxy visible until
            // syncAquariumEmbeddedViewportWindow restores the child at the final rect.
            aquariumEmbeddedViewportSyncNeeded_ = true;
            invalidate();
            forceLiveResizeProxyRepaintNow();
            restoreEmbeddedViewportAfterLiveResize();
            return;
        }

        const int finalWidth = std::max(1, pendingLiveResizeWidth_);
        const int finalHeight = std::max(1, pendingLiveResizeHeight_);

        pendingResizeAfterLiveDrag_ = false;
        gradientsDirty_ = true;
        layout(finalWidth, finalHeight);
        aquariumEmbeddedViewportSyncNeeded_ = true;
        ++liveResizeAppliedFinalCount_;
        invalidate();
        forceLiveResizeProxyRepaintNow();
        restoreEmbeddedViewportAfterLiveResize();
    }

    void AceShellUi::forceLiveResizeProxyRepaintNow()
    {
        if (!parent_)
        {
            return;
        }

        ++liveResizeSynchronousProxyPaintCount_;
        if (windowLiveResizeActive_ && environmentOpen_ && aquarium3DModeActive_ && !aquariumUseSingleHwndCompositeViewport_)
        {
            updateAquariumResizeShieldWindow(aquariumEmbeddedViewportRect_);
        }
        // ACE-AQ3D10: live resize must not wait for a later idle tick/paint.
        // Force a no-erase synchronous parent repaint so the D2D proxy fills the
        // viewport rectangle immediately after each WM_SIZE, before the desktop
        // compositor can expose stale child/DX12 contents.
        RedrawWindow(parent_, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOERASE | RDW_NOCHILDREN);
    }

    void AceShellUi::restoreEmbeddedViewportAfterLiveResize()
    {
        if (!environmentOpen_ || !aquarium3DModeActive_)
        {
            return;
        }

        if (aquariumUseSingleHwndCompositeViewport_)
        {
            hideAquariumResizeShieldWindow();
            aquariumEmbeddedViewportSyncNeeded_ = false;
            aquariumEmbeddedViewportVisible_ = false;
            if (aquariumEmbeddedDx12Viewport_.IsVisible())
            {
                aquariumEmbeddedDx12Viewport_.Hide();
                ++aquariumLegacyChildSuppressedCount_;
            }
            invalidateRect(aquariumEmbeddedViewportRect_.empty() ? mainRect_ : aquariumEmbeddedViewportRect_);
            return;
        }

        syncAquariumEmbeddedViewportWindow();

        if (aquariumControllerReady_ && aquariumEmbeddedViewportVisible_ && aquariumEmbeddedDx12Viewport_.IsVisible())
        {
            std::string viewportError;
            if (!aquariumEmbeddedDx12Viewport_.RenderFrame(aquariumController_, aquariumSceneAdapter_, aquariumController_.DebugTruthEnabled(), &viewportError))
            {
                aquariumEmbeddedViewportStatus_ = L"DX12 Environment renderer unavailable; check logs.";
            }
        }

        hideAquariumResizeShieldWindow();
    }

    bool AceShellUi::ensureAquariumResizeShieldWindow()
    {
        if (!parent_)
        {
            return false;
        }

        constexpr wchar_t kShieldClassName[] = L"ArhqenCognitionEngineAquariumResizeShieldPopup";
        HINSTANCE instance = GetModuleHandleW(nullptr);

        if (!aquariumResizeShieldClassRegistered_)
        {
            WNDCLASSEXW wc{};
            wc.cbSize = sizeof(wc);
            wc.style = 0;
            wc.lpfnWndProc = &AceShellUi::AquariumResizeShieldWindowProc;
            wc.hInstance = instance;
            wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
            wc.hbrBackground = nullptr;
            wc.lpszClassName = kShieldClassName;

            if (RegisterClassExW(&wc) || GetLastError() == ERROR_CLASS_ALREADY_EXISTS)
            {
                aquariumResizeShieldClassRegistered_ = true;
            }
            else
            {
                return false;
            }
        }

        if (aquariumResizeShieldHwnd_)
        {
            return true;
        }

        aquariumResizeShieldHwnd_ = CreateWindowExW(
            WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
            kShieldClassName,
            L"",
            WS_POPUP,
            -32000,
            -32000,
            1,
            1,
            parent_,
            nullptr,
            instance,
            this
        );

        if (!aquariumResizeShieldHwnd_)
        {
            return false;
        }

        ++resizeShieldCreateCount_;
        return true;
    }

    void AceShellUi::updateAquariumResizeShieldWindow(UiRect rect)
    {
        if (!windowLiveResizeActive_ || !environmentOpen_ || !aquarium3DModeActive_ || rect.empty())
        {
            return;
        }

        if (!ensureAquariumResizeShieldWindow())
        {
            return;
        }

        POINT topLeft{static_cast<LONG>(std::floor(rect.left)), static_cast<LONG>(std::floor(rect.top))};
        POINT bottomRight{static_cast<LONG>(std::ceil(rect.right)), static_cast<LONG>(std::ceil(rect.bottom))};
        ClientToScreen(parent_, &topLeft);
        ClientToScreen(parent_, &bottomRight);

        const int x = topLeft.x;
        const int y = topLeft.y;
const int w = std::max(1, static_cast<int>(bottomRight.x - topLeft.x));
const int h = std::max(1, static_cast<int>(bottomRight.y - topLeft.y));

        aquariumResizeShieldRect_ = rect;
        ++resizeShieldMoveCount_;
        SetWindowPos(
            aquariumResizeShieldHwnd_,
            HWND_TOP,
            x,
            y,
            w,
            h,
            SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_SHOWWINDOW
        );

        if (!aquariumResizeShieldVisible_)
        {
            ++resizeShieldShowCount_;
        }
        aquariumResizeShieldVisible_ = true;
        RedrawWindow(aquariumResizeShieldHwnd_, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOERASE);
    }

    void AceShellUi::hideAquariumResizeShieldWindow()
    {
        if (aquariumResizeShieldHwnd_ && aquariumResizeShieldVisible_)
        {
            ++resizeShieldHideCount_;
            ShowWindow(aquariumResizeShieldHwnd_, SW_HIDE);
        }
        aquariumResizeShieldVisible_ = false;
    }

    void AceShellUi::destroyAquariumResizeShieldWindow()
    {
        if (aquariumResizeShieldHwnd_)
        {
            DestroyWindow(aquariumResizeShieldHwnd_);
            aquariumResizeShieldHwnd_ = nullptr;
        }
        aquariumResizeShieldVisible_ = false;
    }

    LRESULT CALLBACK AceShellUi::AquariumResizeShieldWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        AceShellUi* self = nullptr;
        if (message == WM_NCCREATE)
        {
            const auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            self = static_cast<AceShellUi*>(cs ? cs->lpCreateParams : nullptr);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        else
        {
            self = reinterpret_cast<AceShellUi*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        }

        switch (message)
        {
        case WM_ERASEBKGND:
            return 1;

        case WM_PAINT:
        {
            if (self)
            {
                ++self->resizeShieldPaintCount_;
            }

            PAINTSTRUCT ps{};
            HDC dc = BeginPaint(hwnd, &ps);
            RECT rc{};
            GetClientRect(hwnd, &rc);

            HBRUSH background = CreateSolidBrush(RGB(3, 9, 18));
            FillRect(dc, &rc, background);
            DeleteObject(background);

            HPEN gridPen = CreatePen(PS_SOLID, 1, RGB(18, 60, 70));
            HPEN oldPen = static_cast<HPEN>(SelectObject(dc, gridPen));
            for (int x = 0; x < rc.right; x += 32)
            {
                MoveToEx(dc, x, 0, nullptr);
                LineTo(dc, x, rc.bottom);
            }
            for (int y = 0; y < rc.bottom; y += 32)
            {
                MoveToEx(dc, 0, y, nullptr);
                LineTo(dc, rc.right, y);
            }
            SelectObject(dc, oldPen);
            DeleteObject(gridPen);

            HBRUSH agentBrush = CreateSolidBrush(RGB(80, 230, 210));
            const int cx = (rc.right - rc.left) / 2;
            const int cy = (rc.bottom - rc.top) / 2;
            RECT agent{cx - 7, cy - 7, cx + 7, cy + 7};
            FillRect(dc, &agent, agentBrush);
            DeleteObject(agentBrush);

            SetBkMode(dc, TRANSPARENT);
            SetTextColor(dc, RGB(125, 210, 210));
            RECT textRect{18, 14, std::max<LONG>(18, rc.right - 18), 42};
            DrawTextW(dc, L"Resizing viewport...", -1, &textRect, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);

            EndPaint(hwnd, &ps);
            return 0;
        }

        default:
            return DefWindowProcW(hwnd, message, wParam, lParam);
        }
    }

    UiRect AceShellUi::inflateRect(UiRect rect, float padding)
    {
        if (rect.empty())
        {
            return rect;
        }

        return am::ui::makeUiRect(rect.left - padding, rect.top - padding, rect.right + padding, rect.bottom + padding);
    }

    void AceShellUi::invalidateRect(UiRect rect)
    {
        if (!parent_ || rect.empty())
        {
            return;
        }

        RECT r{};
        r.left = static_cast<LONG>(std::floor(rect.left));
        r.top = static_cast<LONG>(std::floor(rect.top));
        r.right = static_cast<LONG>(std::ceil(rect.right));
        r.bottom = static_cast<LONG>(std::ceil(rect.bottom));

        ++partialInvalidationCount_;
        uiInvalidation_.MarkRect(rect, AceUiDirtyReason::Paint);
        ++aceUi8InvalidationFrameCount_;
        // ACE-AQ3D7: dirty rect invalidation excludes child HWNDs and never
        // triggers background erase. Hover/press updates use this path.
        RedrawWindow(parent_, &r, nullptr, RDW_INVALIDATE | RDW_NOERASE | RDW_NOCHILDREN);
    }

    void AceShellUi::invalidateAquariumChrome()
    {
        if (!environmentOpen_)
        {
            invalidate();
            return;
        }

        // ACE-AQ3D7: repaint panels/topbar/logs, not the DX12 child viewport.
        invalidateRect(inflateRect(aquariumDetailsToggleRect_, 8.0f));
        invalidateRect(inflateRect(aquariumLogsToggleRect_, 8.0f));
        invalidateRect(inflateRect(aquariumDebugRect_, 8.0f));
        invalidateRect(inflateRect(environmentModalCloseRect_, 8.0f));
        invalidateRect(inflateRect(aquariumLeftPanelRect_, 10.0f));
        invalidateRect(inflateRect(aquariumRightLogsPanelRect_, 10.0f));
        if (!aquarium3DModeActive_)
        {
            invalidateRect(inflateRect(environmentModalRect_, 10.0f));
        }
    }

    void AceShellUi::invalidate()
    {
        if (parent_)
        {
            ++fullInvalidationCount_;
            uiInvalidation_.Mark(AceUiDirtyReason::All);
            ++aceUi8InvalidationFrameCount_;
            // ACE-AQ3D7: full invalidation is reserved for major layout/mode changes.
            // It still avoids erase and child invalidation to keep DX12 stable.
            RedrawWindow(parent_, nullptr, nullptr, RDW_INVALIDATE | RDW_NOERASE | RDW_NOCHILDREN);
        }
    }

    void AceShellUi::updateCursor()
    {
        POINT p{};
        GetCursorPos(&p);
        ScreenToClient(parent_, &p);

        const float x = static_cast<float>(p.x);
        const float y = static_cast<float>(p.y);

        if (environmentOpen_ && aquarium3DModeActive_ && aquariumLeftResizeHandleRect_.contains(x, y))
        {
            SetCursor(LoadCursorW(nullptr, IDC_SIZENWSE));
        }
        else if (environmentOpen_ && aquarium3DModeActive_ && aquariumRightResizeHandleRect_.contains(x, y))
        {
            SetCursor(LoadCursorW(nullptr, IDC_SIZENESW));
        }
        else if (commandPalette_.active())
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
