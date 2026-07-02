#pragma once

#include "ArhqenCognitionEngine/Editor/Workspace/AceEditorWorkspaceGeometry.h"

#include <optional>
#include <string>
#include <string_view>

namespace am::editor
{
    enum class WorkspaceCursor
    {
        Arrow,
        ResizeHorizontal,
        ResizeVertical
    };

    class EditorWorkspaceController final
    {
    public:
        EditorWorkspaceController();
        explicit EditorWorkspaceController(EditorWorkspaceLayout layout);

        [[nodiscard]] const EditorWorkspaceLayout& layout() const noexcept { return layout_; }
        [[nodiscard]] const SolvedWorkspaceGeometry* geometry() const noexcept;
        [[nodiscard]] bool draggingSplitter() const noexcept { return drag_.has_value(); }
        [[nodiscard]] bool dirty() const noexcept { return dirty_; }

        bool arrange(WorkspaceRect bounds, WorkspaceGeometryOptions options = {}, std::string* error = nullptr);
        bool pointerDown(double x, double y, std::string* error = nullptr);
        bool pointerMove(double x, double y, std::string* error = nullptr);
        bool pointerUp(double x, double y, std::string* error = nullptr);
        bool cancelPointerInteraction(std::string* error = nullptr);
        [[nodiscard]] WorkspaceCursor cursorAt(double x, double y) const noexcept;

        bool setTabVisible(std::string_view tabId, bool visible, std::string* error = nullptr);
        bool resetLayout(std::string* error = nullptr);
        [[nodiscard]] bool takeCommitRequested() noexcept;
        void markSaved() noexcept { dirty_ = false; }

    private:
        struct SplitterDrag
        {
            std::string splitterId;
            DockOrientation orientation = DockOrientation::Horizontal;
            std::size_t leadingChildIndex = 0;
            std::size_t trailingChildIndex = 0;
            double pairStart = 0.0;
            double pairAvailable = 0.0;
            double splitterThickness = 0.0;
            double lastPointer = 0.0;
            EditorWorkspaceLayout originalLayout;
            bool dirtyBefore = false;
            bool commitBefore = false;
            bool changed = false;
        };

        bool solve(std::string* error);

        EditorWorkspaceLayout layout_;
        WorkspaceRect bounds_{};
        WorkspaceGeometryOptions options_{};
        std::optional<SolvedWorkspaceGeometry> geometry_;
        std::optional<SplitterDrag> drag_;
        bool arranged_ = false;
        bool dirty_ = false;
        bool commitRequested_ = false;
    };
}
