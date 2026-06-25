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
    ace::aquarium_ui::AceEnvironment3DPanelState state{};
    state.detailsVisible = true;
    state.logsVisible = true;
    state.detailsWidth = 288.0f;
    state.detailsHeight = 790.0f;
    state.logsWidth = 292.0f;
    state.logsHeight = 760.0f;

    const auto layout = mode.Compute(1708.0f, 887.0f, state);

    assert(layout.topOverlay.Height() <= 24.0f);
    assert(!intersects(layout.topbarLeftCluster, layout.topbarCenterCluster));
    assert(!intersects(layout.topbarCenterCluster, layout.topbarRightCluster));
    assert(inside(layout.topbarStatus, layout.topbarCenterCluster));
    assert(inside(layout.debugTruthWarning, layout.topbarRightCluster));

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

    assert(layout.leftInspectorSection.top >= layout.leftManualActionsSection.bottom);
    assert(!intersects(layout.leftResizeHandle, layout.leftContentClip));
    assert(!intersects(layout.rightResizeHandle, layout.rightLogsContent));
    assert(inside(layout.leftRuntimeSection, layout.leftContentClip));
    assert(inside(layout.leftManualActionsSection, layout.leftContentClip));
    assert(inside(layout.rightLogsViewport, layout.rightLogsContent));

    assert(mode.LogsScrollChangesVisibleRange(120, layout.rightLogsViewport.Height(), 0.0f, 160.0f));
    assert(mode.LogsScrollOffsetClamped(120, layout.rightLogsViewport.Height(), 99999.0f));

    ace::aquarium_ui::AceEnvironment3DPanelState resized = state;
    mode.ResizeLeftPanel(resized, state.detailsWidth + 80.0f, state.detailsHeight + 40.0f, 1708.0f, 887.0f);
    assert(resized.detailsWidth > state.detailsWidth);
    assert(resized.detailsHeight > state.detailsHeight);
    mode.ResizeRightPanel(resized, state.logsWidth + 80.0f, state.logsHeight + 40.0f, 1708.0f, 887.0f);
    assert(resized.logsWidth > state.logsWidth);
    assert(resized.logsHeight > state.logsHeight);
    mode.ResizeLeftPanel(resized, 10.0f, 10.0f, 1708.0f, 887.0f);
    assert(resized.detailsWidth >= 264.0f);
    assert(resized.detailsHeight >= 610.0f);

    assert(mode.RectsStableAcrossIdleFrames(1708.0f, 887.0f, state));
    const int before = mode.LayoutPassCount();
    const auto again = mode.Compute(1708.0f, 887.0f, state);
    const int after = mode.LayoutPassCount();
    assert(after == before + 1);
    assert(std::abs(again.dx12Surface.left - mode.LastViewportRect().left) < 0.001f);

    assert(mode.ViewportAreaAtLeast70Percent(layout));
    assert(layout.dx12Surface.Width() > 640.0f);
    assert(layout.dx12Surface.Height() > 640.0f);
    assert(layout.viewport.Area() > layout.leftPanel.Area());
    assert(layout.viewport.Area() > layout.rightLogsPanel.Area());

    ace::aquarium_ui::AceEnvironment3DPanelState hidden = state;
    hidden.detailsVisible = false;
    hidden.logsVisible = false;
    const auto hiddenLayout = mode.Compute(1708.0f, 887.0f, hidden);
    assert(hiddenLayout.leftPanel.Empty());
    assert(hiddenLayout.rightLogsPanel.Empty());
    assert(mode.ViewportAreaAtLeast70Percent(hiddenLayout));

    std::cout << "ACE-AQ3D5 viewport_area_ratio=" << mode.ViewportAreaRatio(layout) << "\n";
    std::cout << "ACE-AQ3D5 dx12_surface=" << layout.dx12Surface.Width() << "x" << layout.dx12Surface.Height() << "\n";
    std::cout << "PASS|ace_aq3d5_layout_determinism_probe\n";
    return 0;
}
