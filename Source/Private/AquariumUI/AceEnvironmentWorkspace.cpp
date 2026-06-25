#include "ArhqenCognitionEngine/AquariumUI/AceEnvironmentWorkspace.h"

#include <algorithm>

namespace ace::aquarium_ui
{
    AceEnvironmentWorkspaceLayout AceEnvironmentWorkspace::Compute(float clientWidth, float clientHeight, float appBarHeight) const
    {
        const float width = std::max(920.0f, clientWidth);
        const float height = std::max(620.0f, clientHeight);
        const float top = std::clamp(appBarHeight, 0.0f, height * 0.20f);
        const float pad = 18.0f;
        const float toolbarH = 88.0f;
        const float logsH = std::clamp(height * 0.16f, 112.0f, 146.0f);
        const float inspectorW = std::clamp(width * 0.25f, 300.0f, 430.0f);
        const float gap = 12.0f;

        AceEnvironmentWorkspaceLayout out{};
        out.workspace = {0.0f, top, width, height};
        out.toolbar = {pad, top + 12.0f, width - pad, top + 12.0f + toolbarH};
        out.logs = {pad, height - logsH - pad, width - pad, height - pad};
        out.inspector = {width - pad - inspectorW, out.toolbar.bottom + gap, width - pad, out.logs.top - gap};
        out.viewport = {pad, out.toolbar.bottom + gap, out.inspector.left - gap, out.logs.top - gap};

        return out;
    }

    bool AceEnvironmentWorkspace::ViewportIsLargeEnough(const AceEnvironmentWorkspaceLayout& layout, float clientWidth, float clientHeight) const
    {
        const float absoluteMinW = 800.0f;
        const float absoluteMinH = 450.0f;
        const float relativeMinW = std::max(1.0f, clientWidth) * 0.60f;
        const float relativeMinH = std::max(1.0f, clientHeight) * 0.55f;

        const float minW = clientWidth >= 1280.0f ? absoluteMinW : relativeMinW;
        const float minH = clientHeight >= 760.0f ? absoluteMinH : relativeMinH;

        return layout.viewport.Width() >= minW && layout.viewport.Height() >= minH;
    }
}
