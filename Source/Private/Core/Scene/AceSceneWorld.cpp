#include "ArhqenCognitionEngine/Core/Scene/AceSceneWorld.h"

#include "ArhqenCognitionEngine/Core/Serialization/AceArchive.h"
#include "ArhqenCognitionEngine/Core/Serialization/AceAtomicFile.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace am::core::scene
{
    namespace
    {
        using serialization::ArchiveReader;
        using serialization::ArchiveWriter;
        using serialization::AtomicFile;

        constexpr std::uint32_t kSceneVersion = 1;
        constexpr std::size_t kMaximumEntities = 100000;
        constexpr std::size_t kMaximumDepth = 256;
        constexpr std::size_t kMaximumFileBytes = 128ull * 1024ull * 1024ull;
        constexpr Guid kSceneSchema = Guid::fromBytes({
            0x48, 0x4f, 0x99, 0x73, 0x3c, 0x22, 0x4a, 0x7f,
            0xb7, 0x86, 0xe5, 0xc5, 0x52, 0xc9, 0x91, 0x11});

        void fail(std::string* output, std::string message)
        {
            if (output) *output = std::move(message);
        }

        std::string foldAscii(std::string_view value)
        {
            std::string result(value);
            std::transform(result.begin(), result.end(), result.begin(), [](unsigned char byte)
            {
                return byte >= 'A' && byte <= 'Z' ? static_cast<char>(byte + ('a' - 'A')) : static_cast<char>(byte);
            });
            return result;
        }

        bool validUtf8(std::string_view text) noexcept
        {
            std::size_t index = 0;
            while (index < text.size())
            {
                const auto first = static_cast<unsigned char>(text[index]);
                if (first < 0x80u) { ++index; continue; }
                std::size_t count = 0;
                std::uint32_t codePoint = 0;
                if ((first & 0xe0u) == 0xc0u) { count = 1; codePoint = first & 0x1fu; }
                else if ((first & 0xf0u) == 0xe0u) { count = 2; codePoint = first & 0x0fu; }
                else if ((first & 0xf8u) == 0xf0u) { count = 3; codePoint = first & 0x07u; }
                else return false;
                if (index + count >= text.size()) return false;
                for (std::size_t offset = 1; offset <= count; ++offset)
                {
                    const auto next = static_cast<unsigned char>(text[index + offset]);
                    if ((next & 0xc0u) != 0x80u) return false;
                    codePoint = (codePoint << 6u) | (next & 0x3fu);
                }
                if ((count == 1 && codePoint < 0x80u) || (count == 2 && codePoint < 0x800u) ||
                    (count == 3 && codePoint < 0x10000u) || codePoint > 0x10ffffu ||
                    (codePoint >= 0xd800u && codePoint <= 0xdfffu)) return false;
                index += count + 1;
            }
            return true;
        }

        bool finiteVec(const Vec3d& value) noexcept
        {
            constexpr double maximum = 1.0e15;
            return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z) &&
                std::abs(value.x) <= maximum && std::abs(value.y) <= maximum && std::abs(value.z) <= maximum;
        }

        void writeVec(ArchiveWriter& writer, const Vec3d& value)
        {
            writer.writeDouble(value.x); writer.writeDouble(value.y); writer.writeDouble(value.z);
        }

        bool readVec(ArchiveReader& reader, Vec3d& value)
        {
            return reader.readDouble(value.x) && reader.readDouble(value.y) && reader.readDouble(value.z);
        }
    }

    SceneWorld::SceneWorld()
    {
        rootId_ = Guid::create();
        Entity root;
        root.id = rootId_;
        root.label = "Scene";
        root.kind = EntityKind::World;
        root.locked = true;
        entities_.push_back(std::move(root));
        indexById_.emplace(rootId_, 0);
        childrenByParent_.emplace(rootId_, std::vector<Guid>{});
    }

    const Entity* SceneWorld::find(const Guid& id) const noexcept
    {
        const auto found = indexById_.find(id);
        return found == indexById_.end() ? nullptr : &entities_[found->second];
    }

    Entity* SceneWorld::find(const Guid& id) noexcept
    {
        const auto found = indexById_.find(id);
        return found == indexById_.end() ? nullptr : &entities_[found->second];
    }

    std::vector<Guid> SceneWorld::childrenOf(const Guid& parentId) const
    {
        const auto found = childrenByParent_.find(parentId);
        return found == childrenByParent_.end() ? std::vector<Guid>{} : found->second;
    }

    bool SceneWorld::isDescendantOf(const Guid& candidate, const Guid& ancestor) const noexcept
    {
        const Entity* current = find(candidate);
        std::size_t depth = 0;
        while (current && current->parentId.isValid() && depth++ <= kMaximumDepth)
        {
            if (current->parentId == ancestor) return true;
            current = find(current->parentId);
        }
        return false;
    }

    bool SceneWorld::validLabel(std::string_view label) noexcept
    {
        if (label.empty() || label.size() > 512 || !validUtf8(label)) return false;
        return std::none_of(label.begin(), label.end(), [](unsigned char value)
        {
            return value == 0 || value == '\r' || value == '\n' || value < 0x20u;
        });
    }

    bool SceneWorld::validTransform(const Transform& transform) noexcept
    {
        return finiteVec(transform.location) && finiteVec(transform.rotationDegrees) && finiteVec(transform.scale);
    }

    std::optional<Guid> SceneWorld::createEntity(EntityKind kind, std::string label, const Guid& parentId,
                                                  const Guid& assetId, std::string* error)
    {
        if (error) error->clear();
        if (entities_.size() >= kMaximumEntities) { fail(error, "Scene entity limit reached"); return std::nullopt; }
        if (kind == EntityKind::World || !find(parentId)) { fail(error, "Scene entity parent or kind is invalid"); return std::nullopt; }
        if (!validLabel(label)) { fail(error, "Scene entity label is empty, invalid UTF-8, or too long"); return std::nullopt; }
        std::size_t depth = 1;
        const Entity* ancestor = find(parentId);
        while (ancestor && ancestor->parentId.isValid())
        {
            if (++depth > kMaximumDepth)
            {
                fail(error, "Scene hierarchy exceeds maximum depth");
                return std::nullopt;
            }
            ancestor = find(ancestor->parentId);
        }
        Entity entity;
        do { entity.id = Guid::create(); } while (find(entity.id));
        entity.parentId = parentId;
        entity.label = std::move(label);
        entity.kind = kind;
        entity.assetId = assetId;
        indexById_.emplace(entity.id, entities_.size());
        entities_.push_back(std::move(entity));
        childrenByParent_[parentId].push_back(entities_.back().id);
        childrenByParent_.try_emplace(entities_.back().id);
        ++generation_;
        return entities_.back().id;
    }

    void SceneWorld::collectSubtree(const Guid& id, std::vector<Guid>& output) const
    {
        output.push_back(id);
        const auto found = childrenByParent_.find(id);
        if (found != childrenByParent_.end())
            for (const Guid& child : found->second) collectSubtree(child, output);
    }

    bool SceneWorld::removeEntity(const Guid& id, std::vector<Guid>* removed, std::string* error)
    {
        if (error) error->clear();
        if (id == rootId_ || !find(id)) { fail(error, "Scene root cannot be removed or entity is missing"); return false; }
        std::vector<Guid> subtree;
        collectSubtree(id, subtree);
        std::unordered_set<Guid, GuidHash> removeSet(subtree.begin(), subtree.end());
        entities_.erase(std::remove_if(entities_.begin(), entities_.end(), [&](const Entity& entity)
        {
            return removeSet.contains(entity.id);
        }), entities_.end());
        if (!rebuildIndex(error)) return false;
        if (removed) *removed = std::move(subtree);
        ++generation_;
        return true;
    }

    bool SceneWorld::reparentEntity(const Guid& id, const Guid& newParentId, std::string* error)
    {
        if (error) error->clear();
        Entity* entity = find(id);
        if (!entity || id == rootId_ || !find(newParentId) || id == newParentId || isDescendantOf(newParentId, id))
        { fail(error, "Scene reparent would use a missing entity or create a cycle"); return false; }
        if (entity->parentId == newParentId) return true;
        const Guid oldParentId = entity->parentId;
        entity->parentId = newParentId;
        auto& oldChildren = childrenByParent_[oldParentId];
        oldChildren.erase(std::remove(oldChildren.begin(), oldChildren.end(), id), oldChildren.end());
        childrenByParent_[newParentId].push_back(id);
        std::string validationError;
        if (!validate(&validationError))
        {
            entity->parentId = oldParentId;
            auto& newChildren = childrenByParent_[newParentId];
            newChildren.erase(std::remove(newChildren.begin(), newChildren.end(), id), newChildren.end());
            childrenByParent_[oldParentId].push_back(id);
            fail(error, std::move(validationError));
            return false;
        }
        ++generation_;
        return true;
    }

    bool SceneWorld::renameEntity(const Guid& id, std::string label, std::string* error)
    {
        if (error) error->clear();
        Entity* entity = find(id);
        if (!entity || id == rootId_ || !validLabel(label)) { fail(error, "Scene entity cannot be renamed to this label"); return false; }
        if (entity->label == label) return true;
        entity->label = std::move(label);
        ++generation_;
        return true;
    }

    bool SceneWorld::setTransform(const Guid& id, const Transform& transform, std::string* error)
    {
        if (error) error->clear();
        Entity* entity = find(id);
        if (!entity || id == rootId_ || !validTransform(transform)) { fail(error, "Scene transform is invalid or immutable"); return false; }
        if (entity->transform == transform) return true;
        entity->transform = transform;
        ++generation_;
        return true;
    }

    bool SceneWorld::setVisible(const Guid& id, bool visible, std::string* error)
    {
        if (error) error->clear();
        Entity* entity = find(id);
        if (!entity || id == rootId_) { fail(error, "Scene visibility target is invalid"); return false; }
        if (entity->visible == visible) return true;
        entity->visible = visible; ++generation_; return true;
    }

    bool SceneWorld::setLocked(const Guid& id, bool locked, std::string* error)
    {
        if (error) error->clear();
        Entity* entity = find(id);
        if (!entity || id == rootId_) { fail(error, "Scene lock target is invalid"); return false; }
        if (entity->locked == locked) return true;
        entity->locked = locked; ++generation_; return true;
    }

    bool SceneWorld::rebuildIndex(std::string* error)
    {
        indexById_.clear();
        childrenByParent_.clear();
        for (std::size_t index = 0; index < entities_.size(); ++index)
        {
            if (!entities_[index].id.isValid() || !indexById_.emplace(entities_[index].id, index).second)
            { fail(error, "Scene contains an invalid or duplicate entity GUID"); return false; }
            childrenByParent_.try_emplace(entities_[index].id);
        }
        for (const auto& entity : entities_)
            if (entity.parentId.isValid()) childrenByParent_[entity.parentId].push_back(entity.id);
        return true;
    }

    bool SceneWorld::validate(std::string* error) const
    {
        if (error) error->clear();
        if (entities_.empty() || entities_.size() > kMaximumEntities || !rootId_.isValid())
        { fail(error, "Scene has invalid entity count or root"); return false; }
        const Entity* root = find(rootId_);
        if (!root || root->kind != EntityKind::World || root->parentId.isValid())
        { fail(error, "Scene root is missing or malformed"); return false; }
        std::size_t worldCount = 0;
        for (const auto& entity : entities_)
        {
            if (entity.kind == EntityKind::World) ++worldCount;
            if (!validLabel(entity.label) || !validTransform(entity.transform))
            { fail(error, "Scene entity label or transform is invalid"); return false; }
            if (entity.id != rootId_ && (!entity.parentId.isValid() || !find(entity.parentId)))
            { fail(error, "Scene entity parent is missing"); return false; }
            const Entity* current = &entity;
            std::unordered_set<Guid, GuidHash> visited;
            std::size_t depth = 0;
            while (current->parentId.isValid())
            {
                if (++depth > kMaximumDepth || !visited.insert(current->id).second)
                { fail(error, "Scene hierarchy is cyclic or too deep"); return false; }
                current = find(current->parentId);
                if (!current) { fail(error, "Scene hierarchy parent disappeared"); return false; }
            }
            if (current->id != rootId_) { fail(error, "Scene entity is not rooted in the world"); return false; }
        }
        if (worldCount != 1) { fail(error, "Scene must contain exactly one world root"); return false; }
        return true;
    }

    bool SceneWorld::save(const std::filesystem::path& path, std::string* error) const
    {
        if (!validate(error)) return false;
        ArchiveWriter writer(kSceneSchema, kSceneVersion);
        writer.writeGuid(rootId_);
        writer.writeU64(generation_);
        writer.writeU32(static_cast<std::uint32_t>(entities_.size()));
        for (const auto& entity : entities_)
        {
            writer.writeGuid(entity.id); writer.writeGuid(entity.parentId);
            if (!writer.writeString(entity.label)) { fail(error, "Scene label is not serializable"); return false; }
            writer.writeU8(static_cast<std::uint8_t>(entity.kind));
            writeVec(writer, entity.transform.location); writeVec(writer, entity.transform.rotationDegrees); writeVec(writer, entity.transform.scale);
            writer.writeGuid(entity.assetId); writer.writeBool(entity.visible); writer.writeBool(entity.locked);
        }
        return AtomicFile::write(path, writer.finish(), error);
    }

    std::optional<SceneWorld> SceneWorld::load(const std::filesystem::path& path, std::string* error)
    {
        std::vector<std::uint8_t> bytes;
        if (!AtomicFile::read(path, bytes, kMaximumFileBytes, error)) return std::nullopt;
        ArchiveReader reader;
        if (!reader.open(bytes, kSceneSchema, kSceneVersion, kSceneVersion, kMaximumFileBytes))
        { fail(error, std::string("Scene archive rejected: ") + std::string(serialization::archiveErrorText(reader.error()))); return std::nullopt; }
        SceneWorld result(LoadTag{});
        std::uint32_t count = 0;
        if (!reader.readGuid(result.rootId_) || !reader.readU64(result.generation_) || !reader.readU32(count) ||
            count == 0 || count > kMaximumEntities)
        { fail(error, "Scene header is malformed"); return std::nullopt; }
        result.entities_.resize(count);
        for (auto& entity : result.entities_)
        {
            std::uint8_t kind = 0;
            if (!reader.readGuid(entity.id) || !reader.readGuid(entity.parentId) || !reader.readString(entity.label, 512) ||
                !reader.readU8(kind) || kind > static_cast<std::uint8_t>(EntityKind::Landscape) ||
                !readVec(reader, entity.transform.location) || !readVec(reader, entity.transform.rotationDegrees) ||
                !readVec(reader, entity.transform.scale) || !reader.readGuid(entity.assetId) ||
                !reader.readBool(entity.visible) || !reader.readBool(entity.locked))
            { fail(error, "Scene entity payload is malformed"); return std::nullopt; }
            entity.kind = static_cast<EntityKind>(kind);
        }
        if (!reader.atEnd() || !result.rebuildIndex(error) || !result.validate(error)) return std::nullopt;
        return result;
    }

    std::string_view SceneWorld::kindName(EntityKind kind) noexcept
    {
        switch (kind)
        {
        case EntityKind::World: return "World"; case EntityKind::Folder: return "Folder";
        case EntityKind::Empty: return "Empty"; case EntityKind::StaticMesh: return "Static Mesh";
        case EntityKind::Camera: return "Camera"; case EntityKind::DirectionalLight: return "Directional Light";
        case EntityKind::PointLight: return "Point Light"; case EntityKind::SpotLight: return "Spot Light";
        case EntityKind::Landscape: return "Landscape";
        }
        return "Unknown";
    }

    bool SceneSelection::select(const SceneWorld& world, const Guid& id, SceneSelectionMode mode)
    {
        if (!world.find(id)) return false;
        if (mode == SceneSelectionMode::Replace) clear();
        if (mode == SceneSelectionMode::Toggle && selectedSet_.contains(id))
        {
            selectedSet_.erase(id);
            selected_.erase(std::remove(selected_.begin(), selected_.end(), id), selected_.end());
            primary_ = selected_.empty() ? Guid{} : selected_.back();
            return true;
        }
        if (selectedSet_.insert(id).second) selected_.push_back(id);
        primary_ = id;
        return true;
    }

    void SceneSelection::clear() noexcept
    {
        selected_.clear(); selectedSet_.clear(); primary_ = {};
    }

    void SceneSelection::reconcile(const SceneWorld& world)
    {
        selected_.erase(std::remove_if(selected_.begin(), selected_.end(), [&](const Guid& id)
        {
            if (world.find(id)) return false;
            selectedSet_.erase(id); return true;
        }), selected_.end());
        if (!selectedSet_.contains(primary_)) primary_ = selected_.empty() ? Guid{} : selected_.back();
    }

    bool SceneSelection::contains(const Guid& id) const noexcept { return selectedSet_.contains(id); }
    const Entity* SceneSelection::primaryEntity(const SceneWorld& world) const noexcept { return world.find(primary_); }

    SceneHierarchyModel::SceneHierarchyModel() = default;
    void SceneHierarchyModel::setExpanded(const Guid& id, bool expanded)
    {
        if (expanded) expanded_.insert(id); else expanded_.erase(id);
    }
    bool SceneHierarchyModel::isExpanded(const Guid& id) const noexcept { return expanded_.contains(id); }
    void SceneHierarchyModel::setSearchText(std::string text)
    {
        searchText_ = std::move(text); foldedSearch_ = foldAscii(searchText_);
    }

    void SceneHierarchyModel::rebuild(const SceneWorld& world, const SceneSelection& selection)
    {
        rows_.clear();
        std::unordered_set<Guid, GuidHash> include;
        if (!foldedSearch_.empty())
        {
            for (const auto& entity : world.entities())
            {
                if (foldAscii(entity.label).find(foldedSearch_) == std::string::npos) continue;
                const Entity* current = &entity;
                while (current)
                {
                    include.insert(current->id);
                    current = current->parentId.isValid() ? world.find(current->parentId) : nullptr;
                }
            }
        }

        const bool searching = !foldedSearch_.empty();
        auto append = [&](auto&& self, const Guid& id, std::size_t depth) -> void
        {
            const Entity* entity = world.find(id);
            if (!entity || (searching && !include.contains(id))) return;
            const auto children = world.childrenOf(id);
            const bool expanded = searching || id == world.rootId() || isExpanded(id);
            rows_.push_back({id, depth, entity->label, entity->kind, !children.empty(), expanded, selection.contains(id)});
            if (!expanded) return;
            for (const Guid& child : children) self(self, child, depth + 1);
        };
        append(append, world.rootId(), 0);
    }
}
