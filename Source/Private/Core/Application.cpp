#include "ArhqenCognitionEngine/Core/Application.h"

#include "ArhqenCognitionEngine/Core/PathUtils.h"
#include "ArhqenCognitionEngine/Ui/AceEngineConsole.h"

#include <Windows.h>

#include <algorithm>
#include <exception>
#include <iostream>
#include <string>

namespace am::core
{
    ExitCode Application::run()
    {
        try
        {
            if (!boot())
            {
                return ExitCode::MissingConfig;
            }

            if (!initializeRuntime())
            {
                return ExitCode::RuntimeFailure;
            }

            clock_.reset();

            while (!exitRequested_)
            {
                const auto timing = clock_.tick();

                if (!updateRuntime(timing))
                {
                    requestExit("runtime update failure");
                    shutdownRuntime();
                    return ExitCode::RuntimeFailure;
                }

                if (maxFrames_ > 0 && static_cast<int>(timing.frameIndex + 1) >= maxFrames_)
                {
                    requestExit("configured max frame count reached");
                }
            }

            shutdownRuntime();
            return ExitCode::Success;
        }
        catch (const std::exception& ex)
        {
            if (loggerReady_)
            {
                logger_.error(std::string("Unhandled runtime exception: ") + ex.what());
            }
            else
            {
                std::cerr << "[ACE][BOOT][ERROR] " << ex.what() << "\n";
            }

            return ExitCode::RuntimeFailure;
        }
        catch (...)
        {
            if (loggerReady_)
            {
                logger_.error("Unhandled unknown runtime exception.");
            }
            else
            {
                std::cerr << "[ACE][BOOT][ERROR] unknown exception\n";
            }

            return ExitCode::RuntimeFailure;
        }
    }

    bool Application::boot()
    {
        const auto root = findRepoRootFrom(std::filesystem::current_path());

        if (!root.has_value())
        {
            std::cerr << "[ACE][BOOT][ERROR] Missing config file: Config\\app.amconfig\n";
            return false;
        }

        repoRoot_ = *root;
        configPath_ = configPathFromRepoRoot(repoRoot_);
        config_ = AppConfig::loadFromFile(configPath_);

        if (!config_.loaded())
        {
            std::cerr << "[ACE][BOOT][ERROR] " << config_.error() << "\n";
            return false;
        }

        am::ui::AceEngineSetLogPath(
            (repoRoot_ / config_.getString("engine_log.path", "Build/Logs/ace_engine.log")).lexically_normal());

        maxFrames_ = std::max(0, config_.getInt("runtime.max_frames", 0));
        runtimeSleepMilliseconds_ = std::max(0, config_.getInt("runtime.sleep_ms", 0));
        runtimeIdleWaitMilliseconds_ = std::max(0, config_.getInt("runtime.idle_wait_ms", 8));
        rendererEnabled_ = config_.getInt("renderer.enabled", 0) != 0;

        const auto logPath = (repoRoot_ / config_.getString("log.path", "Build/Logs/ace.log")).lexically_normal();

        std::string logError;
        if (!logger_.open(logPath, &logError))
        {
            std::cerr << "[ACE][BOOT][ERROR] " << logError << "\n";
            return false;
        }

        loggerReady_ = true;

        const auto appName = config_.getString("app.name", "Arhqen Cognition Engine");
        const auto appVersion = config_.getString("app.version", "unknown");

        logger_.info(appName + " " + appVersion + " booted.");
        logger_.info("ACE-CLEAN0 shell booted successfully.");
        logger_.info("RepoRoot: " + repoRoot_.string());
        logger_.info("Config: " + configPath_.string());

        std::cout << "Arhqen Cognition Engine booted\n";
        std::cout << "RepoRoot: " << repoRoot_.string() << "\n";
        std::cout << "Config: " << configPath_.string() << "\n";

        return true;
    }

