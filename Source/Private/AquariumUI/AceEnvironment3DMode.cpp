#include "ArhqenCognitionEngine/AquariumUI/AceEnvironment3DMode.h"

#include <algorithm>
#include <cmath>

namespace ace::aquarium_ui
{
    namespace
    {
        AceEnvironment3DModeRect rect(float l, float t, float r, float b)
        {
            return {l, t, std::max(l, r), std::max(t, b)};
        }

        bool sameRect(const AceEnvironment3DModeRect& a, const AceEnvironment3DModeRect& b)
        {
            constexpr float eps = 0.001f;
            return std::abs(a.left - b.left) < eps &&
                std::abs(a.top - b.top) < eps &&
                std::abs(a.right - b.right) < eps &&
                std::abs(a.bottom - b.bottom) < eps;
        }

        bool intersects(const AceEnvironment3DModeRect& a, const AceEnvironment3DModeRect& b)
        {
            if (a.Empty() || b.Empty())
            {
                return false;
            }

            return a.left < b.right && a.right > b.left && a.top < b.bottom && a.bottom > b.top;
        }

        bool inside(const AceEnvironment3DModeRect& child, const AceEnvironment3DModeRect& parent)
        {
            if (child.Empty())
            {
                return true;
            }
            return child.left >= parent.left - 0.001f && child.right <= parent.right + 0.001f &&
                child.top >= parent.top - 0.001f && child.bottom <= parent.bottom + 0.001f;
        }

        AceEnvironment3DModeRect nextSection(float& y, const AceEnvironment3DModeRect& content, float height, float gap)
        {
            if (y >= content.bottom)
            {
                return rect(content.left, content.bottom, content.right, content.bottom);
            }

            const AceEnvironment3DModeRect out = rect(content.left, y, content.right, std::min(content.bottom, y + height));
            y = out.bottom + gap;
            return out;
        }

        AceEnvironment3DModeRect sectionClip(const AceEnvironment3DModeRect& section, const AceEnvironment3DLayoutConstants& c)
        {
            return rect(
                section.left + c.SectionInnerPad,
                section.top + c.SectionHeaderHeight,
                section.right - c.SectionInnerPad,
                section.bottom - c.SectionInnerPad
            );
        }

        const AceEnvironment3DLayoutConstants kConstants{};
    }

    const AceEnvironment3DLayoutConstants& AceEnvironment3DMode::Constants()
    {
        return kConstants;
    }

    AceEnvironment3DModeLayout AceEnvironment3DMode::Compute(float clientWidth, float clientHeight) const
    {
        AceEnvironment3DPanelState state{};
        state.detailsVisible = false;
        state.logsVisible = false;
        return Compute(clientWidth, clientHeight, state);
    }

