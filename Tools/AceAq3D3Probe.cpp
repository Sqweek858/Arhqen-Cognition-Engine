#include "ArhqenCognitionEngine/AquariumUI/AceEnvironment3DMode.h"

#include <cassert>
#include <iostream>

int main()
{
    ace::aquarium_ui::AceEnvironment3DMode mode;
    const auto layout = mode.Compute(1708.0f, 887.0f);
    const float ratio = mode.ViewportAreaRatio(layout);

    assert(layout.viewport.Width() > 0.0f);
    assert(layout.viewport.Height() > 0.0f);
    assert(layout.viewport.top >= layout.topOverlay.bottom);
    assert(layout.bottomLogStrip.top >= layout.viewport.bottom);
    assert(mode.ViewportAreaAtLeast70Percent(layout));
    assert(ratio >= 0.70f);

    std::cout << "ACE-AQ3D3 viewport_area_ratio=" << ratio << "\n";
    std::cout << "PASS|ace_aq3d3_layout_probe\n";
    return 0;
}
