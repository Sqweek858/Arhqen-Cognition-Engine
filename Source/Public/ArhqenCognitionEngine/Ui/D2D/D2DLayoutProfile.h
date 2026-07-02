#pragma once

#include "ArhqenCognitionEngine/Ui/D2D/D2DUiTypes.h"

#include <algorithm>
#include <string>
#include <vector>

namespace am::ui
{
    struct D2DDockLayoutProfile
    {
        float sidebarWidth = 314.0f;
        float workspaceRatio = 0.255f;
        float inspectorRatio = 0.54f;
        bool sidebarCollapsed = false;
        bool workspaceCollapsed = false;
        bool inspectorCollapsed = false;
        bool diagnosticsPinned = false;
        std::wstring activeWorkspaceTab = L"snapshot";
        float aquariumDetailsWidth = 430.0f;
        float aquariumDetailsHeight = 850.0f;
        float aquariumLogsWidth = 370.0f;
        float aquariumLogsHeight = 850.0f;
        bool aquariumDetailsVisible = true;
        bool aquariumLogsVisible = true;

        void clamp()
        {
            sidebarWidth = std::clamp(sidebarWidth, 72.0f, 420.0f);
            workspaceRatio = std::clamp(workspaceRatio, 0.18f, 0.42f);
            inspectorRatio = std::clamp(inspectorRatio, 0.34f, 0.72f);
            aquariumDetailsWidth = std::max(120.0f, aquariumDetailsWidth);
            aquariumDetailsHeight = std::max(160.0f, aquariumDetailsHeight);
            aquariumLogsWidth = std::max(120.0f, aquariumLogsWidth);
            aquariumLogsHeight = std::max(160.0f, aquariumLogsHeight);
        }

        static D2DDockLayoutProfile defaults()
        {
            D2DDockLayoutProfile profile;
            profile.clamp();
            return profile;
        }
    };

    struct D2DWorkspaceRects
    {
        UiRect header;
        UiRect sidebar;
        UiRect main;
        UiRect toolbar;
        UiRect conversation;
        UiRect input;
        UiRect sendButton;
        UiRect status;
        UiRect tabStrip;
        UiRect workspace;
        UiRect inspector;
        UiRect workspaceSplitter;
        UiRect inspectorSplitter;
        UiRect autocomplete;
        UiRect commandPalette;
        UiRect diagnostics;
        UiRect toast;
        UiRect shortcutHelp;
        UiRect layoutOverlay;
    };

    enum class D2DSplitterKind
    {
        None,
        WorkspaceVertical,
        InspectorHorizontal
    };

    struct D2DLayoutHit
    {
        D2DSplitterKind splitter = D2DSplitterKind::None;
        bool hit = false;
    };
}
