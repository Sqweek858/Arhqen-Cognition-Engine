#include <cassert>
#include <iostream>

namespace
{
    struct EmbeddedViewportModel
    {
        bool resizeApplySuspended = false;
        bool pendingResize = false;
        int pendingWidth = 0;
        int pendingHeight = 0;
        int width = 1280;
        int height = 720;
        int childMoveCount = 0;
        int childResizeCount = 0;
        int rendererRecreateCount = 0;
        int childMoveDuringLiveResizeCount = 0;
        int rendererRecreateDuringLiveResizeCount = 0;

        void setResizeApplySuspended(bool suspended)
        {
            resizeApplySuspended = suspended;
        }

        void queueResize(int w, int h)
        {
            pendingResize = true;
            pendingWidth = w;
            pendingHeight = h;
        }

        void syncChildWindow(int w, int h)
        {
            if (w == width && h == height)
            {
                return;
            }

            if (resizeApplySuspended)
            {
                ++childMoveDuringLiveResizeCount;
            }

            ++childMoveCount;
            queueResize(w, h);
        }

        void applyPendingResizeIfNeeded()
        {
            if (!pendingResize)
            {
                return;
            }

            if (resizeApplySuspended)
            {
                return;
            }

            pendingResize = false;
            width = pendingWidth;
            height = pendingHeight;
            ++childResizeCount;
            ++rendererRecreateCount;
        }
    };

    struct LiveResizeTransactionModel
    {
        bool windowLiveResizeActive = false;
        bool pendingResizeAfterLiveDrag = false;
        int pendingLiveResizeWidth = 0;
        int pendingLiveResizeHeight = 0;
        int layoutPassCount = 0;
        int childSyncCount = 0;
        int liveResizeEnterCount = 0;
        int liveResizeExitCount = 0;
        int liveResizeDeferredSizeCount = 0;
        int liveResizeAppliedFinalCount = 0;
        EmbeddedViewportModel viewport{};

        void beginLiveResize()
        {
            windowLiveResizeActive = true;
            pendingResizeAfterLiveDrag = false;
            viewport.setResizeApplySuspended(true);
            ++liveResizeEnterCount;
        }

        void onWmSize(int w, int h)
        {
            if (windowLiveResizeActive)
            {
                pendingResizeAfterLiveDrag = true;
                pendingLiveResizeWidth = w;
                pendingLiveResizeHeight = h;
                ++liveResizeDeferredSizeCount;
                return;
            }

            ++layoutPassCount;
            viewport.syncChildWindow(w, h);
            ++childSyncCount;
        }

        void tickRenderFrame()
        {
            if (windowLiveResizeActive)
            {
                viewport.setResizeApplySuspended(true);
                return;
            }

            viewport.applyPendingResizeIfNeeded();
        }

        void endLiveResize()
        {
            windowLiveResizeActive = false;
            viewport.setResizeApplySuspended(false);
            ++liveResizeExitCount;

            if (pendingResizeAfterLiveDrag)
            {
                pendingResizeAfterLiveDrag = false;
                ++layoutPassCount;
                ++liveResizeAppliedFinalCount;
                viewport.syncChildWindow(pendingLiveResizeWidth, pendingLiveResizeHeight);
                ++childSyncCount;
            }
        }
    };
}

int main()
{
    LiveResizeTransactionModel tx;

    tx.beginLiveResize();
    assert(tx.windowLiveResizeActive);
    assert(tx.viewport.resizeApplySuspended);
    assert(tx.liveResizeEnterCount == 1);

    for (int i = 0; i < 20; ++i)
    {
        tx.onWmSize(1280 + i * 9, 720 + i * 4);
        tx.tickRenderFrame();
    }

    assert(tx.liveResizeDeferredSizeCount == 20);
    assert(tx.layoutPassCount == 0);
    assert(tx.childSyncCount == 0);
    assert(tx.viewport.childMoveDuringLiveResizeCount == 0);
    assert(tx.viewport.rendererRecreateDuringLiveResizeCount == 0);
    assert(tx.viewport.childResizeCount == 0);
    assert(tx.viewport.rendererRecreateCount == 0);
    assert(tx.viewport.width == 1280);
    assert(tx.viewport.height == 720);

    tx.endLiveResize();
    assert(!tx.windowLiveResizeActive);
    assert(!tx.viewport.resizeApplySuspended);
    assert(tx.liveResizeExitCount == 1);
    assert(tx.liveResizeAppliedFinalCount == 1);
    assert(tx.layoutPassCount == 1);
    assert(tx.childSyncCount == 1);
    assert(tx.viewport.pendingResize);

    tx.tickRenderFrame();
    assert(!tx.viewport.pendingResize);
    assert(tx.viewport.childResizeCount == 1);
    assert(tx.viewport.rendererRecreateCount == 1);
    assert(tx.viewport.width == 1280 + 19 * 9);
    assert(tx.viewport.height == 720 + 19 * 4);

    const int recreateAfterFinal = tx.viewport.rendererRecreateCount;
    tx.onWmSize(tx.viewport.width, tx.viewport.height);
    tx.tickRenderFrame();
    assert(tx.viewport.rendererRecreateCount == recreateAfterFinal);

    std::cout << "PASS|ace_aq3d8_resize_transaction_probe\n";
    return 0;
}
