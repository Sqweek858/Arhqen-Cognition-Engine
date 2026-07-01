#pragma once

#if !defined(_WIN32)
#error AceAquariumEmbeddedDx12Viewport is Windows-only.
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "ArhqenCognitionEngine/AquariumRender/AceAquariumCamera.h"
#include "ArhqenCognitionEngine/AquariumRender/AceAquariumSceneAdapter.h"
#include "ArhqenCognitionEngine/AquariumRender/AceAquariumViewport.h"
#include "ArhqenCognitionEngine/Renderer/Dx12Renderer.h"
#include "ArhqenCognitionEngine/Renderer/UiDrawList.h"

#include <Windows.h>

#include <string>

namespace ace::aquarium_ui
{
    class AceAquariumRuntimeController;
}

namespace ace::aquarium_render
{
    class AceAquariumEmbeddedDx12Viewport
    {
    public:
        AceAquariumEmbeddedDx12Viewport() = default;
        ~AceAquariumEmbeddedDx12Viewport();

        AceAquariumEmbeddedDx12Viewport(const AceAquariumEmbeddedDx12Viewport&) = delete;
        AceAquariumEmbeddedDx12Viewport& operator=(const AceAquariumEmbeddedDx12Viewport&) = delete;

        bool Show(HWND parent, int x, int y, int width, int height, std::string* error);
        void Hide();
        void HideForLiveResize();
        void Destroy();
        bool IsVisible() const { return visible_ && hwnd_ != nullptr; }

        bool RenderFrame(
            const ace::aquarium_ui::AceAquariumRuntimeController& controller,
            const AceAquariumSceneAdapter& sceneAdapter,
            bool debugTruthEnabled,
            float deltaSeconds,
            std::string* error
        );

        int X() const { return x_; }
        int Y() const { return y_; }
        int Width() const { return width_; }
        int Height() const { return height_; }
        const std::string& LastError() const { return lastError_; }
        int InitCount() const { return initCount_; }
        int ResizeCount() const { return resizeCount_; }
        int FrameCount() const { return frameCount_; }
        int ChildShowCount() const { return childShowCount_; }
        int ChildHideCount() const { return childHideCount_; }
        int ChildMoveCount() const { return childMoveCount_; }
        int ChildResizeCount() const { return childResizeCount_; }
        int RendererRecreateCount() const { return rendererRecreateCount_; }
        int RendererRecreateDuringLiveResizeCount() const { return rendererRecreateDuringLiveResizeCount_; }
        int ChildMoveDuringLiveResizeCount() const { return childMoveDuringLiveResizeCount_; }
        void SetResizeApplySuspended(bool suspended);
        bool IsResizeApplySuspended() const { return resizeApplySuspended_; }
        bool HasPendingResize() const { return pendingResize_; }

    private:
        static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
        LRESULT HandleMessage(UINT message, WPARAM wParam, LPARAM lParam);

        bool EnsureWindow(HWND parent, std::string* error);
        bool EnsureRenderer(std::string* error);
        void RecreateRenderer();
        void QueueResize(int width, int height);
        void ApplyPendingResizeIfNeeded();
        am::renderer::Scene3DDrawList BuildScene3D(
            const ace::aquarium_ui::AceAquariumRuntimeController& controller,
            const AceAquariumSceneAdapter& sceneAdapter,
            bool debugTruthEnabled
        );
        am::renderer::UiDrawList BuildOverlay(bool debugTruthEnabled);
        void UpdateCameraInput(float deltaSeconds);

        HWND parent_ = nullptr;
        HWND hwnd_ = nullptr;
        HINSTANCE instance_ = nullptr;
        std::wstring className_ = L"ArhqenCognitionEngineAquariumEmbeddedDx12Viewport";

        int x_ = 0;
        int y_ = 0;
        int width_ = 1;
        int height_ = 1;
        bool visible_ = false;
        bool rendererReady_ = false;
        bool resizeApplySuspended_ = false;
        bool pendingResize_ = false;
        int pendingWidth_ = 0;
        int pendingHeight_ = 0;
        int initCount_ = 0;
        int resizeCount_ = 0;
        int frameCount_ = 0;
        int childShowCount_ = 0;
        int childHideCount_ = 0;
        int childMoveCount_ = 0;
        int childResizeCount_ = 0;
        int rendererRecreateCount_ = 0;
        int rendererRecreateDuringLiveResizeCount_ = 0;
        int childMoveDuringLiveResizeCount_ = 0;
        std::string lastError_;

        am::renderer::Dx12Renderer renderer_{};
        AceAquariumViewport viewport_{};
        AceAquariumCamera camera_{};
        AceAquariumRealCamera realCamera_{};
        bool mouseLookActive_ = false;
        int lastMouseX_ = 0;
        int lastMouseY_ = 0;
    };
}
