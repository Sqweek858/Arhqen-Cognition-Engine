#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace am::editor
{
    enum class DockNodeKind : std::uint8_t
    {
        Split = 0,
        Stack = 1
    };

    enum class DockOrientation : std::uint8_t
    {
        Horizontal = 0,
        Vertical = 1
    };

    struct EditorDockTab
    {
        std::string id;
        std::string label;
        bool visible = true;
        bool closeable = true;
    };

    struct EditorDockNode
    {
        std::string id;
        DockNodeKind kind = DockNodeKind::Stack;
        DockOrientation orientation = DockOrientation::Horizontal;
        double sizeCoefficient = 1.0;
        std::vector<EditorDockNode> children;
        std::vector<EditorDockTab> tabs;
        std::string activeTab;
    };

    class EditorWorkspaceLayout final
    {
    public:
        static EditorWorkspaceLayout createDefault();

        [[nodiscard]] const EditorDockNode& root() const noexcept;
        [[nodiscard]] EditorDockNode& root() noexcept;
        [[nodiscard]] const EditorDockNode* findNode(std::string_view nodeId) const noexcept;
        [[nodiscard]] EditorDockNode* findNode(std::string_view nodeId) noexcept;
        [[nodiscard]] const EditorDockTab* findTab(std::string_view tabId) const noexcept;
        [[nodiscard]] bool isNodeVisible(std::string_view nodeId) const noexcept;

        bool setTabVisible(std::string_view tabId, bool visible, std::string* error = nullptr);
        bool activateTab(std::string_view tabId, std::string* error = nullptr);
        bool setChildCoefficient(std::string_view splitterId, std::size_t childIndex, double coefficient, std::string* error = nullptr);
        bool resetToDefault();

        bool normalizeAndValidate(std::string* error = nullptr);
        [[nodiscard]] bool validate(std::string* error = nullptr) const;
        bool save(const std::filesystem::path& path, std::string* error = nullptr) const;
        static std::optional<EditorWorkspaceLayout> load(const std::filesystem::path& path, std::string* error = nullptr);

    private:
        EditorDockNode root_;
    };
}
