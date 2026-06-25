#include "ArhqenCognitionEngine/AquariumRender/AceAquariumEmbeddedDx12Viewport.h"

#include "ArhqenCognitionEngine/AquariumUI/AceAquariumRuntimeController.h"

#include <algorithm>
#include <sstream>

namespace ace::aquarium_render
{
    namespace
    {
        am::renderer::UiColor color(float r, float g, float b, float a)
        {
            return {std::clamp(r, 0.0f, 1.0f), std::clamp(g, 0.0f, 1.0f), std::clamp(b, 0.0f, 1.0f), std::clamp(a, 0.0f, 1.0f)};
        }

        am::renderer::UiRect rectAround(const AceAqViewportPoint& p, float w, float h)
        {
            return {p.X - w * 0.5f, p.Y - h * 0.5f, w, h};
        }

        am::renderer::UiRect rectFromPoints(const AceAqViewportPoint& a, const AceAqViewportPoint& b, float minThickness)
        {
            const float left = std::min(a.X, b.X);
            const float top = std::min(a.Y, b.Y);
            const float right = std::max(a.X, b.X);
            const float bottom = std::max(a.Y, b.Y);
            return {left, top, std::max(minThickness, right - left), std::max(minThickness, bottom - top)};
        }
    }

    AceAquariumEmbeddedDx12Viewport::~AceAquariumEmbeddedDx12Viewport()
    {
        Destroy();
    }

    bool AceAquariumEmbeddedDx12Viewport::Show(HWND parent, int x, int y, int width, int height, std::string* error)
    {
        if (!parent)
        {
            if (error) { *error = "Embedded DX12 viewport requires a parent HWND."; }
            return false;
        }

        const int newX = std::max(0, x);
        const int newY = std::max(0, y);
        const int newWidth = std::max(1, width);
        const int newHeight = std::max(1, height);
        const bool existingWindow = hwnd_ != nullptr;
        const bool sizeChanged = existingWindow && (width_ != newWidth || height_ != newHeight);
        const bool rectChanged = !hwnd_ || x_ != newX || y_ != newY || width_ != newWidth || height_ != newHeight;

        parent_ = parent;
        x_ = newX;
        y_ = newY;
        width_ = newWidth;
        height_ = newHeight;

        // ACE-AQ3D6: viewport_resources_persistent. Resize resources only when
        // the embedded viewport size actually changes; idle hover/click repaint
        // must not reinitialize the DX12 renderer.
        if (sizeChanged)
        {
            QueueResize(newWidth, newHeight);
        }

        if (!EnsureWindow(parent, error))
        {
            if (error) { lastError_ = *error; }
            return false;
        }

        // ACE-AQ3D2: persistent child surface. Do not MoveWindow every frame;
        // doing so can trigger WM_SIZE and recreate DX12 resources, causing flicker.
        if (rectChanged)
        {
            // ACE-AQ3D5: stable child-window placement. Do not request an
            // immediate erase/redraw from MoveWindow on every layout pass; the
            // persistent DX12 renderer presents the next frame itself.
            if (resizeApplySuspended_)
            {
                ++childMoveDuringLiveResizeCount_;
            }
            ++childMoveCount_;
            SetWindowPos(
                hwnd_,
                nullptr,
                x_,
                y_,
                width_,
                height_,
                SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOCOPYBITS | SWP_NOREDRAW | SWP_NOSENDCHANGING
            );
        }

        if (!visible_)
        {
            ++childShowCount_;
            ShowWindow(hwnd_, SW_SHOWNA);
        }
        visible_ = true;

        if (!EnsureRenderer(error))
        {
            if (error) { lastError_ = *error; }
            return false;
        }

        return true;
    }

    void AceAquariumEmbeddedDx12Viewport::Hide()
    {
        if (visible_)
        {
            ++childHideCount_;
        }
        visible_ = false;
        if (hwnd_)
        {
            ShowWindow(hwnd_, SW_HIDE);
        }
    }

