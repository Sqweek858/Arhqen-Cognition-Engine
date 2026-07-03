#pragma once

#include "ArhqenCognitionEngine/Core/Identity/AceGuid.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace am::core::scene
{
    struct Vec3d final
    {
        double x = 0.0;
        double y = 0.0;
        double z = 0.0;
        friend bool operator==(const Vec3d&, const Vec3d&) = default;
    };

    struct Transform final
    {
        Vec3d location{};
        Vec3d rotationDegrees{};
        Vec3d scale{1.0, 1.0, 1.0};
        friend bool operator==(const Transform&, const Transform&) = default;
    };

    enum class EntityKind : std::uint8_t
    {
        World,
        Folder,
        Empty,
        StaticMesh,
        Camera,
        DirectionalLight,
        PointLight,
        SpotLight,
        Landscape
    };

    struct Entity final
    {
        Guid id{};
        Guid parentId{};
        std::string label;
        EntityKind kind = EntityKind::Empty;
        Transform transform{};
        Guid assetId{};
        bool visible = true;
        bool locked = false;
    };

    class SceneWorld final
    {
    public:
        SceneWorld();

        [[nodiscard]] const Guid& rootId() const noexcept { return rootId_; }
        [[nodiscard]] std::uint64_t generation() const noexcept { return generation_; }
        [[nodiscard]] std::size_t entityCount() const noexcept { return entities_.size(); }
        [[nodiscard]] const std::vector<Entity>& entities() const noexcept { return entities_; }
        [[nodiscard]] const Entity* find(const Guid& id) const noexcept;
        [[nodiscard]] Entity* find(const Guid& id) noexcept;
        [[nodiscard]] std::vector<Guid> childrenOf(const Guid& parentId) const;
        [[nodiscard]] bool isDescendantOf(const Guid& candidate, const Guid& ancestor) const noexcept;

        std::optional<Guid> createEntity(EntityKind kind, std::string label, const Guid& parentId,
                                         const Guid& assetId = {}, std::string* error = nullptr);
        bool removeEntity(const Guid& id, std::vector<Guid>* removed = nullptr, std::string* error = nullptr);
        bool reparentEntity(const Guid& id, const Guid& newParentId, std::string* error = nullptr);
        bool renameEntity(const Guid& id, std::string label, std::string* error = nullptr);
        bool setTransform(const Guid& id, const Transform& transform, std::string* error = nullptr);
        bool setVisible(const Guid& id, bool visible, std::string* error = nullptr);
        bool setLocked(const Guid& id, bool locked, std::string* error = nullptr);

        [[nodiscard]] bool validate(std::string* error = nullptr) const;
        bool save(const std::filesystem::path& path, std::string* error = nullptr) const;
        static std::optional<SceneWorld> load(const std::filesystem::path& path, std::string* error = nullptr);
        [[nodiscard]] static std::string_view kindName(EntityKind kind) noexcept;

    private:
        struct LoadTag {};
        explicit SceneWorld(LoadTag) noexcept {}
        bool rebuildIndex(std::string* error = nullptr);
        static bool validLabel(std::string_view label) noexcept;
        static bool validTransform(const Transform& transform) noexcept;
        void collectSubtree(const Guid& id, std::vector<Guid>& output) const;

        Guid rootId_{};
        std::uint64_t generation_ = 1;
        std::vector<Entity> entities_;
        std::unordered_map<Guid, std::size_t, GuidHash> indexById_;
        std::unordered_map<Guid, std::vector<Guid>, GuidHash> childrenByParent_;
    };

    enum class SceneSelectionMode : std::uint8_t { Replace, Add, Toggle };

    class SceneSelection final
    {
    public:
        bool select(const SceneWorld& world, const Guid& id,
                    SceneSelectionMode mode = SceneSelectionMode::Replace);
        void clear() noexcept;
        void reconcile(const SceneWorld& world);
        [[nodiscard]] bool contains(const Guid& id) const noexcept;
        [[nodiscard]] const std::vector<Guid>& selected() const noexcept { return selected_; }
        [[nodiscard]] const Guid& primary() const noexcept { return primary_; }
        [[nodiscard]] const Entity* primaryEntity(const SceneWorld& world) const noexcept;

    private:
        std::vector<Guid> selected_;
        std::unordered_set<Guid, GuidHash> selectedSet_;
        Guid primary_{};
    };

    struct HierarchyRow final
    {
        Guid id{};
        std::size_t depth = 0;
        std::string label;
        EntityKind kind = EntityKind::Empty;
        bool hasChildren = false;
        bool expanded = false;
        bool selected = false;
    };

    class SceneHierarchyModel final
    {
    public:
        SceneHierarchyModel();
        void setExpanded(const Guid& id, bool expanded);
        [[nodiscard]] bool isExpanded(const Guid& id) const noexcept;
        void setSearchText(std::string text);
        [[nodiscard]] const std::string& searchText() const noexcept { return searchText_; }
        void rebuild(const SceneWorld& world, const SceneSelection& selection);
        [[nodiscard]] const std::vector<HierarchyRow>& rows() const noexcept { return rows_; }

    private:
        std::unordered_set<Guid, GuidHash> expanded_;
        std::string searchText_;
        std::string foldedSearch_;
        std::vector<HierarchyRow> rows_;
    };
}