    bool Application::initializeRuntime()
    {
        logger_.info("Runtime initialize begin.");
        logger_.info("Runtime max frames: " + std::to_string(maxFrames_));

        const auto contentRoot = (repoRoot_ / "Content").lexically_normal();
        const auto assetRegistryState = (repoRoot_ / "Build/Editor/ace-asset-registry.acebin").lexically_normal();
        std::string assetRegistryError;
        if (!assetRegistry_.initialize(contentRoot, assetRegistryState, &assetRegistryError))
        {
            logger_.error("Asset Registry initialization failed: " + assetRegistryError);
            return false;
        }
        if (!assetRegistry_.lastWarning().empty())
        {
            logger_.warning("Asset Registry recovered state: " + assetRegistry_.lastWarning());
        }
        logger_.info("Asset Registry mounted /Game at " + contentRoot.string() +
            " assets=" + std::to_string(assetRegistry_.snapshot().assets.size()) +
            " folders=" + std::to_string(assetRegistry_.snapshot().folders.size()));
        const auto assetUndoRoot = (repoRoot_ / "Build/Editor/AssetUndo").lexically_normal();
        if (!assetOperations_.initialize(assetRegistry_, assetReferences_, editorTransactions_, assetUndoRoot, &assetRegistryError))
        {
            logger_.error("Asset Operation Service initialization failed: " + assetRegistryError);
            return false;
        }
        logger_.info("Asset Operation Service initialized with external undo storage.");
        if (!assetDirectoryWatcher_.start(contentRoot, &assetRegistryError))
        {
            logger_.error("Asset Directory Watcher initialization failed: " + assetRegistryError);
            return false;
        }
        logger_.info("Asset Directory Watcher active on /Game subtree.");

        const auto windowTitle = config_.getString("window.title", "Arhqen Cognition Engine");
        const auto windowWidth = std::max(320, config_.getInt("window.width", 1280));
        const auto windowHeight = std::max(240, config_.getInt("window.height", 720));

        std::string error;
        std::wstring wideTitle(windowTitle.begin(), windowTitle.end());

        if (!window_.create(wideTitle, windowWidth, windowHeight, &error))
        {
            logger_.error("Window creation failed: " + error);
            return false;
        }

        logger_.info("Native window created.");

        if (rendererEnabled_)
        {
            am::renderer::ClearColor clearColor;
            clearColor.r = config_.getFloat("renderer.clear.r", 0.02f);
            clearColor.g = config_.getFloat("renderer.clear.g", 0.04f);
            clearColor.b = config_.getFloat("renderer.clear.b", 0.07f);
            clearColor.a = config_.getFloat("renderer.clear.a", 1.0f);

            if (!renderer_.initialize(window_.hwnd(), window_.width(), window_.height(), clearColor, &error))
            {
                logger_.error("DX12 renderer initialization failed: " + error);
                return false;
            }

            logger_.info("DX12 renderer initialized successfully.");
        }
        else
        {
            logger_.info("DX12 renderer disabled; Direct2D/DirectWrite shell mode active.");
        }

        window_.setCommandHandler([this](WPARAM, LPARAM)
        {
            return false;
        });

        window_.setResizeHandler([this](int newWidth, int newHeight)
        {
            shellUi_.layout(newWidth, newHeight);
        });

        window_.setMessageHandler([this](UINT message, WPARAM wParam, LPARAM lParam, bool* handled)
        {
            return shellUi_.handleWindowMessage(message, wParam, lParam, handled);
        });

        const auto layoutProfilePath = (repoRoot_ / config_.getString("ui.layout.path", "Build/Ui/ace_clean0.layout")).lexically_normal();
        shellUi_.setLayoutProfilePath(layoutProfilePath);
        shellUi_.setVsyncEnabled(config_.getInt("ui.vsync", 0) != 0);

        if (!shellUi_.create(window_.hwnd(), window_.width(), window_.height(), &error))
        {
            logger_.error("ACE shell UI creation failed: " + error);
            return false;
        }

        logger_.info("ACE shell UI created.");

        tasks_.enqueue("ace_clean0_startup_probe", [this]()
        {
            logger_.info("ACE-CLEAN0 startup probe executed. Legacy AI backend is absent.");
        });

        logger_.info("Runtime initialize complete.");
        return true;
    }

