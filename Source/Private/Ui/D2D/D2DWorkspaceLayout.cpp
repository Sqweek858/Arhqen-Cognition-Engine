#include "ArhqenCognitionEngine/Ui/D2D/D2DWorkspaceLayout.h"

namespace am::ui
{
    D2DWorkspaceRects D2DWorkspaceLayout::compute(int width, int height, const D2DTheme& theme, const D2DDockLayoutProfile& profile) const
    {
        D2DDockLayoutProfile localProfile = profile;
        localProfile.clamp();

        D2DWorkspaceRects r;
        const auto& m = theme.metrics;

        r.header = makeUiRect(
            m.margin,
            m.margin,
            static_cast<float>(width) - m.margin,
            m.margin + m.headerHeight
        );

        const float contentTop = r.header.bottom + m.gap;
        const float sidebarWidth = localProfile.sidebarCollapsed ? 74.0f : localProfile.sidebarWidth;

        r.sidebar = makeUiRect(
            m.margin,
            contentTop,
            m.margin + sidebarWidth,
            static_cast<float>(height) - m.margin
        );

        r.main = makeUiRect(
            r.sidebar.right + m.gap,
            contentTop,
            static_cast<float>(width) - m.margin,
            static_cast<float>(height) - m.margin
        );

        const float pad = 18.0f;
        const float workspaceWidth = localProfile.workspaceCollapsed
            ? 0.0f
            : std::clamp(static_cast<float>(width) * localProfile.workspaceRatio, 300.0f, 520.0f);

        if (!localProfile.workspaceCollapsed)
        {
            r.tabStrip = makeUiRect(
                r.main.right - pad - workspaceWidth,
                r.main.top + pad,
                r.main.right - pad,
                r.main.top + pad + 42.0f
            );

            const float rightAvailableTop = r.tabStrip.bottom + 10.0f;
            const float rightAvailableBottom = r.main.bottom - pad;
            const float rightAvailableHeight = std::max(220.0f, rightAvailableBottom - rightAvailableTop);
            const float workspaceHeight = std::clamp(rightAvailableHeight * localProfile.inspectorRatio, 230.0f, rightAvailableHeight - 210.0f);

            r.workspace = makeUiRect(
                r.tabStrip.left,
                rightAvailableTop,
                r.tabStrip.right,
                rightAvailableTop + workspaceHeight
            );

            r.inspectorSplitter = makeUiRect(
                r.tabStrip.left,
                r.workspace.bottom + 4.0f,
                r.tabStrip.right,
                r.workspace.bottom + 10.0f
            );

            r.inspector = makeUiRect(
                r.tabStrip.left,
                r.inspectorSplitter.bottom + 2.0f,
                r.tabStrip.right,
                rightAvailableBottom
            );

            r.workspaceSplitter = makeUiRect(
                r.tabStrip.left - 10.0f,
                r.main.top + pad,
                r.tabStrip.left - 4.0f,
                r.main.bottom - pad
            );
        }
        else
        {
            r.tabStrip = makeUiRect(r.main.right - pad, r.main.top + pad, r.main.right - pad, r.main.top + pad);
            r.workspace = r.tabStrip;
            r.inspector = r.tabStrip;
            r.workspaceSplitter = makeUiRect(r.main.right - pad - 8.0f, r.main.top + pad, r.main.right - pad, r.main.bottom - pad);
        }

        const float chatRight = localProfile.workspaceCollapsed ? r.main.right - pad : r.workspaceSplitter.left - m.gap;

        r.toolbar = makeUiRect(
            r.main.left + pad,
            r.main.top + pad,
            chatRight,
            r.main.top + pad + 58.0f
        );

        r.conversation = makeUiRect(
            r.main.left + pad,
            r.toolbar.bottom + 12.0f,
            chatRight,
            r.main.bottom - pad - m.inputHeight - m.statusHeight - m.gap * 1.35f
        );

        r.input = makeUiRect(
            r.conversation.left,
            r.conversation.bottom + m.gap,
            r.conversation.right - 128.0f,
            r.conversation.bottom + m.gap + m.inputHeight
        );

        r.sendButton = makeUiRect(
            r.input.right + 14.0f,
            r.input.top,
            r.conversation.right,
            r.input.bottom
        );

        r.status = makeUiRect(
            r.conversation.left,
            r.input.bottom + 8.0f,
            r.conversation.right,
            r.input.bottom + 8.0f + m.statusHeight
        );

        r.autocomplete = makeUiRect(
            r.input.left,
            r.input.top - 236.0f,
            std::min(r.input.right, r.input.left + 520.0f),
            r.input.top - 10.0f
        );

        const float paletteWidth = std::min(760.0f, static_cast<float>(width) - 120.0f);
        r.commandPalette = centeredOverlay(width, 92.0f, paletteWidth, 456.0f);

        const float diagnosticsWidth = std::min(760.0f, static_cast<float>(width) - 140.0f);
        r.diagnostics = centeredOverlay(width, 104.0f, diagnosticsWidth, 560.0f);

        r.toast = makeUiRect(
            static_cast<float>(width) - 420.0f,
            118.0f,
            static_cast<float>(width) - 28.0f,
            static_cast<float>(height) - 28.0f
        );

        const float helpWidth = std::min(940.0f, static_cast<float>(width) - 140.0f);
        r.shortcutHelp = centeredOverlay(width, 112.0f, helpWidth, 560.0f);

        const float layoutOverlayWidth = std::min(560.0f, static_cast<float>(width) - 120.0f);
        r.layoutOverlay = centeredOverlay(width, 132.0f, layoutOverlayWidth, 380.0f);

        return r;
    }

    D2DLayoutHit D2DWorkspaceLayout::hitTestSplitters(const D2DWorkspaceRects& rects, float x, float y) const
    {
        if (rects.workspaceSplitter.contains(x, y))
        {
            return {D2DSplitterKind::WorkspaceVertical, true};
        }

        if (rects.inspectorSplitter.contains(x, y))
        {
            return {D2DSplitterKind::InspectorHorizontal, true};
        }

        return {};
    }

    float D2DWorkspaceLayout::workspaceRatioFromX(const D2DWorkspaceRects& rects, int windowWidth, float x) const
    {
        const float rightEdge = rects.main.right - 18.0f;
        const float width = std::clamp(rightEdge - x, 280.0f, 560.0f);
        return std::clamp(width / std::max(1.0f, static_cast<float>(windowWidth)), 0.18f, 0.42f);
    }

    float D2DWorkspaceLayout::inspectorRatioFromY(const D2DWorkspaceRects& rects, float y) const
    {
        const float top = rects.tabStrip.bottom + 10.0f;
        const float bottom = rects.main.bottom - 18.0f;
        const float height = std::max(1.0f, bottom - top);
        return std::clamp((y - top) / height, 0.34f, 0.72f);
    }

    UiRect D2DWorkspaceLayout::centeredOverlay(int width, float top, float maxWidth, float height) const
    {
        const float left = (static_cast<float>(width) - maxWidth) * 0.5f;
        return makeUiRect(left, top, left + maxWidth, top + height);
    }
}
