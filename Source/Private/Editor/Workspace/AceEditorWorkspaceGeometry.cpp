#include "ArhqenCognitionEngine/Editor/Workspace/AceEditorWorkspaceGeometry.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <utility>

namespace am::editor
{
    struct WorkspaceGeometryBuilderAccess
    {
        static std::vector<WorkspaceNodeGeometry>& nodes(SolvedWorkspaceGeometry& geometry) { return geometry.nodes_; }
        static std::vector<WorkspaceTabGeometry>& tabs(SolvedWorkspaceGeometry& geometry) { return geometry.tabs_; }
        static std::vector<WorkspaceSplitterGeometry>& splitters(SolvedWorkspaceGeometry& geometry) { return geometry.splitters_; }
    };

    namespace
    {
        void fail(std::string* error, std::string message)
        {
            if (error) *error = std::move(message);
        }

        bool finiteRect(const WorkspaceRect& rect)
        {
            return std::isfinite(rect.left) && std::isfinite(rect.top) &&
                std::isfinite(rect.right) && std::isfinite(rect.bottom);
        }

        bool nodeVisible(const EditorDockNode& node)
        {
            if (node.kind == DockNodeKind::Stack)
            {
                return std::any_of(node.tabs.begin(), node.tabs.end(), [](const EditorDockTab& tab) { return tab.visible; });
            }
            return std::any_of(node.children.begin(), node.children.end(), [](const EditorDockNode& child) { return nodeVisible(child); });
        }

        std::vector<double> allocateExtents(
            const EditorDockNode& node,
            const std::vector<std::size_t>& visibleIndices,
            double available,
            double minimum)
        {
            const std::size_t count = visibleIndices.size();
            std::vector<double> extents(count, 0.0);
            if (count == 0 || available <= 0.0) return extents;
            if (available < minimum * static_cast<double>(count))
            {
                std::fill(extents.begin(), extents.end(), available / static_cast<double>(count));
                return extents;
            }

            std::vector<bool> fixed(count, false);
            double remaining = available;
            std::size_t remainingCount = count;
            while (remainingCount > 0)
            {
                double weightTotal = 0.0;
                for (std::size_t index = 0; index < count; ++index)
                    if (!fixed[index]) weightTotal += node.children[visibleIndices[index]].sizeCoefficient;

                bool fixedAnother = false;
                for (std::size_t index = 0; index < count; ++index)
                {
                    if (fixed[index]) continue;
                    const double weight = node.children[visibleIndices[index]].sizeCoefficient;
                    const double proposed = weightTotal > 0.0
                        ? remaining * (weight / weightTotal)
                        : remaining / static_cast<double>(remainingCount);
                    if (proposed < minimum)
                    {
                        extents[index] = minimum;
                        fixed[index] = true;
                        remaining -= minimum;
                        --remainingCount;
                        fixedAnother = true;
                    }
                }
                if (fixedAnother) continue;

                for (std::size_t index = 0; index < count; ++index)
                {
                    if (fixed[index]) continue;
                    const double weight = node.children[visibleIndices[index]].sizeCoefficient;
                    extents[index] = weightTotal > 0.0
                        ? remaining * (weight / weightTotal)
                        : remaining / static_cast<double>(remainingCount);
                }
                break;
            }
            return extents;
        }

        WorkspaceRect expandedHitRect(WorkspaceRect visual, DockOrientation orientation, double hitThickness, WorkspaceRect parent)
        {
            const double half = std::max(0.0, hitThickness -
                (orientation == DockOrientation::Horizontal ? visual.width() : visual.height())) * 0.5;
            if (orientation == DockOrientation::Horizontal)
            {
                visual.left = std::max(parent.left, visual.left - half);
                visual.right = std::min(parent.right, visual.right + half);
            }
            else
            {
                visual.top = std::max(parent.top, visual.top - half);
                visual.bottom = std::min(parent.bottom, visual.bottom + half);
            }
            return visual;
        }