    AceEnvironment3DModeLayout AceEnvironment3DMode::Compute(float clientWidth, float clientHeight, const AceEnvironment3DPanelState& panelState) const
    {
        const auto& c = Constants();
        const float width = std::max(920.0f, clientWidth);
        const float height = std::max(660.0f, clientHeight);

        AceEnvironment3DPanelState state = panelState;
        ClampPanelState(state, width, height);

        AceEnvironment3DModeLayout out{};
        out.mode = rect(0.0f, 0.0f, width, height);
        out.topOverlay = rect(0.0f, 0.0f, width, c.TopbarHeight);

        // ACE-AQ3D6: TopbarLayout is deterministic. Left, center and right clusters
        // are reserved before text is drawn, so status/debug strings cannot shove
        // buttons around or overlap during hover/click repaint. Long strings are
        // clipped by the shell inside topbarStatusClip/topbarWarningClip.
        const float buttonTop = 4.0f;
        const float buttonBottom = c.TopbarHeight - 4.0f;
        out.detailsToggle = rect(c.WindowPad, buttonTop, c.WindowPad + 140.0f, buttonBottom);
        out.closeButton = rect(width - c.WindowPad - 34.0f, buttonTop, width - c.WindowPad, buttonBottom);
        out.logsToggle = rect(out.closeButton.left - 98.0f, buttonTop, out.closeButton.left - 9.0f, buttonBottom);
        out.topbarDebugTruth = rect(out.logsToggle.left - 92.0f, buttonTop, out.logsToggle.left - 9.0f, buttonBottom);

        const float rightClusterDesiredLeft = out.topbarDebugTruth.left - 330.0f;
        const float rightClusterMinLeft = out.detailsToggle.right + 280.0f;
        const bool hasWarningRoom = rightClusterDesiredLeft > rightClusterMinLeft;
        out.debugTruthWarning = hasWarningRoom
            ? rect(rightClusterDesiredLeft, buttonTop, out.topbarDebugTruth.left - 10.0f, buttonBottom)
            : rect(out.topbarDebugTruth.left, buttonTop, out.topbarDebugTruth.left, buttonBottom);

        out.topbarLeftCluster = rect(c.WindowPad, 0.0f, out.detailsToggle.right, c.TopbarHeight);
        out.topbarRightCluster = rect(hasWarningRoom ? out.debugTruthWarning.left : out.topbarDebugTruth.left, 0.0f, out.closeButton.right, c.TopbarHeight);
        const float centerLeft = out.topbarLeftCluster.right + 14.0f;
        const float centerRight = std::max(centerLeft, out.topbarRightCluster.left - 14.0f);
        out.topbarStatus = rect(centerLeft, buttonTop, centerRight, buttonBottom);
        out.topbarCenterCluster = rect(out.topbarStatus.left, 0.0f, out.topbarStatus.right, c.TopbarHeight);
        out.topbarStatusClip = out.topbarStatus;
        out.topbarWarningClip = out.debugTruthWarning;
        out.backButton = out.closeButton;

        const float panelTop = out.topOverlay.bottom + c.WindowPad;
        const float panelBottomLimit = height - c.WindowPad;
        const float maxPanelH = std::max(c.MinPanelHeight, panelBottomLimit - panelTop);

        if (state.detailsVisible)
        {
            const float panelH = std::min(state.detailsHeight, maxPanelH);
            out.leftPanel = rect(c.WindowPad, panelTop, c.WindowPad + state.detailsWidth, panelTop + panelH);
            out.leftPanelTitle = rect(out.leftPanel.left + 14.0f, out.leftPanel.top + 9.0f, out.leftPanel.right - 14.0f, out.leftPanel.top + 32.0f);
            out.leftResizeHandle = {};
            out.leftContentClip = rect(
                out.leftPanel.left + c.PanelInset,
                out.leftPanel.top + c.PanelTitleHeight + 8.0f,
                out.leftPanel.right - c.PanelInset,
                out.leftPanel.bottom - c.PanelInset
            );

            float y = out.leftContentClip.top;
            out.leftRuntimeSection = nextSection(y, out.leftContentClip, c.RuntimeSectionHeight, c.SectionGap);
            out.leftScenarioPlannerSection = nextSection(y, out.leftContentClip, c.ScenarioPlannerSectionHeight, c.SectionGap);
            out.leftMainControlsSection = nextSection(y, out.leftContentClip, c.MainControlsSectionHeight, c.SectionGap);
            out.leftManualActionsSection = nextSection(y, out.leftContentClip, c.ManualActionsSectionHeight, c.SectionGap);
            out.leftInspectorSection = rect(out.leftContentClip.left, y, out.leftContentClip.right, out.leftContentClip.bottom);

            out.runtimeClip = sectionClip(out.leftRuntimeSection, c);
            out.scenarioPlannerClip = sectionClip(out.leftScenarioPlannerSection, c);
            out.mainControlsClip = sectionClip(out.leftMainControlsSection, c);
            out.manualActionsClip = sectionClip(out.leftManualActionsSection, c);
            out.inspectorClip = sectionClip(out.leftInspectorSection, c);

            const float scenarioY = out.scenarioPlannerClip.top + 2.0f;
            const float miniW = 46.0f;
            out.scenarioPrev = rect(out.scenarioPlannerClip.left, scenarioY, out.scenarioPlannerClip.left + miniW, scenarioY + c.ButtonHeight);
            out.scenarioNext = rect(out.scenarioPrev.right + c.ButtonGap, scenarioY, out.scenarioPrev.right + c.ButtonGap + miniW, scenarioY + c.ButtonHeight);
            const float pairW = miniW * 2.0f + c.ButtonGap;
            out.plannerPrev = rect(out.scenarioPlannerClip.right - pairW, scenarioY, out.scenarioPlannerClip.right - pairW + miniW, scenarioY + c.ButtonHeight);
            out.plannerNext = rect(out.plannerPrev.right + c.ButtonGap, scenarioY, out.plannerPrev.right + c.ButtonGap + miniW, scenarioY + c.ButtonHeight);

            const float mainLeft = out.mainControlsClip.left;
            const float mainRight = out.mainControlsClip.right;
            const float colW = std::max(78.0f, (mainRight - mainLeft - c.ButtonGap) * 0.5f);
            const float row1 = out.mainControlsClip.top;
            const float row2 = row1 + c.ButtonHeight + c.RowGap;
            const float row3 = row2 + c.ButtonHeight + c.RowGap;
            out.reset = rect(mainLeft, row1, mainLeft + colW, row1 + c.ButtonHeight);
            out.step = rect(out.reset.right + c.ButtonGap, row1, mainRight, row1 + c.ButtonHeight);
            out.runPause = rect(mainLeft, row2, mainLeft + colW, row2 + c.ButtonHeight);
            out.cameraReset = rect(out.runPause.right + c.ButtonGap, row2, mainRight, row2 + c.ButtonHeight);
            out.debugTruth = rect(mainLeft, row3, mainRight, row3 + c.ButtonHeight);

            const float manualLeft = out.manualActionsClip.left;
            const float manualRight = out.manualActionsClip.right;
            const float manualColW = std::max(78.0f, (manualRight - manualLeft - c.ButtonGap) * 0.5f);
            const float manualRow1 = out.manualActionsClip.top;
            const float manualRow2 = manualRow1 + c.ButtonHeight + c.RowGap;
            const float manualRow3 = manualRow2 + c.ButtonHeight + c.RowGap;
            const float manualRow4 = manualRow3 + c.ButtonHeight + c.RowGap;
            out.manualForward = rect(manualLeft, manualRow1, manualRight, manualRow1 + c.ButtonHeight);
            out.manualLeft = rect(manualLeft, manualRow2, manualLeft + manualColW, manualRow2 + c.ButtonHeight);
            out.manualRight = rect(out.manualLeft.right + c.ButtonGap, manualRow2, manualRight, manualRow2 + c.ButtonHeight);
            out.manualWait = rect(manualLeft, manualRow3, manualLeft + manualColW, manualRow3 + c.ButtonHeight);
            out.manualTouch = rect(out.manualWait.right + c.ButtonGap, manualRow3, manualRight, manualRow3 + c.ButtonHeight);
            out.manualConsume = rect(manualLeft, manualRow4, manualLeft + manualColW, manualRow4 + c.ButtonHeight);
            out.manualPush = rect(out.manualConsume.right + c.ButtonGap, manualRow4, manualRight, manualRow4 + c.ButtonHeight);
        }

        if (state.logsVisible)
        {
            const float panelH = std::min(state.logsHeight, maxPanelH);
            out.rightLogsPanel = rect(width - c.WindowPad - state.logsWidth, panelTop, width - c.WindowPad, panelTop + panelH);
            out.rightLogsTitle = rect(out.rightLogsPanel.left + 14.0f, out.rightLogsPanel.top + 9.0f, out.rightLogsPanel.right - 14.0f, out.rightLogsPanel.top + 32.0f);
            out.rightResizeHandle = {};
            out.rightLogsContent = rect(
                out.rightLogsPanel.left + 9.0f,
                out.rightLogsPanel.top + c.PanelTitleHeight + 6.0f,
                out.rightLogsPanel.right - 9.0f,
                out.rightLogsPanel.bottom - c.PanelInset
            );
            out.rightLogsViewport = rect(
                out.rightLogsContent.left + 10.0f,
                out.rightLogsContent.top + 32.0f,
                out.rightLogsContent.right - 24.0f,
                out.rightLogsContent.bottom - 8.0f
            );
        }

        // ACE-AQ3D6: viewport remains the stable dominant background; viewport is the stable dominant background.
        // The child
        // DX12 surface is clipped away from child-window-hosted panels so Win32
        // z-order does not cause flicker, but the full viewport area remains the
        // conceptual background and still clears/presents persistently.
        out.viewport = rect(c.WindowPad, out.topOverlay.bottom + c.PanelGap, width - c.WindowPad, height - c.WindowPad);
        const float dxLeft = state.detailsVisible ? out.leftPanel.right + c.PanelGap : out.viewport.left;
        const float dxRight = state.logsVisible ? out.rightLogsPanel.left - c.PanelGap : out.viewport.right;
        out.dx12Surface = rect(dxLeft, out.viewport.top, dxRight, out.viewport.bottom);
        out.bottomLogStrip = rect(out.viewport.left, out.viewport.bottom, out.viewport.right, out.viewport.bottom);
        out.collapsedInspector = rect(0.0f, 0.0f, 0.0f, 0.0f);

        ++layoutPassCount_;
        lastViewportRect_ = out.dx12Surface;
        lastLeftPanelRect_ = out.leftPanel;
        lastRightPanelRect_ = out.rightLogsPanel;
        return out;
    }