    void AceAquariumEmbeddedDx12Viewport::HideForLiveResize()
    {
        if (visible_)
        {
            ++childHideCount_;
        }
        visible_ = false;
        if (hwnd_)
        {
            // ACE-AQ3D10: stronger live-resize quarantine. Hiding alone can
            // leave the DWM/child clip region visually fighting the parent while
            // the user drags the top-level border. Move the child off-screen and
            // hide it without requesting an erase/redraw; the parent D2D proxy
            // owns the viewport rectangle until WM_EXITSIZEMOVE.
            SetWindowPos(
                hwnd_,
                nullptr,
                -32768,
                -32768,
                1,
                1,
                SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOCOPYBITS | SWP_NOREDRAW | SWP_NOSENDCHANGING | SWP_HIDEWINDOW
            );
        }
    }

    void AceAquariumEmbeddedDx12Viewport::Destroy()
    {
        if (rendererReady_)
        {
            renderer_.shutdown();
            rendererReady_ = false;
        }

        if (hwnd_)
        {
            HWND old = hwnd_;
            hwnd_ = nullptr;
            DestroyWindow(old);
        }

        visible_ = false;
        parent_ = nullptr;
    }

    bool AceAquariumEmbeddedDx12Viewport::EnsureWindow(HWND parent, std::string* error)
    {
        if (hwnd_)
        {
            return true;
        }

        instance_ = GetModuleHandleW(nullptr);

        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(wc);
        // ACE-AQ3D5: avoid class-level H/V redraw invalidations; resizing is
        // handled explicitly through Show/WM_SIZE so idle frames do not flicker.
        wc.style = 0;
        wc.lpfnWndProc = &AceAquariumEmbeddedDx12Viewport::WindowProc;
        wc.hInstance = instance_;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = nullptr;
        wc.lpszClassName = className_.c_str();

        if (!RegisterClassExW(&wc))
        {
            const auto err = GetLastError();
            if (err != ERROR_CLASS_ALREADY_EXISTS)
            {
                if (error)
                {
                    std::ostringstream msg;
                    msg << "RegisterClassExW failed. GetLastError=" << err;
                    *error = msg.str();
                }
                return false;
            }
        }

        hwnd_ = CreateWindowExW(
            0,
            className_.c_str(),
            L"",
            WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
            x_,
            y_,
            width_,
            height_,
            parent,
            nullptr,
            instance_,
            this
        );

        if (!hwnd_)
        {
            if (error)
            {
                std::ostringstream msg;
                msg << "CreateWindowExW child surface failed. GetLastError=" << GetLastError();
                *error = msg.str();
            }
            return false;
        }

        return true;
    }

    bool AceAquariumEmbeddedDx12Viewport::EnsureRenderer(std::string* error)
    {
        if (rendererReady_)
        {
            return true;
        }

        am::renderer::ClearColor clear{};
        clear.r = 0.012f;
        clear.g = 0.022f;
        clear.b = 0.046f;
        clear.a = 1.0f;

        if (!renderer_.initialize(hwnd_, width_, height_, clear, error))
        {
            return false;
        }

        rendererReady_ = true;
        ++initCount_;
        return true;
    }

    void AceAquariumEmbeddedDx12Viewport::RecreateRenderer()
    {
        if (resizeApplySuspended_)
        {
            ++rendererRecreateDuringLiveResizeCount_;
        }
        ++rendererRecreateCount_;
        if (rendererReady_)
        {
            renderer_.shutdown();
            rendererReady_ = false;
        }
    }

    void AceAquariumEmbeddedDx12Viewport::SetResizeApplySuspended(bool suspended)
    {
        resizeApplySuspended_ = suspended;
    }

    void AceAquariumEmbeddedDx12Viewport::QueueResize(int width, int height)
    {
        const int newWidth = std::max(1, width);
        const int newHeight = std::max(1, height);
        // ACE-AQ3D7: viewport_resize_is_debounced_or_pending. WM_SIZE and
        // parent sync only queue the resize; RenderFrame applies it once.
        pendingResize_ = true;
        pendingWidth_ = newWidth;
        pendingHeight_ = newHeight;
    }

