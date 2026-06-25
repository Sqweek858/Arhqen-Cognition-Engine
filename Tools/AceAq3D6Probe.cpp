#include "ArhqenCognitionEngine/AquariumUI/AceEnvironment3DMode.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

namespace
{
    using ace::aquarium_ui::AceEnvironment3DModeRect;

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
        return child.left >= parent.left - 0.01f && child.right <= parent.right + 0.01f &&
            child.top >= parent.top - 0.01f && child.bottom <= parent.bottom + 0.01f;
    }

    bool sameRect(const AceEnvironment3DModeRect& a, const AceEnvironment3DModeRect& b)
    {
        return std::abs(a.left - b.left) < 0.001f &&
            std::abs(a.top - b.top) < 0.001f &&
            std::abs(a.right - b.right) < 0.001f &&
            std::abs(a.bottom - b.bottom) < 0.001f;
    }

    void assertNoOverlap(const std::vector<AceEnvironment3DModeRect>& rects)
    {
        for (std::size_t i = 0; i < rects.size(); ++i)
        {
            for (std::size_t j = i + 1; j < rects.size(); ++j)
            {
                assert(!intersects(rects[i], rects[j]));
            }
        }
    }
}

int main()
{
    ace::aquarium_ui::AceEnvironment3DMode mode;
    const auto& constants = ace::aquarium_ui::AceEnvironment3DMode::Constants();
    assert(constants.TopbarHeight == 24.0f);
    assert(constants.ButtonHeight == 26.0f);
    assert(constants.SectionGap >= 8.0f);
    assert(constants.MinPanelHeight >= 770.0f);

    ace::aquarium_ui::AceEnvironment3DPanelState state{};
    state.detailsVisible = true;
    state.logsVisible = true;
    state.detailsWidth = 388.0f;
    state.detailsHeight = 830.0f;
    state.logsWidth = 330.0f;
    state.logsHeight = 830.0f;

    const auto layout = mode.Compute(1708.0f, 887.0f, state);

    assert(layout.topOverlay.Height() <= 24.0f);
    assert(!intersects(layout.topbarLeftCluster, layout.topbarCenterCluster));
    assert(!intersects(layout.topbarCenterCluster, layout.topbarRightCluster));
    assert(inside(layout.topbarStatus, layout.topbarCenterCluster));
    assert(inside(layout.debugTruthWarning, layout.topbarRightCluster));
    assert(inside(layout.topbarStatusClip, layout.topbarStatus));
    assert(inside(layout.topbarWarningClip, layout.debugTruthWarning));

    assertNoOverlap({
        layout.leftRuntimeSection,
        layout.leftScenarioPlannerSection,
        layout.leftMainControlsSection,
        layout.leftManualActionsSection,
        layout.leftInspectorSection
    });

    assertNoOverlap({layout.reset, layout.step, layout.runPause, layout.cameraReset, layout.debugTruth});
    assertNoOverlap({
        layout.manualForward,
        layout.manualLeft,
        layout.manualRight,
        layout.manualWait,
        layout.manualTouch,
        layout.manualConsume,
        layout.manualPush
    });

    assert(inside(layout.reset, layout.leftMainControlsSection));
    assert(inside(layout.debugTruth, layout.leftMainControlsSection));
    assert(inside(layout.manualForward, layout.leftManualActionsSection));
    assert(inside(layout.manualPush, layout.leftManualActionsSection));
    assert(layout.leftInspectorSection.top >= layout.leftManualActionsSection.bottom);
    assert(!intersects(layout.leftResizeHandle, layout.leftContentClip));
    assert(!intersects(layout.rightResizeHandle, layout.rightLogsContent));
    assert(inside(layout.leftRuntimeSection, layout.leftContentClip));
    assert(inside(layout.leftManualActionsSection, layout.leftContentClip));
    assert(inside(layout.inspectorClip, layout.leftInspectorSection));
    assert(inside(layout.rightLogsViewport, layout.rightLogsContent));

    assert(mode.LogsScrollChangesVisibleRange(120, layout.rightLogsViewport.Height(), 0.0f, 160.0f));
    assert(mode.LogsScrollOffsetClamped(120, layout.rightLogsViewport.Height(), 99999.0f));
    assert(mode.LogsDoNotAutoScrollWhenUserScrolled(50.0f, 250.0f, 500.0f));

    ace::aquarium_ui::AceEnvironment3DPanelState resized = state;
    mode.ResizeLeftPanel(resized, state.detailsWidth + 42.0f, state.detailsHeight + 20.0f, 1708.0f, 887.0f);
    assert(resized.detailsWidth > state.detailsWidth);
    assert(resized.detailsHeight > state.detailsHeight);
    mode.ResizeRightPanel(resized, state.logsWidth + 42.0f, state.logsHeight + 20.0f, 1708.0f, 887.0f);
    assert(resized.logsWidth > state.logsWidth);
    assert(resized.logsHeight > state.logsHeight);
    mode.ResizeLeftPanel(resized, 10.0f, 10.0f, 1708.0f, 887.0f);
    assert(resized.detailsWidth >= constants.MinPanelWidth);
    assert(resized.detailsHeight >= constants.MinPanelHeight);

    assert(mode.RectsStableAcrossIdleFrames(1708.0f, 887.0f, state));
    assert(mode.LayoutDoesNotOscillate(1708.0f, 887.0f, state));
    const auto a = mode.Compute(1708.0f, 887.0f, state);
    const auto b = mode.Compute(1708.0f, 887.0f, state);
    assert(sameRect(a.topbarLeftCluster, b.topbarLeftCluster));
    assert(sameRect(a.topbarCenterCluster, b.topbarCenterCluster));
    assert(sameRect(a.topbarRightCluster, b.topbarRightCluster));
    assert(sameRect(a.leftPanel, b.leftPanel));
    assert(sameRect(a.rightLogsPanel, b.rightLogsPanel));
    assert(sameRect(a.dx12Surface, b.dx12Surface));

    const int rebuildBefore = mode.UiRebuildCount();
    mode.MarkRepaint();
    mode.MarkRepaint();
    assert(mode.UiRebuildCount() == rebuildBefore);
    assert(mode.UiRepaintCount() >= 2);

    assert(mode.ViewportAreaAtLeast70Percent(layout));
    assert(layout.dx12Surface.Width() > 820.0f);
    assert(layout.dx12Surface.Height() > 820.0f);
    assert(layout.viewport.Area() > layout.leftPanel.Area());
    assert(layout.viewport.Area() > layout.rightLogsPanel.Area());

    std::cout << "ACE-AQ3D6 viewport_area_ratio=" << mode.ViewportAreaRatio(layout) << "\n";
    std::cout << "ACE-AQ3D6 dx12_surface=" << layout.dx12Surface.Width() << "x" << layout.dx12Surface.Height() << "\n";
    std::cout << "PASS|ace_aq3d6_ui_render_stability_probe\n";
    return 0;
}