    void AceEnvironment3DMode::ClampPanelState(AceEnvironment3DPanelState& panelState, float clientWidth, float clientHeight) const
    {
        const auto& c = Constants();
        const float width = std::max(920.0f, clientWidth);
        const float height = std::max(660.0f, clientHeight);
        const float availableW = std::max(c.MinPanelWidth, width - (c.WindowPad * 2.0f) - c.PanelGap);
        const float availableH = std::max(c.MinPanelHeight, height - c.TopbarHeight - (c.WindowPad * 2.0f));
        const float reservedForLogs = panelState.logsVisible ? c.MinPanelWidth + c.PanelGap : 0.0f;
        panelState.detailsWidth = std::clamp(panelState.detailsWidth, c.MinPanelWidth, std::max(c.MinPanelWidth, availableW - reservedForLogs));
        const float reservedForDetails = panelState.detailsVisible ? panelState.detailsWidth + c.PanelGap : 0.0f;
        panelState.logsWidth = std::clamp(panelState.logsWidth, c.MinPanelWidth, std::max(c.MinPanelWidth, availableW - reservedForDetails));
        panelState.detailsHeight = std::clamp(panelState.detailsHeight, c.MinPanelHeight, availableH);
        panelState.logsHeight = std::clamp(panelState.logsHeight, c.MinPanelHeight, availableH);
    }