    void AceAquariumEmbeddedDx12Viewport::ApplyPendingResizeIfNeeded()
    {
        if (!pendingResize_)
        {
            return;
        }

        // ACE-AQ3D8: live_resize_suspends_viewport_resize_apply. During a
        // top-level live resize transaction the parent updates pending dimensions
        // only. The embedded DX12 renderer recreates once after suspension ends.
        if (resizeApplySuspended_)
        {
            return;
        }

        pendingResize_ = false;
        const int newWidth = std::max(1, pendingWidth_);
        const int newHeight = std::max(1, pendingHeight_);
        width_ = newWidth;
        height_ = newHeight;
        ++resizeCount_;
        ++childResizeCount_;
        RecreateRenderer();
    }

    LRESULT CALLBACK AceAquariumEmbeddedDx12Viewport::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        AceAquariumEmbeddedDx12Viewport* self = nullptr;

        if (message == WM_NCCREATE)
        {
            const auto* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
            self = static_cast<AceAquariumEmbeddedDx12Viewport*>(createStruct->lpCreateParams);
            if (self)
            {
                self->hwnd_ = hwnd;
                SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            }
        }
        else
        {
            self = reinterpret_cast<AceAquariumEmbeddedDx12Viewport*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        }

        if (self)
        {
            return self->HandleMessage(message, wParam, lParam);
        }

