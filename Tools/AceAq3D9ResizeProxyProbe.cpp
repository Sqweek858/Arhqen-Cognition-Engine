#include <cassert>
#include <iostream>

namespace
{
    struct EmbeddedViewportProxyModel
    {
        bool visible = true;
        bool resizeApplySuspended = false;
        bool pendingResize = false;
        int width = 1280;
        int height = 720;
        int pendingWidth = 0;
        int pendingHeight = 0;
        int hideCount = 0;
        int showCount = 0;
        int childMoveDuringLiveResizeCount = 0;
        int rendererRecreateDuringLiveResizeCount = 0;
        int rendererRecreateCount = 0;

        void hide()
        {
            if (visible)
            {
                ++hideCount;
            }
            visible = false;
        }

        void show()
        {
            if (!visible)
            {
                ++showCount;
            }
            visible = true;
        }

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

        void applyPendingResizeIfNeeded()
        {
            if (!pendingResize || resizeApplySuspended)
            {
                return;
            }

            pendingResize = false;
            width = pendingWidth;
            height = pendingHeight;
            ++rendererRecreateCount;
        }
    };

    struct ResizeProxyTransactionModel
    {
        bool liveResizeActive = false;
        bool hiddenForLiveResize = false;
        bool wasVisibleBeforeLiveResize = false;
        bool pendingResizeAfterLiveDrag = false;
        int pendingW = 0;
        int pendingH = 0;
        int d2dResizeDuringLiveResizeCount = 0;
        int proxyPaintCount = 0;
        int childSyncDuringLiveResizeCount = 0;
        EmbeddedViewportProxyModel viewport{};

        void beginLiveResize()
        {
            liveResizeActive = true;
            viewport.setResizeApplySuspended(true);
            wasVisibleBeforeLiveResize = viewport.visible;
            hiddenForLiveResize = false;
            if (wasVisibleBeforeLiveResize)
            {
                viewport.hide();
                hiddenForLiveResize = true;
            }
        }

        void onWmSize(int w, int h)
        {
            if (liveResizeActive)
            {
                pendingResizeAfterLiveDrag = true;
                pendingW = w;
                pendingH = h;
                ++d2dResizeDuringLiveResizeCount;
                paintProxy();
                return;
            }

            viewport.queueResize(w, h);
        }

        void syncChildHwnd()
        {
            if (liveResizeActive)
            {
                ++childSyncDuringLiveResizeCount;
                return;
            }

            viewport.show();
            hiddenForLiveResize = false;
            wasVisibleBeforeLiveResize = false;
        }

        void paintProxy()
        {
            if (liveResizeActive || hiddenForLiveResize)
            {
                ++proxyPaintCount;
            }
        }

        void tickRenderFrame()
        {
            if (liveResizeActive)
            {
                return;
            }

            viewport.applyPendingResizeIfNeeded();
        }

        void endLiveResize()
        {
            liveResizeActive = false;
            viewport.setResizeApplySuspended(false);
            if (pendingResizeAfterLiveDrag)
            {
                pendingResizeAfterLiveDrag = false;
                viewport.queueResize(pendingW, pendingH);
            }
            paintProxy();
        }
    };
}

int main()
{
    ResizeProxyTransactionModel tx;

    tx.beginLiveResize();
    assert(tx.liveResizeActive);
    assert(tx.hiddenForLiveResize);
    assert(!tx.viewport.visible);
    assert(tx.viewport.hideCount == 1);
    assert(tx.viewport.resizeApplySuspended);

    for (int i = 0; i < 12; ++i)
    {
        tx.onWmSize(1300 + i * 8, 740 + i * 5);
        tx.syncChildHwnd();
        tx.tickRenderFrame();
    }

    assert(tx.d2dResizeDuringLiveResizeCount == 12);
    assert(tx.proxyPaintCount >= 12);
    assert(tx.childSyncDuringLiveResizeCount == 12);
    assert(tx.viewport.hideCount == 1);
    assert(tx.viewport.showCount == 0);
    assert(tx.viewport.rendererRecreateDuringLiveResizeCount == 0);
    assert(tx.viewport.childMoveDuringLiveResizeCount == 0);
    assert(tx.viewport.rendererRecreateCount == 0);

    tx.endLiveResize();
    assert(!tx.liveResizeActive);
    assert(tx.hiddenForLiveResize);
    assert(!tx.viewport.visible);
    assert(tx.proxyPaintCount >= 13);

    tx.syncChildHwnd();
    assert(tx.viewport.visible);
    assert(!tx.hiddenForLiveResize);
    assert(tx.viewport.showCount == 1);

    tx.tickRenderFrame();
    assert(tx.viewport.rendererRecreateCount == 1);
    assert(tx.viewport.width == 1300 + 11 * 8);
    assert(tx.viewport.height == 740 + 11 * 5);

    std::cout << "PASS|ace_aq3d9_resize_proxy_probe\n";
    return 0;
}
