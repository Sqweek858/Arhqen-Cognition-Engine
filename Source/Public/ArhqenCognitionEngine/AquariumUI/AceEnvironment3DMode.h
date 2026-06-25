#pragma once

namespace ace::aquarium_ui
{
    struct AceEnvironment3DModeRect
    {
        float left = 0.0f;
        float top = 0.0f;
        float right = 0.0f;
        float bottom = 0.0f;

        float Width() const { return right - left; }
        float Height() const { return bottom - top; }
        float Area() const { return Width() * Height(); }
        bool Empty() const { return Width() <= 0.0f || Height() <= 0.0f; }
    };

    struct AceEnvironment3DLayoutConstants
    {
        // ACE-UI3G: logical 3D UI scale constants. D2D stays pixel-space
        // after UI3F; these values give the 3D tool a readable “normal” scale
        // without letting monitor DPI inflate coordinates behind our back.
        float LogicalScale = 1.16f;
        float TopbarHeight = 30.0f;
        float WindowPad = 12.0f;
        float PanelGap = 12.0f;
        float PanelInset = 12.0f;
        float PanelTitleHeight = 34.0f;
        float SectionGap = 12.0f;
        float SectionHeaderHeight = 32.0f;
        float SectionInnerPad = 12.0f;
        float ButtonHeight = 30.0f;
        float ButtonGap = 9.0f;
        float RowGap = 9.0f;
        float ResizeHandleSize = 20.0f;
        float ResizeHandlePad = 8.0f;
        float MinPanelWidth = 326.0f;
        float MaxPanelWidth = 500.0f;
        float MinPanelHeight = 790.0f;
        float MaxPanelHeight = 920.0f;
        float RuntimeSectionHeight = 124.0f;
        float ScenarioPlannerSectionHeight = 72.0f;
        float MainControlsSectionHeight = 156.0f;
        float ManualActionsSectionHeight = 194.0f;
        float MinInspectorHeight = 168.0f;
    };

    struct AceEnvironment3DPanelState
    {
        bool detailsVisible = true;
        bool logsVisible = true;
        float detailsWidth = 430.0f;
        float detailsHeight = 850.0f;
        float logsWidth = 370.0f;
        float logsHeight = 850.0f;
    };

    struct AceEnvironment3DModeLayout
    {
        AceEnvironment3DModeRect mode{};
        AceEnvironment3DModeRect topOverlay{};
        AceEnvironment3DModeRect topbarLeftCluster{};
        AceEnvironment3DModeRect topbarCenterCluster{};
        AceEnvironment3DModeRect topbarRightCluster{};
        AceEnvironment3DModeRect topbarStatus{};
        AceEnvironment3DModeRect debugTruthWarning{};
        AceEnvironment3DModeRect topbarDebugTruth{};
        AceEnvironment3DModeRect detailsToggle{};
        AceEnvironment3DModeRect logsToggle{};
        AceEnvironment3DModeRect closeButton{};
        AceEnvironment3DModeRect backButton{};
        AceEnvironment3DModeRect leftPanel{};
        AceEnvironment3DModeRect rightLogsPanel{};
        AceEnvironment3DModeRect leftPanelTitle{};
        AceEnvironment3DModeRect rightLogsTitle{};
        AceEnvironment3DModeRect leftContentClip{};
        AceEnvironment3DModeRect rightLogsContent{};
        AceEnvironment3DModeRect rightLogsViewport{};
        AceEnvironment3DModeRect leftResizeHandle{};
        AceEnvironment3DModeRect rightResizeHandle{};
        AceEnvironment3DModeRect viewport{};
        AceEnvironment3DModeRect dx12Surface{};

        AceEnvironment3DModeRect leftRuntimeSection{};
        AceEnvironment3DModeRect leftScenarioPlannerSection{};
        AceEnvironment3DModeRect leftMainControlsSection{};
        AceEnvironment3DModeRect leftManualActionsSection{};
        AceEnvironment3DModeRect leftInspectorSection{};

