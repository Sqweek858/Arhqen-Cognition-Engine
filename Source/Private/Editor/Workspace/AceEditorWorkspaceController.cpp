#include "ArhqenCognitionEngine/Editor/Workspace/AceEditorWorkspaceController.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace am::editor
{
    namespace
    {
        bool sameRect(const WorkspaceRect& left, const WorkspaceRect& right) noexcept
        {
            return left.left == right.left && left.top == right.top &&
                left.right == right.right && left.bottom == right.bottom;
        }
    }

    EditorWorkspaceController::EditorWorkspaceController()
        : layout_(EditorWorkspaceLayout::createDefault())
    {
    }

    EditorWorkspaceController::EditorWorkspaceController(EditorWorkspaceLayout layout)
        : layout_(std::move(layout))
    {
        std::string ignored;
        if (!layout_.normalizeAndValidate(&ignored)) layout_ = EditorWorkspaceLayout::createDefault();
    }

    const SolvedWorkspaceGeometry* EditorWorkspaceController::geometry() const noexcept
    {
        return geometry_ ? &*geometry_ : nullptr;
    }

    bool EditorWorkspaceController::arrange(WorkspaceRect bounds, WorkspaceGeometryOptions options, std::string* error)
    {
        if (drag_ && (!sameRect(bounds_, bounds) ||
            options.splitterThickness != options_.splitterThickness ||
            options.splitterHitThickness != options_.splitterHitThickness ||
            options.tabBarHeight != options_.tabBarHeight ||
            options.minimumPaneExtent != options_.minimumPaneExtent ||
            options.minimumTabWidth != options_.minimumTabWidth ||
            options.maximumTabWidth != options_.maximumTabWidth))
        {
            if (!cancelPointerInteraction(error)) return false;
        }
        bounds_ = bounds;
        options_ = options;
        arranged_ = true;
        return solve(error);
    }

    bool EditorWorkspaceController::pointerDown(double x, double y, std::string* error)
    {
        if (!geometry_ || drag_) return false;
        if (const auto* splitter = geometry_->hitTestSplitter(x, y))
        {
            SplitterDrag drag;
            drag.splitterId = splitter->splitterId;
            drag.orientation = splitter->orientation;
            drag.leadingChildIndex = splitter->leadingChildIndex;
            drag.trailingChildIndex = splitter->trailingChildIndex;
            drag.pairStart = splitter->orientation == DockOrientation::Horizontal
                ? splitter->leadingPaneBounds.left : splitter->leadingPaneBounds.top;
            drag.pairAvailable = splitter->orientation == DockOrientation::Horizontal
                ? splitter->leadingPaneBounds.width() + splitter->trailingPaneBounds.width()
                : splitter->leadingPaneBounds.height() + splitter->trailingPaneBounds.height();
            drag.splitterThickness = splitter->orientation == DockOrientation::Horizontal
                ? splitter->visualBounds.width() : splitter->visualBounds.height();
            drag.lastPointer = splitter->orientation == DockOrientation::Horizontal ? x : y;
            drag.originalLayout = layout_;
            drag.dirtyBefore = dirty_;
            drag.commitBefore = commitRequested_;
            drag_ = std::move(drag);
            return true;
        }

        if (const auto* tab = geometry_->hitTestTab(x, y))
        {
            const auto* node = layout_.findNode(tab->nodeId);
            if (node && node->activeTab == tab->tabId) return true;
            if (!layout_.activateTab(tab->tabId, error) || !solve(error)) return false;
            dirty_ = true;
            commitRequested_ = true;
            return true;
        }
        return false;
    }

    bool EditorWorkspaceController::pointerMove(double x, double y, std::string* error)
    {
        if (!drag_) return false;
        const double pointer = drag_->orientation == DockOrientation::Horizontal ? x : y;
        if (!std::isfinite(pointer) || drag_->pairAvailable <= 0.0) return false;
        if (pointer == drag_->lastPointer) return true;
        const double firstExtent = pointer - drag_->pairStart - (drag_->splitterThickness * 0.5);
        const double ratio = std::clamp(firstExtent / drag_->pairAvailable, 0.0, 1.0);
        if (!layout_.setChildPairRatio(
            drag_->splitterId,
            drag_->leadingChildIndex,
            drag_->trailingChildIndex,
            ratio,
            error)) return false;
        if (!solve(error)) return false;
        drag_->lastPointer = pointer;
        dirty_ = true;
        drag_->changed = true;
        return true;
    }

    bool EditorWorkspaceController::pointerUp(double x, double y, std::string* error)
    {
        if (!drag_) return false;
        if (std::isfinite(x) && std::isfinite(y) && !pointerMove(x, y, error)) return false;
        const bool changed = drag_->changed;
        drag_.reset();
        commitRequested_ = commitRequested_ || changed;
        return true;
    }

    bool EditorWorkspaceController::cancelPointerInteraction(std::string* error)
    {
        if (!drag_) return false;
        layout_ = std::move(drag_->originalLayout);
        dirty_ = drag_->dirtyBefore;
        commitRequested_ = drag_->commitBefore;
        drag_.reset();
        return solve(error);
    }

    WorkspaceCursor EditorWorkspaceController::cursorAt(double x, double y) const noexcept
    {
        if (drag_)
            return drag_->orientation == DockOrientation::Horizontal ? WorkspaceCursor::ResizeHorizontal : WorkspaceCursor::ResizeVertical;
        if (!geometry_) return WorkspaceCursor::Arrow;
        const auto* splitter = geometry_->hitTestSplitter(x, y);
        if (!splitter) return WorkspaceCursor::Arrow;
        return splitter->orientation == DockOrientation::Horizontal ? WorkspaceCursor::ResizeHorizontal : WorkspaceCursor::ResizeVertical;
    }

    bool EditorWorkspaceController::setTabVisible(std::string_view tabId, bool visible, std::string* error)
    {
        if (drag_ && !cancelPointerInteraction(error)) return false;
        const auto* existing = layout_.findTab(tabId);
        if (!existing) return layout_.setTabVisible(tabId, visible, error);
        if (existing->visible == visible) return true;
        if (!layout_.setTabVisible(tabId, visible, error) || (arranged_ && !solve(error))) return false;
        dirty_ = true;
        commitRequested_ = true;
        return true;
    }

    bool EditorWorkspaceController::resetLayout(std::string* error)
    {
        if (drag_ && !cancelPointerInteraction(error)) return false;
        layout_.resetToDefault();
        if (arranged_ && !solve(error)) return false;
        dirty_ = true;
        commitRequested_ = true;
        return true;
    }

    bool EditorWorkspaceController::takeCommitRequested() noexcept
    {
        const bool requested = commitRequested_;
        commitRequested_ = false;
        return requested;
    }

    bool EditorWorkspaceController::solve(std::string* error)
    {
        if (!arranged_) return true;
        auto solved = EditorWorkspaceGeometrySolver::solve(layout_, bounds_, options_, error);
        if (!solved) return false;
        geometry_ = std::move(*solved);
        return true;
    }
}