    bool Application::updateRuntime(const FrameTiming& timing)
    {
        shellUi_.setRuntimeFrameDeltaSeconds(timing.deltaSeconds);

        const auto assetChanges = assetDirectoryWatcher_.drainChanges();
        if (!assetChanges.empty())
        {
            assetChangesPending_ = true;
            assetChangeQuietSeconds_ = 0.08;
            for (const auto& change : assetChanges)
            {
                if (change.action == am::core::assets::AssetFileChangeAction::RescanRequired)
                    assetFullRescanPending_ = true;
            }
        }
        if (assetChangesPending_)
        {
            const double delta = std::clamp(timing.deltaSeconds, 0.0, 0.10);
            assetChangeQuietSeconds_ -= delta;
            assetChangePendingAgeSeconds_ += delta;
            if (assetChangeQuietSeconds_ <= 0.0 || assetChangePendingAgeSeconds_ >= 0.50)
            {
                std::string registryError;
                if (!assetRegistry_.rescan(&registryError, assetFullRescanPending_))
                {
                    logger_.error("Asset Registry live update failed: " + registryError);
                    return false;
                }
                const auto& deltaResult = assetRegistry_.lastDelta();
                if (!deltaResult.empty() || deltaResult.fullRescan)
                {
                    logger_.info("Asset Registry delta generation=" + std::to_string(deltaResult.generation) +
                        " added=" + std::to_string(deltaResult.added.size()) +
                        " modified=" + std::to_string(deltaResult.modified.size()) +
                        " removed=" + std::to_string(deltaResult.removed.size()) +
                        " full=" + std::to_string(deltaResult.fullRescan ? 1 : 0));
                }
                assetChangesPending_ = false;
                assetFullRescanPending_ = false;
                assetChangeQuietSeconds_ = 0.0;
                assetChangePendingAgeSeconds_ = 0.0;
            }
        }

        if (timing.frameIndex < 3 || timing.frameIndex % 300 == 0)
        {
            logger_.info(
                "Runtime update frame=" + std::to_string(timing.frameIndex) +
                " delta=" + std::to_string(timing.deltaSeconds) +
                " elapsed=" + std::to_string(timing.elapsedSeconds)
            );
        }

        if (!window_.pumpMessages())
        {
            requestExit("window close requested");
            return true;
        }

        if (!tasks_.drain(logger_))
        {
            return false;
        }

        shellUi_.tick(static_cast<float>(timing.deltaSeconds));
        shellUi_.flushPendingPaint();

        if (rendererEnabled_)
        {
            uiDrawList_.clear();

            const float width = static_cast<float>(window_.width());
            const float height = static_cast<float>(window_.height());

            uiDrawList_.addRectPixels({0.0f, 0.0f, width, height}, {0.025f, 0.030f, 0.055f, 1.0f}, width, height);

            std::string renderError;
            if (!renderer_.renderFrame(uiDrawList_, &renderError))
            {
                logger_.error("DX12 render frame failed: " + renderError);
                return false;
            }

            if (timing.frameIndex < 3 || timing.frameIndex % 300 == 0)
            {
                logger_.info("DX12 optional background frame rendered. vertices=" + std::to_string(uiDrawList_.vertexCount()));
            }
        }

        if (runtimeSleepMilliseconds_ > 0)
        {
            Sleep(static_cast<DWORD>(runtimeSleepMilliseconds_));
        }
        else if (runtimeIdleWaitMilliseconds_ > 0 && !shellUi_.wantsUnthrottledTick())
        {
            MsgWaitForMultipleObjectsEx(
                0,
                nullptr,
                static_cast<DWORD>(runtimeIdleWaitMilliseconds_),
                QS_ALLINPUT,
                MWMO_INPUTAVAILABLE);
        }
        return true;
    }

    void Application::shutdownRuntime()
    {
        logger_.info("Runtime shutdown begin.");
        assetDirectoryWatcher_.stop();
        const auto watcherStats = assetDirectoryWatcher_.stats();
        logger_.info("Asset Directory Watcher stopped. batches=" + std::to_string(watcherStats.nativeBatches) +
            " queued=" + std::to_string(watcherStats.queuedEvents) +
            " delivered=" + std::to_string(watcherStats.deliveredEvents) +
            " dropped=" + std::to_string(watcherStats.droppedEvents) +
            " rescans=" + std::to_string(watcherStats.rescanSignals));
        if (rendererEnabled_)
        {
            renderer_.shutdown();
            logger_.info("DX12 renderer shutdown complete.");
        }
        else
        {
            logger_.info("DX12 renderer was disabled; no renderer shutdown needed.");
        }
        logger_.info("Tasks executed: " + std::to_string(tasks_.executedCount()));
        logger_.info("Exit reason: " + exitReason_);
        logger_.info("Arhqen Cognition Engine runtime loop completed.");
        logger_.info("Runtime shutdown complete.");

        std::cout << "Arhqen Cognition Engine runtime loop completed\n";
    }

    void Application::requestExit(const std::string& reason)
    {
        if (!exitRequested_)
        {
            exitRequested_ = true;
            exitReason_ = reason;
            logger_.info("Runtime exit requested: " + reason);
        }
    }
}
