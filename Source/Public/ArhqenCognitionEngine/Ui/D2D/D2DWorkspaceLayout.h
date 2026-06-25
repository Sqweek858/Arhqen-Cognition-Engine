#pragma once

#include "ArhqenCognitionEngine/Ui/D2D/D2DLayoutProfile.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DTheme.h"

namespace am::ui
{
    class D2DWorkspaceLayout
    {
    public:
        D2DWorkspaceRects compute(int width, int height, const D2DTheme& theme, const D2DDockLayoutProfile& profile) const;
        D2DLayoutHit hitTestSplitters(const D2DWorkspaceRects& rects, float x, float y) const;

        float workspaceRatioFromX(const D2DWorkspaceRects& rects, int windowWidth, float x) const;
        float inspectorRatioFromY(const D2DWorkspaceRects& rects, float y) const;

    private:
        UiRect centeredOverlay(int width, float top, float maxWidth, float height) const;
    };
}