    void AceEnvironment3DMode::ResizeLeftPanel(AceEnvironment3DPanelState& panelState, float requestedWidth, float requestedHeight, float clientWidth, float clientHeight) const
    {
        panelState.detailsWidth = requestedWidth;
        panelState.detailsHeight = requestedHeight;
        ClampPanelState(panelState, clientWidth, clientHeight);
    }

    void AceEnvironment3DMode::ResizeRightPanel(AceEnvironment3DPanelState& panelState, float requestedWidth, float requestedHeight, float clientWidth, float clientHeight) const
    {
        panelState.logsWidth = requestedWidth;
        panelState.logsHeight = requestedHeight;
        ClampPanelState(panelState, clientWidth, clientHeight);
    }

    bool AceEnvironment3DMode::LogsScrollChangesVisibleRange(int lineCount, float viewportHeight, float firstOffset, float secondOffset) const
    {
        const float lineHeight = 19.0f;
        const float contentHeight = std::max(0.0f, static_cast<float>(lineCount) * lineHeight);
        const float maxOffset = std::max(0.0f, contentHeight - std::max(0.0f, viewportHeight));
        const float a = std::clamp(firstOffset, 0.0f, maxOffset);
        const float b = std::clamp(secondOffset, 0.0f, maxOffset);
        if (contentHeight <= viewportHeight + 1.0f)
        {
            return false;
        }
        return std::abs(a - b) >= 1.0f;
    }

