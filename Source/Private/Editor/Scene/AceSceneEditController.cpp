#include "ArhqenCognitionEngine/Editor/Scene/AceSceneEditController.h"

#include <algorithm>
#include <memory>
#include <utility>

namespace am::editor::scene
{
    namespace
    {
        using am::core::Guid;
        using am::core::scene::SceneWorld;
        using am::core::scene::Transform;
        using am::editor::transactions::Operation;

        class TransformOperation final : public Operation
        {
        public:
            TransformOperation(SceneWorld& world, std::vector<Guid> ids,
                               std::vector<Transform> before, std::vector<Transform> after)
                : world_(&world), ids_(std::move(ids)), before_(std::move(before)), after_(std::move(after)) {}
            void undo() noexcept override { apply(before_); }
            void redo() noexcept override { apply(after_); }
            std::size_t memoryCost() const noexcept override
            {
                return sizeof(*this) + ids_.capacity() * sizeof(Guid) +
                    (before_.capacity() + after_.capacity()) * sizeof(Transform);
            }
        private:
            void apply(const std::vector<Transform>& values) noexcept
            {
                if (!world_) return;
                for (std::size_t index = 0; index < ids_.size() && index < values.size(); ++index)
                    world_->setTransform(ids_[index], values[index], nullptr);
            }
            SceneWorld* world_ = nullptr;
            std::vector<Guid> ids_;
            std::vector<Transform> before_;
            std::vector<Transform> after_;
        };

        class RenameOperation final : public Operation
        {
        public:
            RenameOperation(SceneWorld& world, Guid id, std::string before, std::string after)
                : world_(&world), id_(id), before_(std::move(before)), after_(std::move(after)) {}
            void undo() noexcept override { if (world_) world_->renameEntity(id_, before_, nullptr); }
            void redo() noexcept override { if (world_) world_->renameEntity(id_, after_, nullptr); }
            std::size_t memoryCost() const noexcept override
            { return sizeof(*this) + before_.capacity() + after_.capacity(); }
        private:
            SceneWorld* world_ = nullptr;
            Guid id_{};
            std::string before_;
            std::string after_;
        };
    }

    bool SceneEditController::initialize(am::core::scene::SceneWorld& world,
                                         am::core::scene::SceneSelection& selection,
                                         am::editor::transactions::TransactionManager& transactions,
                                         std::string* error)
    {
        if (error) error->clear();
        if (!world.validate(error)) return false;
        world_ = &world; selection_ = &selection; transactions_ = &transactions;
        return true;
    }

    bool SceneEditController::initialized() const noexcept { return world_ && selection_ && transactions_; }
    SceneEditResult SceneEditController::failure(std::string message) const { return {false, std::move(message)}; }
    SceneEditResult SceneEditController::success() const { return {true, {}}; }

    SceneEditResult SceneEditController::rename(const Guid& id, std::string label)
    {
        if (!initialized() || interactiveActive_) return failure("Scene edit controller is unavailable or tracking");
        const auto* entity = world_->find(id);
        if (!entity || id == world_->rootId() || entity->locked) return failure("Scene entity is missing, root, or locked");
        const std::string before = entity->label;
        if (before == label) return success();
        if (!transactions_->begin("Rename scene entity")) return failure("Another editor transaction is active");
        std::string error;
        if (!world_->renameEntity(id, label, &error)) { transactions_->cancel(); return failure(std::move(error)); }
        if (!transactions_->add(std::make_unique<RenameOperation>(*world_, id, before, label)) || !transactions_->commit())
        { world_->renameEntity(id, before, nullptr); transactions_->cancel(); return failure("Could not record scene rename"); }
        return success();
    }

    SceneEditResult SceneEditController::setTransform(const Guid& id, const Transform& transform)
    {
        if (!initialized() || interactiveActive_) return failure("Scene edit controller is unavailable or tracking");
        const auto* entity = world_->find(id);
        if (!entity || id == world_->rootId() || entity->locked) return failure("Scene entity is missing, root, or locked");
        const Transform before = entity->transform;
        if (before == transform) return success();
        if (!transactions_->begin("Transform scene entity")) return failure("Another editor transaction is active");
        std::string error;
        if (!world_->setTransform(id, transform, &error)) { transactions_->cancel(); return failure(std::move(error)); }
        if (!transactions_->add(std::make_unique<TransformOperation>(*world_, std::vector<Guid>{id},
            std::vector<Transform>{before}, std::vector<Transform>{transform})) || !transactions_->commit())
        { world_->setTransform(id, before, nullptr); transactions_->cancel(); return failure("Could not record scene transform"); }
        return success();
    }

    SceneEditResult SceneEditController::beginInteractiveTransform(const std::vector<Guid>& ids)
    {
        if (!initialized() || interactiveActive_ || ids.empty()) return failure("Interactive transform cannot begin");
        targetIds_.clear(); before_.clear(); current_.clear();
        targetIds_.reserve(ids.size()); before_.reserve(ids.size());
        for (const Guid& id : ids)
        {
            const auto* entity = world_->find(id);
            if (!entity || id == world_->rootId() || entity->locked)
            { targetIds_.clear(); before_.clear(); return failure("Transform target is missing, root, or locked"); }
            if (std::find(targetIds_.begin(), targetIds_.end(), id) != targetIds_.end()) continue;
            targetIds_.push_back(id); before_.push_back(entity->transform);
        }
        if (targetIds_.empty() || !transactions_->begin("Interactive scene transform"))
        { targetIds_.clear(); before_.clear(); return failure("Another editor transaction is active"); }
        current_ = before_; interactiveActive_ = true; return success();
    }

    SceneEditResult SceneEditController::updateInteractiveTransform(const std::vector<Transform>& transforms)
    {
        if (!interactiveActive_ || transforms.size() != targetIds_.size()) return failure("Interactive transform payload does not match targets");
        std::size_t applied = 0;
        std::string error;
        for (; applied < transforms.size(); ++applied)
        {
            if (!world_->setTransform(targetIds_[applied], transforms[applied], &error))
            {
                for (std::size_t rollback = 0; rollback < applied; ++rollback)
                    world_->setTransform(targetIds_[rollback], current_[rollback], nullptr);
                return failure(std::move(error));
            }
        }
        current_ = transforms; return success();
    }

    void SceneEditController::restoreBefore() noexcept
    {
        if (!world_) return;
        for (std::size_t index = 0; index < targetIds_.size(); ++index)
            world_->setTransform(targetIds_[index], before_[index], nullptr);
    }

    SceneEditResult SceneEditController::commitInteractiveTransform()
    {
        if (!interactiveActive_) return failure("No interactive transform is active");
        interactiveActive_ = false;
        if (before_ == current_)
        {
            transactions_->cancel(); targetIds_.clear(); before_.clear(); current_.clear(); return success();
        }
        if (!transactions_->add(std::make_unique<TransformOperation>(*world_, targetIds_, before_, current_)) ||
            !transactions_->commit())
        {
            restoreBefore(); transactions_->cancel(); targetIds_.clear(); before_.clear(); current_.clear();
            return failure("Could not record interactive transform");
        }
        targetIds_.clear(); before_.clear(); current_.clear(); return success();
    }

    SceneEditResult SceneEditController::cancelInteractiveTransform()
    {
        if (!interactiveActive_) return failure("No interactive transform is active");
        restoreBefore(); interactiveActive_ = false; transactions_->cancel();
        targetIds_.clear(); before_.clear(); current_.clear(); return success();
    }
}
