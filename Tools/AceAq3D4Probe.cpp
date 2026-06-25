#include "ArhqenCognitionEngine/AquariumUI/AceEnvironment3DMode.h"

#include <cassert>
#include <iostream>

int main()
{
    ace::aquarium_ui::AceEnvironment3DMode mode;
    ace::aquarium_ui::AceEnvironment3DPanelState state{};
    state.detailsVisible = true;
    state.logsVisible = true;
    state.detailsWidth = 220.0f;
    state.detailsHeight = 620.0f;
    state.logsWidth = 230.0f;
    state.logsHeight = 620.0f;

    const auto layout = mode.Compute(1708.0f, 887.0f, state);

    assert(layout.topOverlay.Height() <= 24.0f);
    assert(layout.leftPanel.Width() > 0.0f);
    assert(layout.rightLogsPanel.Width() > 0.0f);
    assert(layout.leftResizeHandle.Width() > 0.0f);
    assert(layout.rightResizeHandle.Width() > 0.0f);
    assert(layout.viewport.Width() > layout.leftPanel.Width());
    assert(layout.viewport.Area() > layout.leftPanel.Area());
    assert(layout.viewport.Area() > layout.rightLogsPanel.Area());

    ace::aquarium_ui::AceEnvironment3DPanelState resized = state;
    mode.ResizeLeftPanel(resized, 360.0f, 700.0f, 1708.0f, 887.0f);
    assert(resized.detailsWidth > state.detailsWidth);
    assert(resized.detailsHeight > state.detailsHeight);

    mode.ResizeRightPanel(resized, 900.0f, 1000.0f, 1708.0f, 887.0f);
    assert(resized.logsWidth <= 420.0f);
    assert(resized.logsHeight <= 860.0f);

    assert(mode.LogsScrollChangesVisibleRange(80, 220.0f, 0.0f, 120.0f));

    ace::aquarium_ui::AceEnvironment3DPanelState hidden = state;
    hidden.detailsVisible = false;
    hidden.logsVisible = false;
    const auto hiddenLayout = mode.Compute(1708.0f, 887.0f, hidden);
    assert(hiddenLayout.leftPanel.Area() == 0.0f);
    assert(hiddenLayout.rightLogsPanel.Area() == 0.0f);
    assert(mode.ViewportAreaAtLeast70Percent(hiddenLayout));

    std::cout << "ACE-AQ3D4 viewport_area_ratio_hidden=" << mode.ViewportAreaRatio(hiddenLayout) << "\n";
    std::cout << "PASS|ace_aq3d4_layout_probe\n";
    return 0;
}
