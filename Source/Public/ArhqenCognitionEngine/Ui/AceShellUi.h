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
#include "ArhqenCognitionEngine/Ui/D2D/D2DDisplayMetrics.h"
#include "ArhqenCognitionEngine/Ui/D2D/AceD2DFlipFrameCompositor.h"
#include "ArhqenCognitionEngine/Ui/D2D/AceD2DSharedViewportBridgePolicy.h"
#include "ArhqenCognitionEngine/Ui/D2D/AceD2DViewportBridgeRuntime.h"
#include "ArhqenCognitionEngine/Ui/D2D/AceD2DViewportCopyScheduler.h"
#include "ArhqenCognitionEngine/Ui/D2D/AceD2DViewportTextureCache.h"
#include "ArhqenCognitionEngine/Ui/D2D/AceD2DPresentScheduler.h"
#include "ArhqenCognitionEngine/Ui/D2D/AceD2DFrameDiagnostics.h"
#include "ArhqenCognitionEngine/Ui/D2D/AceD2DCompositorAudit.h"
#include "ArhqenCognitionEngine/Ui/D2D/AceD2DFrameTransaction.h"
#include "ArhqenCognitionEngine/Ui/Slate/AceSlateElementList.h"
#include "ArhqenCognitionEngine/Ui/Slate/AceSlateWindowRendererPipeline.h"
#include "ArhqenCognitionEngine/Ui/Slate/AceSlateLayerTree.h"
#include "ArhqenCognitionEngine/Ui/Slate/AceSlatePaintJournal.h"
#include "ArhqenCognitionEngine/Ui/Slate/AceSlateViewportElement.h"
#include "ArhqenCognitionEngine/Ui/Core/AceUiInvalidationRoot.h"
#include "ArhqenCognitionEngine/Ui/Core/AceUiRetainedLayout.h"
#include "ArhqenCognitionEngine/Ui/Core/AceUiStyleSet.h"
#include "ArhqenCognitionEngine/Ui/Core/AcePanelResizePolicy.h"
#include "ArhqenCognitionEngine/Editor/Workspace/AceEditorWorkspaceController.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DDrawCommandBuffer.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DUiDebugOverlay.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DAquariumTelemetryWidgets.h"
#include "ArhqenCognitionEngine/AquariumUI/AceAquariumRuntimeController.h"
#include "ArhqenCognitionEngine/AquariumUI/AceEnvironmentWorkspace.h"
#include "ArhqenCognitionEngine/AquariumUI/AceEnvironment3DMode.h"
#include "ArhqenCognitionEngine/AquariumRender/AceAquariumCamera.h"
#include "ArhqenCognitionEngine/AquariumRender/AceAquariumSceneAdapter.h"
#include "ArhqenCognitionEngine/AquariumRender/AceAquariumViewport.h"
#include "ArhqenCognitionEngine/AquariumRender/AceAquariumEmbeddedDx12Viewport.h"
#include "ArhqenCognitionEngine/Renderer/Scene/AceAquariumGpuViewportRenderer.h"
#include "ArhqenCognitionEngine/Ui/AceEngineConsole.h"

#include <wrl/client.h>
#include <d3d11.h>
#include <d2d1_1.h>
#include <d3d11on12.h>
#include <dxgi1_2.h>
#include <dxgi1_3.h>


