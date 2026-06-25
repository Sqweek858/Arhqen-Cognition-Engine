#include "ArhqenCognitionEngine/AquariumUI/AceEnvironment3DMode.h"

#include <iostream>

int main()
{
    using ace::aquarium_ui::AceEnvironment3DMode;
    using ace::aquarium_ui::AceEnvironment3DPanelState;

    const auto& c = AceEnvironment3DMode::Constants();
    bool ok = true;

    ok = ok && c.LogicalScale >= 1.10f && c.LogicalScale <= 1.25f;
    ok = ok && c.TopbarHeight >= 28.0f;
    ok = ok && c.ButtonHeight >= 30.0f;
    ok = ok && c.MinPanelWidth >= 320.0f;
    ok = ok && c.PanelInset >= 12.0f;

    AceEnvironment3DMode mode;
    AceEnvironment3DPanelState state{};
    state.detailsVisible = true;
    state.logsVisible = true;
    const auto layout = mode.Compute(1708.0f, 887.0f, state);

    ok = ok && layout.leftPanel.Width() >= 326.0f;
    ok = ok && layout.rightLogsPanel.Width() >= 326.0f;
    ok = ok && layout.topOverlay.Height() >= 28.0f;
    ok = ok && layout.dx12Surface.Width() > 600.0f;
    ok = ok && layout.dx12Surface.Height() > 780.0f;
    ok = ok && mode.ViewportAreaAtLeast70Percent(layout);

    if (!ok)
    {
        std::cerr << "FAIL|ace_ui3g_logical_scale_probe\n";
        return 1;
    }

    std::cout << "PASS|ace_ui3g_logical_scale_probe\n";
    return 0;
}