        // ACE-AQ3D6: explicit clip rects for deterministic panel rendering.
        AceEnvironment3DModeRect runtimeClip{};
        AceEnvironment3DModeRect scenarioPlannerClip{};
        AceEnvironment3DModeRect mainControlsClip{};
        AceEnvironment3DModeRect manualActionsClip{};
        AceEnvironment3DModeRect inspectorClip{};
        AceEnvironment3DModeRect topbarStatusClip{};
        AceEnvironment3DModeRect topbarWarningClip{};

        // Legacy button slots kept so the AQ3D3/AQ3D4/AQ3D5 shell/probe contract remains source-compatible.
        AceEnvironment3DModeRect scenarioPrev{};
        AceEnvironment3DModeRect scenarioNext{};
        AceEnvironment3DModeRect plannerPrev{};
        AceEnvironment3DModeRect plannerNext{};
        AceEnvironment3DModeRect reset{};
        AceEnvironment3DModeRect step{};
        AceEnvironment3DModeRect runPause{};
        AceEnvironment3DModeRect debugTruth{};
        AceEnvironment3DModeRect cameraReset{};
        AceEnvironment3DModeRect manualForward{};
        AceEnvironment3DModeRect manualLeft{};
        AceEnvironment3DModeRect manualRight{};
        AceEnvironment3DModeRect manualWait{};
        AceEnvironment3DModeRect manualTouch{};
        AceEnvironment3DModeRect manualConsume{};
        AceEnvironment3DModeRect manualPush{};
        AceEnvironment3DModeRect bottomLogStrip{};
        AceEnvironment3DModeRect collapsedInspector{};
    };

    class AceEnvironment3DMode
    {
    public:
        AceEnvironment3DModeLayout Compute(float clientWidth, float clientHeight) const;
        AceEnvironment3DModeLayout Compute(float clientWidth, float clientHeight, const AceEnvironment3DPanelState& panelState) const;

        static const AceEnvironment3DLayoutConstants& Constants();
        void ClampPanelState(AceEnvironment3DPanelState& panelState, float clientWidth, float clientHeight) const;
        void ResizeLeftPanel(AceEnvironment3DPanelState& panelState, float requestedWidth, float requestedHeight, float clientWidth, float clientHeight) const;
        void ResizeRightPanel(AceEnvironment3DPanelState& panelState, float requestedWidth, float requestedHeight, float clientWidth, float clientHeight) const;
        bool LogsScrollChangesVisibleRange(int lineCount, float viewportHeight, float firstOffset, float secondOffset) const;
        bool LogsScrollOffsetClamped(int lineCount, float viewportHeight, float requestedOffset) const;
        bool LogsDoNotAutoScrollWhenUserScrolled(float oldOffset, float oldMaxOffset, float newMaxOffset) const;
        bool RectsStableAcrossIdleFrames(float clientWidth, float clientHeight, const AceEnvironment3DPanelState& panelState) const;
        bool LayoutDoesNotOscillate(float clientWidth, float clientHeight, const AceEnvironment3DPanelState& panelState) const;

        float ViewportAreaRatio(const AceEnvironment3DModeLayout& layout) const;
        bool ViewportAreaAtLeast70Percent(const AceEnvironment3DModeLayout& layout) const;
        int LayoutPassCount() const { return layoutPassCount_; }
        int UiRebuildCount() const { return uiRebuildCount_; }
        int UiRepaintCount() const { return uiRepaintCount_; }
        AceEnvironment3DModeRect LastViewportRect() const { return lastViewportRect_; }
        AceEnvironment3DModeRect LastLeftPanelRect() const { return lastLeftPanelRect_; }
        AceEnvironment3DModeRect LastRightPanelRect() const { return lastRightPanelRect_; }
        void MarkRepaint() const { ++uiRepaintCount_; }
        void MarkStructuralRebuild() const { ++uiRebuildCount_; }

    private:
        mutable int layoutPassCount_ = 0;
        mutable int uiRebuildCount_ = 0;
        mutable int uiRepaintCount_ = 0;
        mutable AceEnvironment3DModeRect lastViewportRect_{};
        mutable AceEnvironment3DModeRect lastLeftPanelRect_{};
        mutable AceEnvironment3DModeRect lastRightPanelRect_{};
    };
}