#include <array>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
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
        void setVsyncEnabled(bool enabled);
        void setRuntimeFrameDeltaSeconds(double deltaSeconds);
        void flushPendingPaint();
        bool create(HWND parent, int width, int height, std::string* error);
        void layout(int width, int height);
        LRESULT handleWindowMessage(UINT message, WPARAM wParam, LPARAM lParam, bool* handled);

        bool created() const;
        bool wantsUnthrottledTick() const;
        void tick(float dtSeconds);

    private:
        bool createDeviceIndependentResources(std::string* error);
        bool createDeviceResources(std::string* error);
        bool createBrushes(std::string* error);
        bool createGradients(std::string* error);
        bool createTextFormats(std::string* error);
        bool createTextFormat(const wchar_t* family, float size, DWRITE_FONT_WEIGHT weight, Microsoft::WRL::ComPtr<IDWriteTextFormat>& format, std::string* error);
        void discardDeviceResources();
        void applyPixelAlignedD2DTargetDpi();
        bool createD2DDeviceContextBackbufferTarget(std::string* error);
        bool resizeD2DDeviceContextBackbufferTarget(int width, int height, std::string* error);

        D2DRenderContext makeContext();
        void seedInitialState();
        void render();
        bool renderFastAquariumViewportFrame(D2DRenderContext& ctx, UiRect dirtyRect);
        bool canUseFastAquariumViewportFrame(UiRect dirtyRect) const;
        UiRect currentAquariumViewportDynamicLayerRect() const;
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
        void renderAquariumControlPanelHome(D2DRenderContext& ctx);
        void renderAquariumFullScreen3DMode(D2DRenderContext& ctx);
        void renderEngineEditorMode(D2DRenderContext& ctx);
        void enterEngineEditorMode();
        void leaveEngineEditorMode();
        bool handleEngineEditorClick(float x, float y);
        void commitEngineWorkspaceLayout();
        std::filesystem::path engineWorkspaceLayoutPath() const;
        void renderAquariumDx12ViewportSurface(D2DRenderContext& ctx, UiRect rect, bool debugTruthEnabled);
        void renderAquariumResizeProxyViewport(D2DRenderContext& ctx, UiRect rect);
        void renderAquariumSlateCompositeViewport(D2DRenderContext& ctx, UiRect rect, bool debugTruthEnabled);
        bool renderAquariumDirectCompositionFrame(std::string* error);
        am::renderer::scene::AceAquariumGpuViewportOverlay buildAquariumGpuViewportOverlay(UiRect viewportSurface, const ace::aquarium_ui::AceAquariumUiSnapshot& snapshot) const;
        void syncAquariumEmbeddedViewportWindow();
        void renderAquariumButton(D2DRenderContext& ctx, UiRect rect, const std::wstring& label, bool active = false);
        void renderAquariumMiniButton(D2DRenderContext& ctx, UiRect rect, const std::wstring& label, bool active = false);
        bool isAquariumButtonHovered(UiRect rect) const;
        int aquariumHotIdAt(float x, float y) const;
        UiRect aquariumHotRectById(int hotId) const;
        static UiRect inflateRect(UiRect rect, float padding);
        void invalidateRect(UiRect rect);
        void invalidateAquariumChrome();
        void renderAquariumLines(D2DRenderContext& ctx, const std::wstring& title, const std::vector<std::string>& lines, UiRect rect, std::size_t maxLines);
        struct AquariumScrollPanel
        {
            UiRect viewport{};
            UiRect track{};
            UiRect thumb{};
            float offset = 0.0f;
            float contentHeight = 0.0f;
            float viewportHeight = 0.0f;
            float maxScroll = 0.0f;
            std::size_t lineCount = 0;
            std::size_t visibleLogStart = 0;
            std::size_t visibleLogEnd = 0;
            bool userScrolled = false;
            bool autoScrollWhenAtBottom = true;
        };

        void renderAquariumScrollableLines(D2DRenderContext& ctx, const std::wstring& title, const std::vector<std::string>& lines, UiRect rect, AquariumScrollPanel& scroll, std::size_t maxVisibleLines);
        void renderAquariumScrollbar(D2DRenderContext& ctx, AquariumScrollPanel& scroll);
        void clampAquariumScroll(AquariumScrollPanel& scroll);
        bool handleAquariumWheel(float x, float y, int wheelDelta);
        bool beginAquariumScrollbarDrag(float x, float y);
        bool updateAquariumScrollbarDrag(float x, float y);
        void endAquariumScrollbarDrag();
        bool beginAquariumPanelResize(float x, float y);
        bool updateAquariumPanelResize(float x, float y);
        void endAquariumPanelResize();
        AquariumScrollPanel* activeAquariumScrollPanelAt(float x, float y);
        bool handleAquariumPanelClick(float x, float y);
        bool isAquariumSingleHwndViewportInputAvailable() const;
        bool isAquariumSingleHwndViewportPoint(float x, float y) const;
        bool shouldAquariumViewportInputDeferToOverlay(float x, float y) const;
        bool beginAquariumSingleHwndMouseLook(float x, float y, const char* reason);
        bool updateAquariumSingleHwndMouseLook(float x, float y, const char* reason);
        bool registerAquariumRawMouseInput();
        void unregisterAquariumRawMouseInput();
        bool consumeAquariumRawMouseDelta();
        bool endAquariumSingleHwndMouseLook(const char* reason);
        void cancelAquariumSingleHwndMouseLook(const char* reason);
        std::string aquariumViewportInputDiagnostics() const;
        std::wstring aquariumViewportInputDiagnosticsWide() const;
        void cycleAquariumScenario(int direction);
        void cycleAquariumPlanner(int direction);
        std::wstring widen(const std::string& text) const;
        void startNewConversation();
        void selectConversation(std::size_t index);
        void prefillRenameConversation(std::size_t index);
        void saveActiveConversation();
        bool handleLocalInputCommand(const std::wstring& text);
        bool executeEngineStatsCommand(const std::wstring& commandId);
        AceEngineStatsSnapshot buildEngineStatsSnapshot() const;
        std::wstring formatStatCoords(std::string* logLine = nullptr) const;
        std::wstring formatStatRhi(std::string* logLine = nullptr) const;
        std::wstring formatStatFps(std::string* logLine = nullptr) const;
        std::wstring formatStatUi(std::string* logLine = nullptr) const;
        void appendEngineCommandOutput(const std::wstring& title, const std::wstring& body, const std::string& logTag, const std::string& logLine);

        void sendCurrentInput();
        void appendAssistantMockReply();
        void appendExternalMessage(ChatMessage message);
        void appendSubmitResult(ChatSubmitResult result);
        void refreshSnapshot();
        void refreshSuggestions();
        void refreshInspector(am::core::AceUiSelection selection);
        void inspectLocalMessage(std::uint64_t messageId);
        void refreshDiagnostics();
        void refreshUiDebugOverlay();
        void renderUiDebugOverlay(D2DRenderContext& ctx);
        void toggleEngineLogOverlay();
        void refreshEngineLogOverlayLines();
        bool submitEngineLogOverlayInput();
        bool handleEngineLogOverlayMouseDown(D2DRenderContext& ctx, float x, float y, unsigned clickCount);
        bool handleEngineLogOverlayMouseUp(D2DRenderContext& ctx, float x, float y);
        bool handleEngineLogOverlayMouseMove(D2DRenderContext& ctx, float x, float y, bool leftButtonDown);
        bool handleEngineLogOverlayWheel(D2DRenderContext& ctx, float x, float y, int wheelDelta);
        bool handleEngineLogOverlayChar(WPARAM wParam);
        bool handleEngineLogOverlayKeyDown(WPARAM wParam, const D2DKeyboardState& keyboard);
        struct EngineLogTextPosition
        {
            std::size_t line = 0;
            std::size_t column = 0;
            bool valid = false;
        };
        EngineLogTextPosition hitTestEngineLogText(float x, float y) const;
        Microsoft::WRL::ComPtr<IDWriteTextLayout> engineLogTextLayoutForLine(std::size_t line) const;
        bool engineLogHasTextSelection() const;
        std::wstring selectedEngineLogText() const;
        void clearEngineLogTextSelection();
        void selectAllEngineLogText();
        bool copyEngineLogTextSelectionToClipboard();
        void renderEngineLogTextSelection(D2DRenderContext& ctx, float lineHeight) const;
        void renderEngineLogOverlay(D2DRenderContext& ctx);
        void renderAquariumViewportHudLayer(D2DRenderContext& ctx, UiRect viewportRect, const ace::aquarium_ui::AceAquariumUiSnapshot& snapshot);
        UiRect computeEngineLogOverlayRect(UiRect anchor) const;
        UiRect computeAquariumTelemetryOverlayRect(UiRect viewportRect, UiRect avoidRect) const;
        UiRect computeAquariumNativeViewportRect(UiRect logicalRect) const;
        bool isViewportLocalOverlayActive() const;
        void requestParentCompositedViewportHold(std::uint32_t frames, const wchar_t* reason);
        bool shouldUseDirectCompositionForAquariumViewport() const;
        bool renderAquariumD2DCompositionHud(const ace::aquarium_ui::AceAquariumUiSnapshot& snapshot, std::string* error);
        bool ensureAquariumD2DCompositionHud(UiRect hudRect, std::string* error);
        void resetAquariumD2DCompositionHud();
        void resetAquariumDirectCompositionIfActive();
        struct AquariumViewportCacheKey
        {
            am::renderer::rhi::U32 width = 0;
            am::renderer::rhi::U32 height = 0;
            int step = -1;
            int cameraX = 0;
            int cameraY = 0;
            int cameraZ = 0;
            int yaw = 0;
            int pitch = 0;
            bool debugTruth = false;
            std::string scenario;

            bool operator==(const AquariumViewportCacheKey& other) const;
        };
        AquariumViewportCacheKey makeAquariumViewportCacheKey(am::renderer::rhi::U32 width, am::renderer::rhi::U32 height, bool debugTruth) const;
        bool canReuseAquariumViewportBitmap(const AquariumViewportCacheKey& key) const;
        bool drawCachedAquariumSlateViewportElement(D2DRenderContext& ctx, UiRect rect, const AquariumViewportCacheKey& key);
        bool drawAquariumGpuSnapshotAsSlateViewportElement(D2DRenderContext& ctx, UiRect rect, const am::renderer::scene::AceAquariumGpuViewportSnapshot& snapshot, const AquariumViewportCacheKey& key, std::string* error);
        bool drawAquariumGpuTextureWithD2DDeviceContext(D2DRenderContext& ctx, UiRect rect, const am::renderer::scene::AceAquariumGpuViewportSnapshot& snapshot, const AquariumViewportCacheKey& key, std::string* error);
        bool ensureAquariumD2DInteropDevices(std::string* error);
        bool ensureAquariumD2DTextureBridge(const am::renderer::rhi::AceViewportTextureResource& resource, std::string* error);
        void resetAquariumD2DTextureBridge();
        void resetAquariumD2DTextureBridgeResourceObjects();
        void setAquariumD2DBridgeFatalError(const std::string& error);
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
        void refreshDisplayMetrics(const char* reason);
        float dpiScale() const;
        void layoutForLiveResize(int width, int height);
        void beginWindowLiveResize();
        void endWindowLiveResize();
        void captureFrozenNativeResizeRect();
        bool handleFrozenNativeResizeSizing(LPARAM lParam);
        void applyFrozenNativeResizeCommit();
        bool shouldFreezeNativeLiveResize() const;
        void applyDeferredLiveResize();
        void forceLiveResizeProxyRepaintNow();
        void restoreEmbeddedViewportAfterLiveResize();
        bool ensureAquariumResizeShieldWindow();
        void updateAquariumResizeShieldWindow(UiRect rect);
        void hideAquariumResizeShieldWindow();
        void destroyAquariumResizeShieldWindow();
        static LRESULT CALLBACK AquariumResizeShieldWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
        bool isWindowLiveResizeActive() const { return windowLiveResizeActive_; }
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
        bool engineEditorModeActive_ = false;
        bool engineWorkspaceLoaded_ = false;
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
        D2DDisplayMetricsSnapshot displayMetrics_{};
        AceUiInvalidationRoot uiInvalidation_{};
        AceUiRetainedLayoutTree uiRetainedLayout_{};
        AceUiStyleSet uiStyleSet_{};
        D2DDrawCommandBuffer uiDrawCommands_{};
        D2DUiDebugOverlay uiDebugOverlay_{};
        D2DAquariumTelemetryWidgets aquariumTelemetryWidgets_{};
        am::editor::EditorWorkspaceController engineWorkspaceController_{};
        std::uint64_t aceUi5TextClampCount_ = 0;
        std::uint64_t aceUi6RetainedLayoutFrameCount_ = 0;
        std::uint64_t aceUi7DrawCommandFrameCount_ = 0;
        std::uint64_t aceUi8InvalidationFrameCount_ = 0;
        std::uint64_t aceUi9DebugOverlayToggleCount_ = 0;
        bool engineLogOverlayVisible_ = false;
        std::vector<std::wstring> engineLogOverlayLines_;
        mutable std::vector<Microsoft::WRL::ComPtr<IDWriteTextLayout>> engineLogOverlayLineLayouts_;
        UiRect engineLogOverlayRect_{};
        UiRect engineLogOverlayLogViewportRect_{};
        UiRect engineLogOverlayInputRect_{};
        AquariumScrollPanel engineLogOverlayScroll_{};
        D2DTextInput engineLogOverlayInput_{};
        bool engineLogOverlayInputFocused_ = false;
        bool engineLogTextFocused_ = false;
        bool engineLogTextSelecting_ = false;
        EngineLogTextPosition engineLogSelectionAnchor_{};
        EngineLogTextPosition engineLogSelectionActive_{};
        std::uint32_t engineLogLastClickTime_ = 0;
        float engineLogLastClickX_ = 0.0f;
        float engineLogLastClickY_ = 0.0f;
        unsigned engineLogClickCount_ = 0;
        std::uint64_t engineLogOverlayToggleCount_ = 0;
        bool engineLogOverlaySuppressNextBacktickChar_ = false;
        float uiDpiScale_ = 1.0f;
        std::uint64_t displayMetricsRefreshCount_ = 0;
        std::uint64_t d2dPixelDpiFixApplyCount_ = 0;
        std::uint64_t dpiChangedMessageCount_ = 0;
        std::uint64_t displayChangeMessageCount_ = 0;
        float cyberTextTime_ = 0.0f;
        bool settingsHovered_ = false;
        bool environmentHovered_ = false;
        float mouseX_ = 0.0f;
        float mouseY_ = 0.0f;
        ace::aquarium_ui::AceAquariumRuntimeController aquariumController_;
        ace::aquarium_render::AceAquariumSceneAdapter aquariumSceneAdapter_{};
        ace::aquarium_render::AceAquariumViewport aquariumViewport_{};
        ace::aquarium_render::AceAquariumCamera aquariumCamera_{};
        ace::aquarium_ui::AceEnvironmentWorkspace environmentWorkspace_{};
        ace::aquarium_ui::AceEnvironment3DMode environment3DMode_{};
        ace::aquarium_render::AceAquariumEmbeddedDx12Viewport aquariumEmbeddedDx12Viewport_{};
        std::unique_ptr<am::renderer::scene::AceAquariumGpuViewportRenderer> aquariumGpuViewportRenderer_{};
        ace::aquarium_render::AceAquariumRealCamera aquariumSingleHwndCamera_{};
        bool aquariumSingleHwndMouseLookActive_ = false;
        bool aquariumSingleHwndMouseCaptured_ = false;
        float aquariumSingleHwndLastMouseX_ = 0.0f;
        float aquariumSingleHwndLastMouseY_ = 0.0f;
        bool aquariumRawMouseRegistered_ = false;
        LONG aquariumPendingRawMouseX_ = 0;
        LONG aquariumPendingRawMouseY_ = 0;
        std::uint64_t aquariumRawMousePacketCount_ = 0;
        std::uint64_t aquariumRawMouseConsumeCount_ = 0;
        std::uint64_t aquariumViewportInputRmbDownCount_ = 0;
        std::uint64_t aquariumViewportInputRmbUpCount_ = 0;
        std::uint64_t aquariumViewportInputBeginCount_ = 0;
        std::uint64_t aquariumViewportInputEndCount_ = 0;
        std::uint64_t aquariumViewportInputCancelCount_ = 0;
        std::uint64_t aquariumViewportInputCaptureLostCount_ = 0;
        std::uint64_t aquariumViewportInputHitCount_ = 0;
        std::uint64_t aquariumViewportInputMissCount_ = 0;
        std::uint64_t aquariumViewportInputOverlayBlockedCount_ = 0;
        std::uint64_t aquariumViewportInputSceneMoveCount_ = 0;
        std::uint64_t aquariumViewportInputZeroDeltaMoveCount_ = 0;
        std::uint64_t aquariumViewportInputInvalidationCount_ = 0;
        std::uint64_t aquariumViewportInputPollBeginCount_ = 0;
        float aquariumViewportInputLastX_ = 0.0f;
        float aquariumViewportInputLastY_ = 0.0f;
        float aquariumViewportInputLastDeltaX_ = 0.0f;
        float aquariumViewportInputLastDeltaY_ = 0.0f;
        std::string aquariumViewportInputLastRoute_ = "idle";
        // ACE-AQ3D12: main Environment 3D path is single-HWND composition.
        // The legacy child HWND/DX12 swapchain remains available as fallback only.
        bool aquariumUseSingleHwndCompositeViewport_ = true;
        std::uint64_t aquariumCompositeFrameCount_ = 0;
        std::uint64_t aquariumCompositeCachedResizeFrameCount_ = 0;
        std::uint64_t aquariumLegacyChildSuppressedCount_ = 0;
        AceEngineRenderPath aquariumActiveRenderPath_ = AceEngineRenderPath::Unknown;
        AceEnginePerfStats enginePerfStats_{};
        bool aquariumCompositeViewportFrameValid_ = false;
        UiRect aquariumCompositeViewportLastRect_{};
        bool aquariumEmbeddedViewportVisible_ = false;
        bool aquariumEmbeddedViewportSyncNeeded_ = false;
        bool aquariumPendingViewportValid_ = false;
        bool aquarium3DModeActive_ = false;
        bool windowLiveResizeActive_ = false;
        bool pendingResizeAfterLiveDrag_ = false;
        int pendingLiveResizeWidth_ = 0;
        int pendingLiveResizeHeight_ = 0;
        bool frozenNativeLiveResizeActive_ = false;
        bool frozenNativeLiveResizeRectValid_ = false;
        bool pendingNativeLiveResizeRectValid_ = false;
        RECT frozenNativeLiveResizeRect_{};
        RECT pendingNativeLiveResizeRect_{};
        std::uint64_t nativeResizeFreezeEnterCount_ = 0;
        std::uint64_t nativeResizeFrozenSizingMessageCount_ = 0;
        std::uint64_t nativeResizePreventedClientResizeCount_ = 0;
        std::uint64_t nativeResizeFinalCommitCount_ = 0;
        bool gradientsDirty_ = false;
        bool aquariumViewportHiddenForLiveResize_ = false;
        bool aquariumViewportWasVisibleBeforeLiveResize_ = false;
        bool aquariumResizeQuarantineActive_ = false;
        float aquariumResizeQuarantineDelaySeconds_ = 0.0f;
        std::uint64_t aquariumResizeQuarantineEnterCount_ = 0;
        std::uint64_t aquariumResizeQuarantineExitCount_ = 0;
        std::uint64_t aquariumResizeQuarantineProxyFrameCount_ = 0;
        UiRect aquariumEmbeddedViewportRect_{};
        UiRect aquariumNativeViewportRect_{};
        UiRect aquariumPendingViewportRect_{};
        UiRect aquariumTelemetryOverlayRect_{};
        Microsoft::WRL::ComPtr<ID2D1Bitmap> aquariumSlateViewportBitmap_{};
        am::renderer::rhi::Extent2D aquariumSlateViewportBitmapExtent_{};
        Microsoft::WRL::ComPtr<ID3D11Device> aquariumBridgeD3D11Device_{};
        Microsoft::WRL::ComPtr<ID3D11DeviceContext> aquariumBridgeD3D11Context_{};
        Microsoft::WRL::ComPtr<ID3D11On12Device> aquariumBridgeD3D11On12_{};
        Microsoft::WRL::ComPtr<ID2D1Factory1> aquariumBridgeD2DFactory1_{};
        Microsoft::WRL::ComPtr<ID2D1Device> aquariumBridgeD2DDevice_{};
        Microsoft::WRL::ComPtr<ID2D1DeviceContext> aquariumBridgeD2DContext_{};
        Microsoft::WRL::ComPtr<ID3D11Resource> aquariumBridgeWrappedResource_{};
        Microsoft::WRL::ComPtr<ID3D11Texture2D> aquariumBridgeUiSharedTexture_{};
        Microsoft::WRL::ComPtr<ID3D11Texture2D> aquariumBridgeInteropSharedTexture_{};
        Microsoft::WRL::ComPtr<ID2D1Bitmap1> aquariumBridgeSurfaceBitmap_{};
        // ACE-VTBRIDGE4: CreateSharedBitmap into ID2D1HwndRenderTarget is forbidden.
        // Legacy validator breadcrumb only: __uuidof(ID2D1Bitmap), CreateSharedBitmap from D2D device-context bitmap.
        Microsoft::WRL::ComPtr<ID2D1Bitmap> aquariumBridgeSharedBitmap_{};
        struct AquariumD2DSharedBridgeSlot
        {
            Microsoft::WRL::ComPtr<ID3D11Texture2D> uiTexture{};
            Microsoft::WRL::ComPtr<ID3D11Texture2D> interopTexture{};
            Microsoft::WRL::ComPtr<ID2D1Bitmap1> surfaceBitmap{};
            Microsoft::WRL::ComPtr<IDXGIKeyedMutex> uiMutex{};
            Microsoft::WRL::ComPtr<IDXGIKeyedMutex> interopMutex{};
            HANDLE sharedHandle = nullptr;
            bool ready = false;
            bool keyedMutex = false;
        };
        static constexpr std::size_t kAquariumD2DSharedBridgeSlotCount = 2;
        std::array<AquariumD2DSharedBridgeSlot, kAquariumD2DSharedBridgeSlotCount> aquariumBridgeSharedSlots_{};
        void* aquariumBridgeNativeResource_ = nullptr;
        HANDLE aquariumBridgeSharedHandle_ = nullptr;
        am::renderer::rhi::Extent2D aquariumBridgeBitmapExtent_{};
        bool aquariumBridgeUsesSharedIntermediate_ = false;
        std::size_t aquariumBridgeSharedWriteIndex_ = 0;
        std::size_t aquariumBridgeSharedReadyIndex_ = 0;
        bool aquariumBridgeSharedHasReadyFrame_ = false;
        std::string aquariumD2DBridgeLastError_;
        std::string aquariumD2DBridgeSurfaceDiagnostics_;
        std::string aquariumD2DBridgeFatalStep_;
        std::string aquariumD2DBridgeFatalHresult_;
        bool aquariumD2DBridgeDisabled_ = false;
        bool aquariumDirectCompositionActive_ = false;
        Microsoft::WRL::ComPtr<IDXGISwapChain1> aquariumD2DCompositionHudSwapChain_{};
        Microsoft::WRL::ComPtr<ID2D1Bitmap1> aquariumD2DCompositionHudTarget_{};
        am::renderer::rhi::Extent2D aquariumD2DCompositionHudExtent_{};
        UiRect aquariumD2DCompositionHudRect_{};
        bool aquariumD2DCompositionHudAttached_ = false;
        bool aquariumD2DCompositionHudCacheValid_ = false;
        double aquariumD2DCompositionHudHydration_ = 0.0;
        double aquariumD2DCompositionHudNutrition_ = 0.0;
        double aquariumD2DCompositionHudIntegrity_ = 0.0;
        std::uint64_t aquariumD2DCompositionHudDrawCount_ = 0;
        std::uint64_t aquariumD2DCompositionHudPresentCount_ = 0;
        bool aquariumResetDirectCompositionAfterPaint_ = false;
        bool aquariumViewportLocalOverlayActiveLastFrame_ = false;
        std::uint32_t aquariumParentCompositedHoldFrames_ = 0;
        std::uint64_t aquariumViewportLayerModeSwitchCount_ = 0;
        std::uint64_t aquariumSlateViewportBitmapReuseCount_ = 0;
        std::uint64_t aquariumD2DBridgeAttemptCount_ = 0;
        std::uint64_t aquariumD2DBridgeSuccessCount_ = 0;
        std::uint64_t aquariumD2DBridgeDirectSuccessCount_ = 0;
        std::uint64_t aquariumD2DBridgeSharedSuccessCount_ = 0;
        std::uint64_t aquariumD2DBridgeSharedBufferCount_ = 0;
        std::uint64_t aquariumD2DBridgeSharedWriteIndexStat_ = 0;
        std::uint64_t aquariumD2DBridgeSharedDrawIndexStat_ = 0;
        std::uint64_t aquariumD2DBridgeSharedBitmapRecreateCount_ = 0;
        std::uint64_t aquariumD2DBridgeSharedCopyCount_ = 0;
        std::uint64_t aquariumD2DBridgeSharedMutexAcquireCount_ = 0;
        std::uint64_t aquariumD2DBridgeSharedMutexContentionCount_ = 0;
        std::uint64_t aquariumD2DBridgeSharedD2DFlushCount_ = 0;
        bool aquariumViewportCacheKeyValid_ = false;
        AquariumViewportCacheKey aquariumViewportCacheKey_{};
        std::uint64_t aquariumViewportCacheHitCount_ = 0;
        std::uint64_t aquariumViewportCacheMissCount_ = 0;
        std::uint64_t aquariumFastViewportPaintCount_ = 0;
        std::uint64_t aquariumFullViewportPaintCount_ = 0;
        std::wstring aquariumViewportLayerReason_ = L"scene-owned";
        std::wstring aquariumEmbeddedViewportStatus_ = L"DX12 Environment ready";
        bool aquariumControllerReady_ = false;
        bool aquariumLogsFocus_ = false;
        bool aquariumDetailsPanelVisible_ = true;
        bool aquariumLogsPanelVisible_ = true;
        ace::aquarium_ui::AceEnvironment3DPanelState aquarium3DPanelState_{};
        enum class AquariumPanelResizeTarget
        {
            None,
            LeftDetails,
            RightLogs
        };
        AquariumPanelResizeTarget aquariumPanelResizeTarget_ = AquariumPanelResizeTarget::None;
        PanelResizeEdge aquariumPanelResizeEdges_ = PanelResizeEdge::None;
        float aquariumPanelResizeStartX_ = 0.0f;
        float aquariumPanelResizeStartY_ = 0.0f;
        ace::aquarium_ui::AceEnvironment3DPanelState aquariumPanelResizeStartState_{};
        int aquariumHoverHotId_ = -1;
        std::uint64_t aquariumHoverStateChangeCount_ = 0;
        std::uint64_t aquariumUiLayoutPassCount_ = 0;
        std::uint64_t aquariumUiRebuildCount_ = 0;
        std::uint64_t aquariumUiRepaintCount_ = 0;
        std::uint64_t parentPaintCount_ = 0;
        std::uint64_t fullInvalidationCount_ = 0;
        std::uint64_t partialInvalidationCount_ = 0;
        std::uint64_t hoverInvalidationCount_ = 0;
        std::uint64_t childSyncCount_ = 0;
        std::uint64_t liveResizeEnterCount_ = 0;
        std::uint64_t liveResizeExitCount_ = 0;
        std::uint64_t liveResizeDeferredSizeCount_ = 0;
        std::uint64_t liveResizeAppliedFinalCount_ = 0;
        std::uint64_t rendererRecreateDuringLiveResizeCount_ = 0;
        std::uint64_t childMoveDuringLiveResizeCount_ = 0;
        std::uint64_t viewportHideForLiveResizeCount_ = 0;
        std::uint64_t viewportShowAfterLiveResizeCount_ = 0;
        std::uint64_t liveResizeProxyPaintCount_ = 0;
        std::uint64_t d2dResizeDuringLiveResizeCount_ = 0;
        std::uint64_t liveResizeSynchronousProxyPaintCount_ = 0;
        HWND aquariumResizeShieldHwnd_ = nullptr;
        bool aquariumResizeShieldVisible_ = false;
        bool aquariumResizeShieldClassRegistered_ = false;
        UiRect aquariumResizeShieldRect_{};
        std::uint64_t resizeShieldCreateCount_ = 0;
        std::uint64_t resizeShieldShowCount_ = 0;
        std::uint64_t resizeShieldMoveCount_ = 0;
        std::uint64_t resizeShieldPaintCount_ = 0;
        std::uint64_t resizeShieldHideCount_ = 0;
        UiRect aquariumLastViewportRect_{};
        UiRect aquariumLastLeftPanelRect_{};
        UiRect aquariumLastRightPanelRect_{};
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
        UiRect aquariumDetailsDebugRect_{};
        UiRect aquariumLogsFocusRect_{};
        UiRect aquariumDetailsToggleRect_{};
        UiRect aquariumLogsToggleRect_{};
        UiRect aquariumEngineModeRect_{};
        UiRect engineBackToAiRect_{};
        UiRect engineOutlinerToggleRect_{};
        UiRect engineDetailsToggleRect_{};
        UiRect engineResetLayoutRect_{};
        UiRect aquariumLeftPanelRect_{};
        UiRect aquariumRightLogsPanelRect_{};
        UiRect aquariumLeftResizeHandleRect_{};
        UiRect aquariumRightResizeHandleRect_{};
        UiRect aquariumOpen3DRect_{};
        UiRect aquariumCameraResetRect_{};
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
        AquariumScrollPanel aquarium3DLogsScroll_{};
        AquariumScrollPanel aquarium3DDetailsScroll_{};
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

        Microsoft::WRL::ComPtr<ID2D1Factory1> d2dFactory_;
        Microsoft::WRL::ComPtr<IDWriteFactory> dwriteFactory_;
        Microsoft::WRL::ComPtr<ID3D11Device> uiD3D11Device_;
        Microsoft::WRL::ComPtr<ID3D11DeviceContext> uiD3D11Context_;
        Microsoft::WRL::ComPtr<IDXGIFactory2> uiDxgiFactory_;
        Microsoft::WRL::ComPtr<IDXGISwapChain1> uiSwapChain_;
        Microsoft::WRL::ComPtr<ID2D1Device> uiD2DDevice_;
        Microsoft::WRL::ComPtr<ID2D1DeviceContext> renderTarget_;
        Microsoft::WRL::ComPtr<ID2D1Bitmap1> uiD2DTargetBitmap_;
        AceD2DFlipFrameCompositor d2dFrameCompositor_{};
        AceD2DSharedViewportBridgePolicy d2dViewportBridgePolicy_{};
        AceD2DViewportBridgeRuntime d2dViewportBridgeRuntime_{};
        AceD2DViewportCopyScheduler d2dViewportCopyScheduler_{};
        AceD2DViewportTextureCache d2dViewportTextureCache_{8};
        AceD2DPresentScheduler d2dPresentScheduler_{};
        AceD2DFrameDiagnostics d2dFrameDiagnostics_{};
        AceD2DCompositorAudit d2dCompositorAudit_{};
        AceD2DFrameTransaction d2dFrameTransaction_{};
        slate::AceSlateWindowElementList slateFrameElements_{};
        slate::AceSlateWindowRendererPipeline slateRendererPipeline_{};
        slate::AceSlateLayerTree slateLayerTree_{};
        slate::AceSlatePaintJournal slatePaintJournal_{32};
        slate::AceSlateViewportElementBuilder slateViewportElementBuilder_{};
        std::uint64_t d2dFrameInvalidationSerial_ = 0;
        double runtimeFrameDeltaSeconds_ = 0.0;
        bool uiVsyncEnabled_ = false;

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
