#pragma once

#include "ArhqenCognitionEngine/AquariumUI/AceAquariumUiSnapshot.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DRenderContext.h"

#include <cstdint>

namespace am::ui
{
    struct D2DAquariumTelemetryStats
    {
        std::uint64_t renderCount = 0;
        std::uint64_t barCount = 0;
        std::uint64_t timelineCount = 0;
    };

    class D2DAquariumTelemetryWidgets
    {
    public:
        void RenderOverlay(D2DRenderContext& ctx, UiRect viewportRect, const ace::aquarium_ui::AceAquariumUiSnapshot& snapshot);
        void RenderPanel(D2DRenderContext& ctx, UiRect rect, const ace::aquarium_ui::AceAquariumUiSnapshot& snapshot);
        D2DAquariumTelemetryStats Stats() const;

    private:
        void RenderBar(D2DRenderContext& ctx, UiRect rect, const std::wstring& label, double value, ID2D1Brush* accent);
        void RenderMiniTimeline(D2DRenderContext& ctx, UiRect rect, const ace::aquarium_ui::AceAquariumUiSnapshot& snapshot);

        D2DAquariumTelemetryStats stats_{};
    };
}
