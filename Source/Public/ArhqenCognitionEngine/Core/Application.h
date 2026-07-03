#pragma once

#include "ArhqenCognitionEngine/Core/AppConfig.h"
#include "ArhqenCognitionEngine/Core/Assets/AceAssetRegistry.h"
#include "ArhqenCognitionEngine/Core/ExitCode.h"
#include "ArhqenCognitionEngine/Core/Logger.h"
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
    };
}
