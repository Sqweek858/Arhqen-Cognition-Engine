#include "ArhqenCognitionEngine/Editor/ContentBrowser/AceContentBrowserController.h"

#include <utility>

namespace am::editor::content_browser
{
    namespace
    {
        void fail(std::string* output, std::string message)
        {
            if (output) *output = std::move(message);
        }
    }

    bool ContentBrowserController::initialize(am::core::assets::AssetRegistry& registry,
                                              am::editor::assets::AssetOperationService& operations,
                                              ContentBrowserModel& model,
                                              std::string* error)
    {
        if (error) error->clear();
        if (!registry.initialized() || !operations.initialized())
        {
            fail(error, "Content Browser requires initialized registry and asset operations");
            return false;
        }
        registry_ = &registry;
        operations_ = &operations;
        model_ = &model;
        synchronize();
        return true;
    }

    bool ContentBrowserController::initialized() const noexcept
    {
        return registry_ && operations_ && model_;
    }

    void ContentBrowserController::synchronize()
    {
        if (initialized()) model_->synchronize(registry_->snapshot());
    }

    ContentBrowserActionResult ContentBrowserController::unavailable(std::string message) const
    {
        ContentBrowserActionResult result;
        result.error = std::move(message);
        return result;
    }

    ContentBrowserActionResult ContentBrowserController::publish(am::editor::assets::AssetOperationResult operation,
                                                                 bool selectResult)
    {
        ContentBrowserActionResult result;
        result.success = operation.success;
        result.path = operation.path;
        result.id = operation.id;
        result.error = std::move(operation.error);
        if (!result.success) return result;

        synchronize();
        if (selectResult)
        {
            if (result.id.isValid())
            {
                (void)model_->selectAsset(result.id);
            }
            else if (!result.path.empty())
            {
                (void)model_->selectFolder(result.path);
            }
        }
        return result;
    }

    ContentBrowserActionResult ContentBrowserController::createFolder(std::string_view name)
    {
        if (!initialized()) return unavailable("Content Browser is not initialized");
        return publish(operations_->createFolder(model_->currentFolder(), name), true);
    }

    ContentBrowserActionResult ContentBrowserController::renameSelection(std::string_view displayName)
    {
        if (!initialized()) return unavailable("Content Browser is not initialized");
        const auto selected = model_->selectedItems();
        if (selected.size() != 1) return unavailable("Rename requires exactly one selected item");
        return publish(operations_->rename(selected.front().path, displayName), true);
    }

    ContentBrowserActionResult ContentBrowserController::duplicateSelection(std::string_view displayName)
    {
        if (!initialized()) return unavailable("Content Browser is not initialized");
        const auto selected = model_->selectedItems();
        if (selected.size() != 1 || selected.front().kind != ItemKind::Asset)
            return unavailable("Duplicate requires exactly one selected asset");
        return publish(operations_->duplicateAsset(selected.front().path, displayName), true);
    }

    ContentBrowserActionResult ContentBrowserController::deleteSelection()
    {
        if (!initialized()) return unavailable("Content Browser is not initialized");
        const auto selected = model_->selectedItems();
        if (selected.size() != 1) return unavailable("Delete requires exactly one selected item");
        auto result = selected.front().kind == ItemKind::Asset ?
            operations_->deleteAsset(selected.front().path, false) :
            operations_->deleteEmptyFolder(selected.front().path);
        return publish(std::move(result), false);
    }

    ContentBrowserActionResult ContentBrowserController::undo()
    {
        if (!initialized()) return unavailable("Content Browser is not initialized");
        std::string error;
        if (!operations_->undo(&error)) return unavailable(std::move(error));
        model_->clearSelection();
        synchronize();
        ContentBrowserActionResult result;
        result.success = true;
        result.path = model_->currentFolder();
        return result;
    }

    ContentBrowserActionResult ContentBrowserController::redo()
    {
        if (!initialized()) return unavailable("Content Browser is not initialized");
        std::string error;
        if (!operations_->redo(&error)) return unavailable(std::move(error));
        model_->clearSelection();
        synchronize();
        ContentBrowserActionResult result;
        result.success = true;
        result.path = model_->currentFolder();
        return result;
    }
}
