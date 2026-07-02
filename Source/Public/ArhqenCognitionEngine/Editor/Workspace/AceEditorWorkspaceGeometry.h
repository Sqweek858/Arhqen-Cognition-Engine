#pragma once

#include "ArhqenCognitionEngine/Editor/Workspace/AceEditorWorkspaceLayout.h"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace am::editor
{
    struct WorkspaceRect
    {
        double left = 0.0;
        double top = 0.0;
        double right = 0.0;
        double bottom = 0.0;

        [[nodiscard]] double width() const noexcept { return right - left; }
        [[nodiscard]] double height() const noexcept { return bottom - top; }
        [[nodiscard]] bool empty() const noexcept { return width() <= 0.0 || height() <= 0.0; }
        [[nodiscard]] bool contains(double x, double y) const noexcept
        {
            return x >= left && x < right && y >= top && y < bottom;
        }
    };

    struct WorkspaceGeometryOptions
    {
        double splitterThickness = 1.0;
        double splitterHitThickness = 9.0;
        double tabBarHeight = 28.0;
        double minimumPaneExtent = 64.0;
        double minimumTabWidth = 72.0;
        double maximumTabWidth = 220.0;
    };

    struct WorkspaceNodeGeometry
    {
        std::string nodeId;
        DockNodeKind kind = DockNodeKind::Stack;
        WorkspaceRect bounds;
        WorkspaceRect tabBar;
        WorkspaceRect content;
    };

    struct WorkspaceTabGeometry
    {
        std::string nodeId;
        std::string tabId;
        WorkspaceRect bounds;
        bool active = false;
    };

    struct WorkspaceSplitterGeometry
    {
        std::string splitterId;
        DockOrientation orientation = DockOrientation::Horizontal;
        std::size_t leadingChildIndex = 0;
        std::size_t trailingChildIndex = 0;
        WorkspaceRect visualBounds;
        WorkspaceRect hitBounds;
        WorkspaceRect leadingPaneBounds;
        WorkspaceRect trailingPaneBounds;
    };

    class SolvedWorkspaceGeometry final
    {
    public:
        [[nodiscard]] const std::vector<WorkspaceNodeGeometry>& nodes() const noexcept { return nodes_; }
        [[nodiscard]] const std::vector<WorkspaceTabGeometry>& tabs() const noexcept { return tabs_; }
        [[nodiscard]] const std::vector<WorkspaceSplitterGeometry>& splitters() const noexcept { return splitters_; }
        [[nodiscard]] const WorkspaceNodeGeometry* findNode(std::string_view nodeId) const noexcept;
        [[nodiscard]] const WorkspaceSplitterGeometry* hitTestSplitter(double x, double y) const noexcept;
        [[nodiscard]] const WorkspaceTabGeometry* hitTestTab(double x, double y) const noexcept;

    private:
        friend class EditorWorkspaceGeometrySolver;
        friend struct WorkspaceGeometryBuilderAccess;
        std::vector<WorkspaceNodeGeometry> nodes_;
        std::vector<WorkspaceTabGeometry> tabs_;
        std::vector<WorkspaceSplitterGeometry> splitters_;
    };

    class EditorWorkspaceGeometrySolver final
    {
    public:
        static std::optional<SolvedWorkspaceGeometry> solve(
            const EditorWorkspaceLayout& layout,
            WorkspaceRect bounds,
            WorkspaceGeometryOptions options = {},
            std::string* error = nullptr);
    };
}
