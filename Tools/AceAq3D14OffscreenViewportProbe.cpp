#include <cstdint>
#include <iostream>

struct CompositeViewportModel
{
    bool singleHwndCompositeEnabled = true;
    bool legacyChildVisible = false;
    bool liveResizeActive = false;
    bool viewportFrameValid = false;
    std::uint64_t compositeFrames = 0;
    std::uint64_t cachedResizeFrames = 0;
    std::uint64_t legacyChildSuppressed = 0;
    std::uint64_t childMoves = 0;
    std::uint64_t childShows = 0;

    void renderFrame()
    {
        if (singleHwndCompositeEnabled)
        {
            if (legacyChildVisible)
            {
                legacyChildVisible = false;
                ++legacyChildSuppressed;
            }
            viewportFrameValid = true;
            ++compositeFrames;
            if (liveResizeActive)
            {
                ++cachedResizeFrames;
            }
            return;
        }

        ++childShows;
    }

    void liveResizeTick()
    {
        liveResizeActive = true;
        renderFrame();
        liveResizeActive = false;
    }
};

int main()
{
    CompositeViewportModel model;
    model.legacyChildVisible = true;
    model.renderFrame();

    if (model.legacyChildVisible || model.legacyChildSuppressed != 1)
    {
        std::cerr << "FAIL|legacy_child_not_suppressed\n";
        return 1;
    }

    model.liveResizeTick();

    if (!model.viewportFrameValid || model.compositeFrames != 2 || model.cachedResizeFrames != 1)
    {
        std::cerr << "FAIL|composite_resize_frame_not_cached\n";
        return 1;
    }

    if (model.childMoves != 0 || model.childShows != 0)
    {
        std::cerr << "FAIL|child_hwnd_used_in_composite_path\n";
        return 1;
    }

    std::cout << "PASS|ace_aq3d14_offscreen_viewport_probe\n";
    return 0;
}
