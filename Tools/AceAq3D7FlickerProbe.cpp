#include <cassert>
#include <iostream>

namespace
{
    struct Rect
    {
        float left = 0.0f;
        float top = 0.0f;
        float right = 0.0f;
        float bottom = 0.0f;

        bool empty() const { return right <= left || bottom <= top; }
    };

    struct FlickerCounters
    {
        int parentPaintCount = 0;
        int fullInvalidationCount = 0;
        int partialInvalidationCount = 0;
        int hoverInvalidationCount = 0;
        int childShowCount = 0;
        int childHideCount = 0;
        int childMoveCount = 0;
        int childResizeCount = 0;
        int rendererRecreateCount = 0;
    };

    struct DirtyInvalidationModel
    {
        FlickerCounters counters{};
        Rect lastDirty{};

        void fullInvalidate()
        {
            ++counters.fullInvalidationCount;
        }

        void partialInvalidate(Rect rect)
        {
            assert(!rect.empty());
            lastDirty = rect;
            ++counters.partialInvalidationCount;
        }

        void hoverChange(Rect oldHot, Rect newHot)
        {
            ++counters.hoverInvalidationCount;
            if (!oldHot.empty()) { partialInvalidate(oldHot); }
            if (!newHot.empty()) { partialInvalidate(newHot); }
        }
    };

    struct StableChildViewportModel
    {
        FlickerCounters counters{};
        bool visible = false;
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
        bool pendingResize = false;
        int pendingWidth = 0;
        int pendingHeight = 0;

        void sync(int nextX, int nextY, int nextW, int nextH)
        {
            if (!visible)
            {
                visible = true;
                ++counters.childShowCount;
            }

            if (x != nextX || y != nextY || width != nextW || height != nextH)
            {
                x = nextX;
                y = nextY;
                width = nextW;
                height = nextH;
                ++counters.childMoveCount;
                queueResize(nextW, nextH);
            }
        }

        void queueResize(int nextW, int nextH)
        {
            pendingResize = true;
            pendingWidth = nextW;
            pendingHeight = nextH;
        }

        void applyPendingResize()
        {
            if (!pendingResize) { return; }
            pendingResize = false;
            width = pendingWidth;
            height = pendingHeight;
            ++counters.childResizeCount;
            ++counters.rendererRecreateCount;
        }
    };
}

int main()
{
    DirtyInvalidationModel ui;
    ui.hoverChange({10, 10, 90, 36}, {110, 10, 190, 36});
    assert(ui.counters.hoverInvalidationCount == 1);
    assert(ui.counters.partialInvalidationCount == 2);
    assert(ui.counters.fullInvalidationCount == 0);

    StableChildViewportModel child;
    child.sync(10, 24, 1200, 800);
    assert(child.counters.childShowCount == 1);
    assert(child.counters.childMoveCount == 1);
    child.applyPendingResize();
    assert(child.counters.childResizeCount == 1);
    assert(child.counters.rendererRecreateCount == 1);

    const auto beforeResize = child.counters.childResizeCount;
    const auto beforeRecreate = child.counters.rendererRecreateCount;
    ui.hoverChange({110, 10, 190, 36}, {220, 10, 300, 36});
    assert(ui.counters.fullInvalidationCount == 0);
    assert(child.counters.childResizeCount == beforeResize);
    assert(child.counters.rendererRecreateCount == beforeRecreate);

    // Syncing the same stable rect is a no-op for move/resize/recreate.
    const auto beforeMove = child.counters.childMoveCount;
    child.sync(10, 24, 1200, 800);
    child.applyPendingResize();
    assert(child.counters.childMoveCount == beforeMove);
    assert(child.counters.childResizeCount == beforeResize);
    assert(child.counters.rendererRecreateCount == beforeRecreate);

    std::cout << "PASS|ace_aq3d7_flicker_probe\n";
    return 0;
}