        bool solveNode(
            const EditorDockNode& node,
            WorkspaceRect bounds,
            const WorkspaceGeometryOptions& options,
            SolvedWorkspaceGeometry& output,
            std::string* error)
        {
            static_cast<void>(error);
            WorkspaceNodeGeometry nodeGeometry;
            nodeGeometry.nodeId = node.id;
            nodeGeometry.kind = node.kind;
            nodeGeometry.bounds = bounds;

            if (node.kind == DockNodeKind::Stack)
            {
                const double barBottom = std::min(bounds.bottom, bounds.top + options.tabBarHeight);
                nodeGeometry.tabBar = {bounds.left, bounds.top, bounds.right, barBottom};
                nodeGeometry.content = {bounds.left, barBottom, bounds.right, bounds.bottom};
                WorkspaceGeometryBuilderAccess::nodes(output).push_back(nodeGeometry);

                std::vector<const EditorDockTab*> visibleTabs;
                for (const auto& tab : node.tabs) if (tab.visible) visibleTabs.push_back(&tab);
                if (visibleTabs.empty()) return true;
                const double available = std::max(0.0, nodeGeometry.tabBar.width());
                const double equalWidth = available / static_cast<double>(visibleTabs.size());
                const double desiredWidth = available >= options.minimumTabWidth * static_cast<double>(visibleTabs.size())
                    ? std::clamp(equalWidth, options.minimumTabWidth, options.maximumTabWidth)
                    : equalWidth;
                double cursor = nodeGeometry.tabBar.left;
                for (std::size_t index = 0; index < visibleTabs.size(); ++index)
                {
                    const double right = std::min(nodeGeometry.tabBar.right, cursor + desiredWidth);
                    WorkspaceGeometryBuilderAccess::tabs(output).push_back({node.id, visibleTabs[index]->id,
                        {cursor, nodeGeometry.tabBar.top, right, nodeGeometry.tabBar.bottom},
                        visibleTabs[index]->id == node.activeTab});
                    cursor = right;
                    if (cursor >= nodeGeometry.tabBar.right) break;
                }
                return true;
            }

            WorkspaceGeometryBuilderAccess::nodes(output).push_back(nodeGeometry);
            std::vector<std::size_t> visibleIndices;
            for (std::size_t index = 0; index < node.children.size(); ++index)
                if (nodeVisible(node.children[index])) visibleIndices.push_back(index);
            if (visibleIndices.empty()) return true;
            if (visibleIndices.size() == 1)
                return solveNode(node.children[visibleIndices.front()], bounds, options, output, error);

            const double axisExtent = node.orientation == DockOrientation::Horizontal ? bounds.width() : bounds.height();
            const double splitterThickness = std::min(
                options.splitterThickness,
                axisExtent / static_cast<double>(visibleIndices.size() - 1));
            const double totalSplitterExtent = splitterThickness * static_cast<double>(visibleIndices.size() - 1);
            const double available = std::max(0.0, axisExtent - totalSplitterExtent);
            const auto extents = allocateExtents(node, visibleIndices, available, options.minimumPaneExtent);
            double cursor = node.orientation == DockOrientation::Horizontal ? bounds.left : bounds.top;
            std::vector<WorkspaceRect> childBounds;
            childBounds.reserve(visibleIndices.size());

            for (std::size_t visibleIndex = 0; visibleIndex < visibleIndices.size(); ++visibleIndex)
            {
                const double end = visibleIndex + 1 == visibleIndices.size()
                    ? (node.orientation == DockOrientation::Horizontal ? bounds.right : bounds.bottom)
                    : cursor + extents[visibleIndex];
                WorkspaceRect child = bounds;
                if (node.orientation == DockOrientation::Horizontal) { child.left = cursor; child.right = end; }
                else { child.top = cursor; child.bottom = end; }
                childBounds.push_back(child);

                if (visibleIndex + 1 < visibleIndices.size())
                {
                    cursor = end + splitterThickness;
                }
            }

            for (std::size_t visibleIndex = 0; visibleIndex < visibleIndices.size(); ++visibleIndex)
            {
                if (!solveNode(node.children[visibleIndices[visibleIndex]], childBounds[visibleIndex], options, output, error)) return false;
                if (visibleIndex + 1 >= visibleIndices.size()) continue;

                WorkspaceRect visual = bounds;
                if (node.orientation == DockOrientation::Horizontal)
                {
                    visual.left = childBounds[visibleIndex].right;
                    visual.right = visual.left + splitterThickness;
                }
                else
                {
                    visual.top = childBounds[visibleIndex].bottom;
                    visual.bottom = visual.top + splitterThickness;
                }
                WorkspaceGeometryBuilderAccess::splitters(output).push_back({
                    node.id,
                    node.orientation,
                    visibleIndices[visibleIndex],
                    visibleIndices[visibleIndex + 1],
                    visual,
                    expandedHitRect(visual, node.orientation, options.splitterHitThickness, bounds),
                    childBounds[visibleIndex],
                    childBounds[visibleIndex + 1]});
            }
            return true;
        }
    }

    const WorkspaceNodeGeometry* SolvedWorkspaceGeometry::findNode(std::string_view nodeId) const noexcept
    {
        const auto found = std::find_if(nodes_.begin(), nodes_.end(), [nodeId](const WorkspaceNodeGeometry& node) { return node.nodeId == nodeId; });
        return found == nodes_.end() ? nullptr : &*found;
    }

    const WorkspaceSplitterGeometry* SolvedWorkspaceGeometry::hitTestSplitter(double x, double y) const noexcept
    {
        const auto found = std::find_if(splitters_.rbegin(), splitters_.rend(), [x, y](const WorkspaceSplitterGeometry& splitter)
        {
            return splitter.hitBounds.contains(x, y);
        });
        return found == splitters_.rend() ? nullptr : &*found;
    }

    const WorkspaceTabGeometry* SolvedWorkspaceGeometry::hitTestTab(double x, double y) const noexcept
    {
        const auto found = std::find_if(tabs_.rbegin(), tabs_.rend(), [x, y](const WorkspaceTabGeometry& tab)
        {
            return tab.bounds.contains(x, y);
        });
        return found == tabs_.rend() ? nullptr : &*found;
    }

    std::optional<SolvedWorkspaceGeometry> EditorWorkspaceGeometrySolver::solve(
        const EditorWorkspaceLayout& layout,
        WorkspaceRect bounds,
        WorkspaceGeometryOptions options,
        std::string* error)
    {
        if (error) error->clear();
        if (!layout.validate(error)) return std::nullopt;
        if (!finiteRect(bounds) || bounds.empty())
        {
            fail(error, "Workspace geometry bounds are invalid");
            return std::nullopt;
        }
        if (!std::isfinite(options.splitterThickness) || options.splitterThickness < 0.0 ||
            !std::isfinite(options.splitterHitThickness) || options.splitterHitThickness < options.splitterThickness ||
            !std::isfinite(options.tabBarHeight) || options.tabBarHeight < 0.0 ||
            !std::isfinite(options.minimumPaneExtent) || options.minimumPaneExtent < 0.0 ||
            !std::isfinite(options.minimumTabWidth) || options.minimumTabWidth <= 0.0 ||
            !std::isfinite(options.maximumTabWidth) || options.maximumTabWidth < options.minimumTabWidth)
        {
            fail(error, "Workspace geometry options are invalid");
            return std::nullopt;
        }

        SolvedWorkspaceGeometry result;
        if (!solveNode(layout.root(), bounds, options, result, error)) return std::nullopt;
        return result;
    }
}
