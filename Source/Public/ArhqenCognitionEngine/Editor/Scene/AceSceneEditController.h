#pragma once

#include "ArhqenCognitionEngine/Core/Scene/AceSceneWorld.h"
#include "ArhqenCognitionEngine/Editor/Transactions/AceTransaction.h"

#include <string>
#include <string_view>
#include <vector>

namespace am::editor::scene
{
    struct SceneEditResult final
    {
        bool success = false;
        std::string error;
        explicit operator bool() const noexcept { return success; }
    };

    class SceneEditController final
    {
    public:
        bool initialize(am::core::scene::SceneWorld& world,
                        am::core::scene::SceneSelection& selection,
                        am::editor::transactions::TransactionManager& transactions,
                        std::string* error = nullptr);
        [[nodiscard]] bool initialized() const noexcept;

        SceneEditResult rename(const am::core::Guid& id, std::string label);
        SceneEditResult setTransform(const am::core::Guid& id, const am::core::scene::Transform& transform);

        SceneEditResult beginInteractiveTransform(const std::vector<am::core::Guid>& ids);
        SceneEditResult updateInteractiveTransform(const std::vector<am::core::scene::Transform>& transforms);
        SceneEditResult commitInteractiveTransform();
        SceneEditResult cancelInteractiveTransform();
        [[nodiscard]] bool transforming() const noexcept { return interactiveActive_; }
        [[nodiscard]] const std::vector<am::core::Guid>& transformTargets() const noexcept { return targetIds_; }

    private:
        SceneEditResult failure(std::string message) const;
        SceneEditResult success() const;
        void restoreBefore() noexcept;

        am::core::scene::SceneWorld* world_ = nullptr;
        am::core::scene::SceneSelection* selection_ = nullptr;
        am::editor::transactions::TransactionManager* transactions_ = nullptr;
        bool interactiveActive_ = false;
        std::vector<am::core::Guid> targetIds_;
        std::vector<am::core::scene::Transform> before_;
        std::vector<am::core::scene::Transform> current_;
    };
}
