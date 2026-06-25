#pragma once

#include "ArhqenCognitionEngine/Ui/Core/AceUiInvalidationRoot.h"
#include "ArhqenCognitionEngine/Ui/Core/AceUiRetainedLayout.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DDrawCommandBuffer.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DRenderContext.h"

#include <string>
#include <vector>

namespace am::ui
{
    struct D2DUiDebugRect
    {
        std::wstring label;
        UiRect rect{};
        bool hot = false;
    };

    struct D2DUiDebugOverlaySnapshot
    {
        bool visible = false;
        std::wstring title = L"Arhqen UI Debug";
        std::vector<D2DUiDebugRect> rects;
        AceUiLayoutStats layoutStats{};
        AceUiDirtySnapshot dirty{};
        D2DDrawCommandStats drawStats{};
        D2DTextLayoutStats textStats{};
        std::uint64_t paintCount = 0;
        std::uint64_t repaintCount = 0;
        std::wstring hoverId;
        std::wstring focusId;
    };

    class D2DUiDebugOverlay
    {
    public:
        void SetVisible(bool visible);
        void Toggle();
        bool Visible() const;
        void SetSnapshot(D2DUiDebugOverlaySnapshot snapshot);
        void Render(D2DRenderContext& ctx);

    private:
        D2DUiDebugOverlaySnapshot snapshot_{};
    };
}