    bool AceEnvironment3DMode::LogsScrollOffsetClamped(int lineCount, float viewportHeight, float requestedOffset) const
    {
        const float lineHeight = 19.0f;
        const float contentHeight = std::max(0.0f, static_cast<float>(lineCount) * lineHeight);
        const float maxOffset = std::max(0.0f, contentHeight - std::max(0.0f, viewportHeight));
        const float clamped = std::clamp(requestedOffset, 0.0f, maxOffset);
        return clamped >= 0.0f && clamped <= maxOffset;
    }

    bool AceEnvironment3DMode::LogsDoNotAutoScrollWhenUserScrolled(float oldOffset, float oldMaxOffset, float newMaxOffset) const
    {
        const float clampedOld = std::clamp(oldOffset, 0.0f, std::max(0.0f, oldMaxOffset));
        const bool userWasAwayFromBottom = clampedOld < std::max(0.0f, oldMaxOffset) - 2.0f;
        const float preserved = userWasAwayFromBottom ? clampedOld : std::max(0.0f, newMaxOffset);
        return userWasAwayFromBottom ? std::abs(preserved - clampedOld) < 0.001f : preserved >= newMaxOffset - 0.001f;
    }

    bool AceEnvironment3DMode::RectsStableAcrossIdleFrames(float clientWidth, float clientHeight, const AceEnvironment3DPanelState& panelState) const
    {
        const auto a = Compute(clientWidth, clientHeight, panelState);
        const auto b = Compute(clientWidth, clientHeight, panelState);
        return sameRect(a.viewport, b.viewport) &&
            sameRect(a.dx12Surface, b.dx12Surface) &&
            sameRect(a.leftPanel, b.leftPanel) &&
            sameRect(a.rightLogsPanel, b.rightLogsPanel) &&
            sameRect(a.topbarLeftCluster, b.topbarLeftCluster) &&
            sameRect(a.topbarCenterCluster, b.topbarCenterCluster) &&
            sameRect(a.topbarRightCluster, b.topbarRightCluster) &&
            !intersects(a.topbarLeftCluster, a.topbarCenterCluster) &&
            !intersects(a.topbarCenterCluster, a.topbarRightCluster) &&
            inside(a.debugTruth, a.leftMainControlsSection) &&
            inside(a.manualPush, a.leftManualActionsSection) &&
            a.leftInspectorSection.top >= a.leftManualActionsSection.bottom - 0.001f;
    }

    bool AceEnvironment3DMode::LayoutDoesNotOscillate(float clientWidth, float clientHeight, const AceEnvironment3DPanelState& panelState) const
    {
        const auto a = Compute(clientWidth, clientHeight, panelState);
        const auto b = Compute(clientWidth, clientHeight, panelState);
        const auto c = Compute(clientWidth, clientHeight, panelState);
        return sameRect(a.dx12Surface, b.dx12Surface) && sameRect(b.dx12Surface, c.dx12Surface) &&
            sameRect(a.leftContentClip, b.leftContentClip) && sameRect(b.leftContentClip, c.leftContentClip) &&
            sameRect(a.rightLogsContent, b.rightLogsContent) && sameRect(b.rightLogsContent, c.rightLogsContent);
    }

    float AceEnvironment3DMode::ViewportAreaRatio(const AceEnvironment3DModeLayout& layout) const
    {
        const float modeArea = std::max(1.0f, layout.mode.Area());
        return layout.viewport.Area() / modeArea;
    }

    bool AceEnvironment3DMode::ViewportAreaAtLeast70Percent(const AceEnvironment3DModeLayout& layout) const
    {
        return ViewportAreaRatio(layout) >= 0.70f;
    }
}
