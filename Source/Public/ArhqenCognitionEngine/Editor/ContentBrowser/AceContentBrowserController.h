#pragma once

#include "ArhqenCognitionEngine/Editor/Assets/AceAssetOperationService.h"
#include "ArhqenCognitionEngine/Editor/ContentBrowser/AceContentBrowserModel.h"

#include <string>
#include <string_view>

namespace am::editor::content_browser
{
    struct ContentBrowserActionResult final
    {
        bool success = false;
        am::core::assets::AssetPath path{};
        am::core::Guid id{};
        std::string error;

        explicit operator bool() const noexcept { return success; }
    };

    class ContentBrowserController final
    {
    public:
        bool initialize(am::core::assets::AssetRegistry& registry,
                        am::editor::assets::AssetOperationService& operations,
                        ContentBrowserModel& model,
                        std::string* error = nullptr);

        [[nodiscard]] bool initialized() const noexcept;
        [[nodiscard]] ContentBrowserModel* model() noexcept { return model_; }
        [[nodiscard]] const ContentBrowserModel* model() const noexcept { return model_; }

        void synchronize();
        ContentBrowserActionResult createFolder(std::string_view name);
        ContentBrowserActionResult renameSelection(std::string_view displayName);
        ContentBrowserActionResult duplicateSelection(std::string_view displayName);
        ContentBrowserActionResult deleteSelection();
        ContentBrowserActionResult undo();
        ContentBrowserActionResult redo();

    private:
        [[nodiscard]] ContentBrowserActionResult unavailable(std::string message) const;
        ContentBrowserActionResult publish(am::editor::assets::AssetOperationResult result,
                                           bool selectResult);

        am::core::assets::AssetRegistry* registry_ = nullptr;
        am::editor::assets::AssetOperationService* operations_ = nullptr;
        ContentBrowserModel* model_ = nullptr;
    };
}
