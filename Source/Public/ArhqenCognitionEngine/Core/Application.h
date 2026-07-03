#pragma once

#include "ArhqenCognitionEngine/Core/AppConfig.h"
#include "ArhqenCognitionEngine/Core/Assets/AceAssetRegistry.h"
#include "ArhqenCognitionEngine/Core/Assets/AceAssetDirectoryWatcher.h"
#include "ArhqenCognitionEngine/Core/Assets/AceAssetReferences.h"
#include "ArhqenCognitionEngine/Editor/Assets/AceAssetOperationService.h"
#include "ArhqenCognitionEngine/Editor/ContentBrowser/AceContentBrowserModel.h"
#include "ArhqenCognitionEngine/Editor/ContentBrowser/AceContentBrowserController.h"
#include "ArhqenCognitionEngine/Editor/Transactions/AceTransaction.h"
#include "ArhqenCognitionEngine/Core/ExitCode.h"
#include "ArhqenCognitionEngine/Core/Logger.h"
#include "ArhqenCognitionEngine/Core/Scene/AceSceneWorld.h"
#include "ArhqenCognitionEngine/Core/RuntimeClock.h"
#include "ArhqenCognitionEngine/Core/TaskQueue.h"
#include "ArhqenCognitionEngine/Renderer/Dx12Renderer.h"
#include "ArhqenCognitionEngine/Renderer/NativeWindow.h"
#include "ArhqenCognitionEngine/Ui/AceShellUi.h"

#include <filesystem>
#include <string>

namespace am::core
{
    class Application
    {
    public:
        Application() = default;

        Application(const Application&) = delete;
        Application& operator=(const Application&) = delete;

        ExitCode run();

    private:
        bool boot();
        bool initializeRuntime();
        bool updateRuntime(const FrameTiming& timing);
        void shutdownRuntime();
        void requestExit(const std::string& reason);

        std::filesystem::path repoRoot_;
        std::filesystem::path configPath_;
        AppConfig config_;
        Logger logger_;
        RuntimeClock clock_;
        TaskQueue tasks_;
        am::core::assets::AssetRegistry assetRegistry_;
        am::core::assets::AssetDirectoryWatcher assetDirectoryWatcher_;
        am::core::assets::AssetReferenceIndex assetReferences_;
        am::editor::transactions::TransactionManager editorTransactions_{};
        am::editor::assets::AssetOperationService assetOperations_{};
        am::editor::content_browser::ContentBrowserModel contentBrowserModel_{};
        am::editor::content_browser::ContentBrowserController contentBrowserController_{};
        am::core::scene::SceneWorld editorScene_{};
        am::core::scene::SceneSelection editorSceneSelection_{};
        am::core::scene::SceneHierarchyModel editorSceneHierarchy_{};
        am::renderer::NativeWindow window_;
        am::renderer::Dx12Renderer renderer_;
        am::renderer::UiDrawList uiDrawList_;
        am::ui::AceShellUi shellUi_;

        bool rendererEnabled_ = false;
        int runtimeSleepMilliseconds_ = 0;
        int runtimeIdleWaitMilliseconds_ = 8;
        int maxFrames_ = 0;
        bool loggerReady_ = false;
        bool exitRequested_ = false;
        std::string exitReason_;
        bool assetChangesPending_ = false;
        bool assetFullRescanPending_ = false;
        double assetChangeQuietSeconds_ = 0.0;
        double assetChangePendingAgeSeconds_ = 0.0;
    };
}