        return DefWindowProcW(hwnd, message, wParam, lParam);
    }

    LRESULT AceAquariumEmbeddedDx12Viewport::HandleMessage(UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch (message)
        {
        case WM_SIZE:
        {
            const int newWidth = std::max(1, static_cast<int>(LOWORD(lParam)));
            const int newHeight = std::max(1, static_cast<int>(HIWORD(lParam)));
            if (newWidth != width_ || newHeight != height_)
            {
                QueueResize(newWidth, newHeight);
            }
            return 0;
        }

        case WM_ERASEBKGND:
            return 1;

        default:
            return DefWindowProcW(hwnd_, message, wParam, lParam);
        }
    }

    bool AceAquariumEmbeddedDx12Viewport::RenderFrame(
        const ace::aquarium_ui::AceAquariumRuntimeController& controller,
        const AceAquariumSceneAdapter& sceneAdapter,
        bool debugTruthEnabled,
        std::string* error
    )
    {
        if (!IsVisible())
        {
            return true;
        }

        ApplyPendingResizeIfNeeded();

        if (!EnsureRenderer(error))
        {
            if (error) { lastError_ = *error; }
            return false;
        }

        ++frameCount_;
        auto drawList = BuildDrawList(controller, sceneAdapter, debugTruthEnabled);
        if (!renderer_.renderFrame(drawList, error))
        {
            if (error) { lastError_ = *error; }
            return false;
        }

        return true;
    }

    am::renderer::UiDrawList AceAquariumEmbeddedDx12Viewport::BuildDrawList(
        const ace::aquarium_ui::AceAquariumRuntimeController& controller,
        const AceAquariumSceneAdapter& sceneAdapter,
        bool debugTruthEnabled
    )
    {
        am::renderer::UiDrawList drawList;
        const float surfaceW = static_cast<float>(std::max(1, width_));
        const float surfaceH = static_cast<float>(std::max(1, height_));

        drawList.addRectPixels({0.0f, 0.0f, surfaceW, surfaceH}, color(0.010f, 0.020f, 0.044f, 1.0f), surfaceW, surfaceH);
        drawList.addRectPixels({0.0f, 0.0f, surfaceW, 28.0f}, debugTruthEnabled ? color(0.52f, 0.20f, 0.06f, 0.92f) : color(0.020f, 0.060f, 0.110f, 0.90f), surfaceW, surfaceH);

        const auto primitives = sceneAdapter.BuildPrimitives(controller, debugTruthEnabled);

        float maxX = 1.0f;
        float maxY = 1.0f;
        for (const auto& primitive : primitives)
        {
            maxX = std::max(maxX, primitive.X + std::max(0.0f, primitive.SizeX));
            maxY = std::max(maxY, primitive.Y + std::max(0.0f, primitive.SizeY));
        }

        auto camera = camera_;
        camera.Zoom = std::clamp(std::min(surfaceW / std::max(maxX + maxY + 2.0f, 1.0f), (surfaceH - 44.0f) / std::max((maxX + maxY) * 0.62f + 3.0f, 1.0f)) * 2.55f, 22.0f, 92.0f);
        camera.OriginY = std::max(18.0f, surfaceH * 0.045f);

        const auto model = viewport_.BuildViewportModel(primitives, camera, surfaceW, surfaceH);
        const float tileW = std::clamp(camera.Zoom * 0.86f, 28.0f, 76.0f);
        const float tileH = std::clamp(camera.Zoom * 0.42f, 14.0f, 38.0f);
        const float blockW = std::clamp(camera.Zoom * 0.72f, 28.0f, 70.0f);
        const float blockH = std::clamp(camera.Zoom * 1.04f, 36.0f, 96.0f);
        const float agentW = std::clamp(camera.Zoom * 0.92f, 40.0f, 86.0f);
        const float agentH = std::clamp(camera.Zoom * 1.22f, 52.0f, 116.0f);

        for (const auto& primitive : model)
        {
            const auto c = color(primitive.R, primitive.G, primitive.Bc, primitive.Aalpha);

            switch (primitive.Kind)
            {
            case AceAqRenderPrimitiveKind::GridLine:
                drawList.addRectPixels(rectFromPoints(primitive.A, primitive.B, 1.0f), c, surfaceW, surfaceH);
                break;

            case AceAqRenderPrimitiveKind::Tile:
                drawList.addRectPixels(rectAround(primitive.A, tileW, tileH), c, surfaceW, surfaceH);
                break;

            case AceAqRenderPrimitiveKind::Block:
                drawList.addRectPixels(rectAround(primitive.A, blockW, blockH), c, surfaceW, surfaceH);
                drawList.addRectPixels({primitive.A.X - blockW * 0.35f, primitive.A.Y - blockH * 0.56f, blockW * 0.70f, std::max(4.0f, blockH * 0.14f)}, color(primitive.R + 0.12f, primitive.G + 0.12f, primitive.Bc + 0.12f, primitive.Aalpha), surfaceW, surfaceH);
                break;

            case AceAqRenderPrimitiveKind::Agent:
                drawList.addRectPixels(rectAround(primitive.A, agentW * 1.42f, agentH * 1.16f), color(0.08f, 0.50f, 0.72f, 0.22f), surfaceW, surfaceH);
                drawList.addRectPixels(rectAround(primitive.A, agentW, agentH), color(0.10f, 0.95f, 1.0f, 0.92f), surfaceW, surfaceH);
                break;

            case AceAqRenderPrimitiveKind::DirectionArrow:
                drawList.addRectPixels(rectFromPoints(primitive.A, primitive.B, std::max(4.0f, camera.Zoom * 0.08f)), color(1.0f, 0.90f, 0.20f, 0.92f), surfaceW, surfaceH);
                drawList.addRectPixels(rectAround(primitive.B, std::max(8.0f, camera.Zoom * 0.16f), std::max(8.0f, camera.Zoom * 0.16f)), color(1.0f, 0.90f, 0.20f, 0.92f), surfaceW, surfaceH);
                break;

            case AceAqRenderPrimitiveKind::Highlight:
                drawList.addRectPixels(rectAround(primitive.A, tileW * 1.16f, tileH * 1.55f), color(1.0f, 0.82f, 0.20f, 0.34f), surfaceW, surfaceH);
                break;

            case AceAqRenderPrimitiveKind::DebugLabel:
                if (debugTruthEnabled)
                {
                    drawList.addRectPixels(rectAround(primitive.A, 18.0f, 6.0f), color(1.0f, 0.55f, 0.18f, 0.90f), surfaceW, surfaceH);
                }
                break;
            }
        }

        return drawList;
    }
}
