#pragma once

#if !defined(_WIN32)
#error Arhqen Cognition Engine ACE-CLEAN0 AceShellUi is Windows-only.
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "ArhqenCognitionEngine/Ui/D2D/D2DAutocompletePopup.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DButton.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DCommandPalette.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DFocusManager.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DInspectorPanel.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DTabStrip.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DKeyboard.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DLayoutProfile.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DLayoutPersistence.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DWorkspaceLayout.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DSplitter.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DCommandHistory.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DLayoutOverlay.h" 
#include "ArhqenCognitionEngine/Ui/D2D/D2DMessageList.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DSidebar.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DStatusBar.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DWorkspacePanel.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DTextInput.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DToolbar.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DToastCenter.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DShortcutHelpOverlay.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DDiagnosticsOverlay.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DMessageInspector.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DCyberBackgroundField.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DBlurRuntime.h"
#include "ArhqenCognitionEngine/AquariumUI/AceAquariumRuntimeController.h"

#include <wrl/client.h>

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace am::ui
{
    class AceShellUi
    {
        static constexpr std::size_t kNoHoveredConversation = static_cast<std::size_t>(-1);

    public:
        AceShellUi() = default;
        ~AceShellUi();

        AceShellUi(const AceShellUi&) = delete;
        AceShellUi& operator=(const AceShellUi&) = delete;

        using SubmitHandler = std::function<ChatSubmitResult(const std::wstring&)>;
        using SnapshotProvider = std::function<am::core::AceUiSnapshot()>;
        using SuggestionProvider = std::function<std::vector<am::core::AceSuggestion>(const std::wstring&)>;
        using InspectorProvider = std::function<am::core::AceUiInspectorRecord(const am::core::AceUiSelection&)>;

        void setSubmitHandler(SubmitHandler handler);
        void setSnapshotProvider(SnapshotProvider provider);
        void setSuggestionProvider(SuggestionProvider provider);
        void setInspectorProvider(InspectorProvider provider);
        void setLayoutProfilePath(std::filesystem::path path);
        bool create(HWND parent, int width, int height, std::string* error);
        void layout(int width, int height);
        LRESULT handleWindowMessage(UINT message, WPARAM wParam, LPARAM lParam, bool* handled);

        bool created() const;
        void tick(float dtSeconds);

    private:
        bool createDeviceIndependentResources(std::string* error);
        bool createDeviceResources(std::string* error);
        bool createBrushes(std::string* error);
        bool createGradients(std::string* error);
        bool createTextFormats(std::string* error);
        bool createTextFormat(const wchar_t* family, float size, DWRITE_FONT_WEIGHT weight, Microsoft::WRL::ComPtr<IDWriteTextFormat>& format, std::string* error);
        void discardDeviceResources();

        D2DRenderContext makeContext();
        void seedInitialState();
        void render();
        void renderBackground(D2DRenderContext& ctx);
        void renderHeader(D2DRenderContext& ctx);
        void renderMainPanel(D2DRenderContext& ctx);
        void renderAppTopBar(D2DRenderContext& ctx);
        void renderSideNav(D2DRenderContext& ctx);
        void renderSendCircle(D2DRenderContext& ctx);
        void renderEmptyState(D2DRenderContext& ctx);
        void renderSettingsModal(D2DRenderContext& ctx);
        void renderEnvironmentPlaceholder(D2DRenderContext& ctx);
        void renderAquariumControlPanel(D2DRenderContext& ctx);
        void renderAquariumButton(D2DRenderContext& ctx, UiRect rect, const std::wstring& label, bool active = false);
        bool isAquariumButtonHovered(UiRect rect) const;
        void renderAquariumLines(D2DRenderContext& ctx, const std::wstring& title, const std::vector<std::string>& lines, UiRect rect, std::size_t maxLines);
        struct AquariumScrollPanel
        {
            UiRect viewport{};
            UiRect track{};
            UiRect thumb{};
            float offset = 0.0f;
            float contentHeight = 0.0f;
            float viewportHeight = 0.0f;
            std::size_t lineCount = 0;
        };

        void renderAquariumScrollableLines(D2DRenderContext& ctx, const std::wstring& title, const std::vector<std::string>& lines, UiRect rect, AquariumScrollPanel& scroll, std::size_t maxVisibleLines);
        void renderAquariumScrollbar(D2DRenderContext& ctx, AquariumScrollPanel& scroll);
        void clampAquariumScroll(AquariumScrollPanel& scroll);
        bool handleAquariumWheel(float x, float y, int wheelDelta);
        bool beginAquariumScrollbarDrag(float x, float y);
        bool updateAquariumScrollbarDrag(float x, float y);
        void endAquariumScrollbarDrag();
        AquariumScrollPanel* activeAquariumScrollPanelAt(float x, float y);
        bool handleAquariumPanelClick(float x, float y);
        void cycleAquariumScenario(int direction);
        void cycleAquariumPlanner(int direction);
        std::wstring widen(const std::string& text) const;
        void startNewConversation();
        void selectConversation(std::size_t index);
        void prefillRenameConversation(std::size_t index);
        void saveActiveConversation();
        bool handleLocalInputCommand(const std::wstring& text);

        void sendCurrentInput();
        void appendAssistantMockReply();
        void appendExternalMessage(ChatMessage message);
        void appendSubmitResult(ChatSubmitResult result);
        void refreshSnapshot();
        void refreshSuggestions();
        void refreshInspector(am::core::AceUiSelection selection);
        void inspectLocalMessage(std::uint64_t messageId);
        void refreshDiagnostics();
        void showToast(std::wstring title, std::wstring body, D2DToastKind kind = D2DToastKind::Info);
        void saveLayoutProfile();
        void loadLayoutProfile();
        void resetLayoutProfile();
        void refreshLayoutOverlay();
        void clearInspector();
        void acceptAutocomplete();
        void executeCommand(const std::wstring& commandId);
        void openCommandPalette();
        void seedDemoMessages();
        void handleResize(int width, int height);
        void invalidate();
        void updateCursor();

        ChatMessage makeMessage(std::wstring author, std::wstring text, bool fromUser, bool system, ChatMessageKind kind, std::wstring metadata = {});
        std::wstring currentTimestamp() const;
        static std::string hresultToString(const char* label, HRESULT hr);

        HWND parent_ = nullptr;
        int width_ = 0;
        int height_ = 0;
        bool created_ = false;
        bool mouseCaptured_ = false;
        bool settingsOpen_ = false;
        bool environmentOpen_ = false;
        std::uint64_t nextMessageId_ = 1;
        std::uint64_t nextConversationNumber_ = 1;
        std::size_t activeConversationIndex_ = 0;
        std::vector<std::wstring> conversations_;
        std::vector<std::vector<ChatMessage>> conversationMessages_;
        std::vector<UiRect> conversationRowRects_;
        std::vector<UiRect> conversationRenameRects_;
        SubmitHandler submitHandler_;
        SnapshotProvider snapshotProvider_;
        SuggestionProvider suggestionProvider_;
        InspectorProvider inspectorProvider_;

        D2DTheme theme_{};
        D2DCyberBackgroundField cyberBackground_{};
        D2DBlurRuntimeStatus blurStatus_{};
        float cyberTextTime_ = 0.0f;
        bool settingsHovered_ = false;
        bool environmentHovered_ = false;
        float mouseX_ = 0.0f;
        float mouseY_ = 0.0f;
        ace::aquarium_ui::AceAquariumRuntimeController aquariumController_;
        bool aquariumControllerReady_ = false;
        bool aquariumLogsFocus_ = false;
        bool newConversationHovered_ = false;
        std::size_t hoveredConversationIndex_ = kNoHoveredConversation;

        UiRect headerRect_{};
        UiRect appTopBarRect_{};
        UiRect brandLogoRect_{};
        UiRect settingsButtonRect_{};
        UiRect environmentButtonRect_{};
        UiRect sidebarRect_{};
        UiRect newConversationRect_{};
        UiRect conversationListRect_{};
        UiRect mainRect_{};
        UiRect toolbarRect_{};
        UiRect conversationRect_{};
        UiRect inputRect_{};
        UiRect sendButtonRect_{};
        UiRect sendCircleRect_{};
        UiRect statusRect_{};
        UiRect workspaceRect_{};
        UiRect inspectorRect_{};
        UiRect tabRect_{};
        UiRect autocompleteRect_{};
        UiRect diagnosticsRect_{};
        UiRect toastRect_{};
        UiRect shortcutHelpRect_{};
        UiRect settingsModalRect_{};
        UiRect settingsModalCloseRect_{};
        UiRect environmentModalRect_{};
        UiRect environmentModalCloseRect_{};
        UiRect aquariumScenarioPrevRect_{};
        UiRect aquariumScenarioNextRect_{};
        UiRect aquariumPlannerPrevRect_{};
        UiRect aquariumPlannerNextRect_{};
        UiRect aquariumResetRect_{};
        UiRect aquariumStepRect_{};
        UiRect aquariumRunPauseRect_{};
        UiRect aquariumDebugRect_{};
        UiRect aquariumLogsFocusRect_{};
        UiRect aquariumManualForwardRect_{};
        UiRect aquariumManualLeftRect_{};
        UiRect aquariumManualRightRect_{};
        UiRect aquariumManualWaitRect_{};
        UiRect aquariumManualTouchRect_{};
        UiRect aquariumManualConsumeRect_{};
        UiRect aquariumManualPushRect_{};
        AquariumScrollPanel aquariumLogScroll_{};
        AquariumScrollPanel aquariumDebugScroll_{};
        AquariumScrollPanel aquariumCounterfactualScroll_{};
        AquariumScrollPanel aquariumMetricsScroll_{};
        AquariumScrollPanel aquariumContentScroll_{};
        AquariumScrollPanel* aquariumDraggingScroll_ = nullptr;
        float aquariumDragStartY_ = 0.0f;
        float aquariumDragStartOffset_ = 0.0f;
        UiRect layoutOverlayRect_{};
        UiRect rightSplitterRect_{};
        UiRect inspectorSplitterRect_{};

        D2DMessageList messageList_;
        D2DTextInput input_;
        D2DButton sendButton_;
        D2DSidebar sidebar_;
        D2DStatusBar statusBar_;
        D2DToolbar toolbar_;
        D2DDiagnosticsOverlay diagnostics_;
        D2DToastCenter toastCenter_;
        D2DShortcutHelpOverlay shortcutHelp_;
        D2DLayoutOverlay layoutOverlay_;
        D2DWorkspaceLayout workspaceLayout_;
        D2DDockLayoutProfile layoutProfile_{};
        D2DWorkspaceRects layoutRects_{};
        D2DSplitter workspaceSplitter_;
        D2DSplitter inspectorSplitter_;
        D2DCommandHistory commandHistory_;
        std::filesystem::path layoutProfilePath_;
        D2DWorkspacePanel workspacePanel_;
        D2DInspectorPanel inspectorPanel_;
        D2DTabStrip workspaceTabs_;
        D2DAutocompletePopup autocomplete_;
        D2DCommandPalette commandPalette_;
        D2DFocusManager focus_;
        DWriteFontEngine fontEngine_;
        DWriteTextCache textCache_{512};

        Microsoft::WRL::ComPtr<ID2D1Factory> d2dFactory_;
        Microsoft::WRL::ComPtr<IDWriteFactory> dwriteFactory_;
        Microsoft::WRL::ComPtr<ID2D1HwndRenderTarget> renderTarget_;

        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> textBrush_;
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> textDimBrush_;
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> mutedBrush_;
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> panelBrush_;
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> panelDeepBrush_;
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> panelSoftBrush_;
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> panelElevatedBrush_;
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> borderBrush_;
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> borderDimBrush_;
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> accentBrush_;
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> accentBlueBrush_;
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> accentWarmBrush_;
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> dangerBrush_;
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> userBubbleBrush_;
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> assistantBubbleBrush_;
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> systemBubbleBrush_;
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> inputBrush_;
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> inputFocusedBrush_;

        Microsoft::WRL::ComPtr<ID2D1LinearGradientBrush> backgroundGradientBrush_;
        Microsoft::WRL::ComPtr<ID2D1LinearGradientBrush> accentGradientBrush_;
        Microsoft::WRL::ComPtr<ID2D1LinearGradientBrush> buttonGradientBrush_;
        Microsoft::WRL::ComPtr<ID2D1LinearGradientBrush> buttonHoverGradientBrush_;

        Microsoft::WRL::ComPtr<IDWriteTextFormat> titleFormat_;
        Microsoft::WRL::ComPtr<IDWriteTextFormat> subtitleFormat_;
        Microsoft::WRL::ComPtr<IDWriteTextFormat> bodyFormat_;
        Microsoft::WRL::ComPtr<IDWriteTextFormat> bodyStrongFormat_;
        Microsoft::WRL::ComPtr<IDWriteTextFormat> smallFormat_;
        Microsoft::WRL::ComPtr<IDWriteTextFormat> monoFormat_;
        Microsoft::WRL::ComPtr<IDWriteTextFormat> buttonFormat_;
    };
}
