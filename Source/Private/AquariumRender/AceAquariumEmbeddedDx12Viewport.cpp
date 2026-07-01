#include "ArhqenCognitionEngine/AquariumRender/AceAquariumEmbeddedDx12Viewport.h"

#include "ArhqenCognitionEngine/AquariumUI/AceAquariumRuntimeController.h"

#include <windowsx.h>

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

        am::renderer::Scene3DColor sceneColor(float r, float g, float b, float a)
        {
            return {std::clamp(r, 0.0f, 1.0f), std::clamp(g, 0.0f, 1.0f), std::clamp(b, 0.0f, 1.0f), std::clamp(a, 0.0f, 1.0f)};
        }

        am::renderer::Scene3DVec3 sceneVec(float x, float y, float z)
        {
            return {x, y, z};
        }

        float sceneY(const AceAqRenderPrimitive& primitive)
        {
            return primitive.Z;
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

        case WM_RBUTTONDOWN:
            mouseLookActive_ = true;
            lastMouseX_ = GET_X_LPARAM(lParam);
            lastMouseY_ = GET_Y_LPARAM(lParam);
            SetCapture(hwnd_);
            SetFocus(hwnd_);
            return 0;

        case WM_RBUTTONUP:
            mouseLookActive_ = false;
            if (GetCapture() == hwnd_)
            {
                ReleaseCapture();
            }
            return 0;

        case WM_MOUSEMOVE:
            if (mouseLookActive_)
            {
                const int x = GET_X_LPARAM(lParam);
                const int y = GET_Y_LPARAM(lParam);
                realCamera_.ApplyMouseDelta(static_cast<float>(x - lastMouseX_), static_cast<float>(y - lastMouseY_));
                lastMouseX_ = x;
                lastMouseY_ = y;
                return 0;
            }
            break;

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
        float deltaSeconds,
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
        UpdateCameraInput(deltaSeconds);

        auto scene = BuildScene3D(controller, sceneAdapter, debugTruthEnabled);
        auto overlay = BuildOverlay(debugTruthEnabled);
        am::renderer::Scene3DConstants constants{};
        const float aspect = static_cast<float>(std::max(1, width_)) / static_cast<float>(std::max(1, height_));
        const auto viewProjection = realCamera_.ViewProjectionMatrix(aspect);
        for (std::size_t i = 0; i < 16; ++i)
        {
            constants.viewProjection[i] = viewProjection.m[i];
        }

        if (!renderer_.renderFrame3D(scene, constants, overlay, error))
        {
            if (error) { lastError_ = *error; }
            return false;
        }

        return true;
    }


    void AceAquariumEmbeddedDx12Viewport::UpdateCameraInput(float deltaSeconds)
    {
        AceAqCameraInput input{};
        input.moveForward = (GetAsyncKeyState('W') & 0x8000) != 0;
        input.moveBackward = (GetAsyncKeyState('S') & 0x8000) != 0;
        input.moveLeft = (GetAsyncKeyState('A') & 0x8000) != 0;
        input.moveRight = (GetAsyncKeyState('D') & 0x8000) != 0;
        input.moveDown = (GetAsyncKeyState('Q') & 0x8000) != 0;
        input.moveUp = (GetAsyncKeyState('E') & 0x8000) != 0;
        realCamera_.UpdateFromInput(input, deltaSeconds);
    }

    am::renderer::UiDrawList AceAquariumEmbeddedDx12Viewport::BuildOverlay(bool debugTruthEnabled)
    {
        am::renderer::UiDrawList overlay;
        const float surfaceW = static_cast<float>(std::max(1, width_));
        const float surfaceH = static_cast<float>(std::max(1, height_));

        overlay.addRectPixels({0.0f, 0.0f, surfaceW, 28.0f}, debugTruthEnabled ? color(0.52f, 0.20f, 0.06f, 0.84f) : color(0.020f, 0.060f, 0.110f, 0.74f), surfaceW, surfaceH);
        return overlay;
    }

    am::renderer::Scene3DDrawList AceAquariumEmbeddedDx12Viewport::BuildScene3D(
        const ace::aquarium_ui::AceAquariumRuntimeController& controller,
        const AceAquariumSceneAdapter& sceneAdapter,
        bool debugTruthEnabled
    )
    {
        (void)controller;
        am::renderer::Scene3DDrawList scene;
        const auto primitives = sceneAdapter.BuildPrimitives(controller, debugTruthEnabled);

        float maxX = 10.0f;
        float maxZ = 10.0f;
        for (const auto& primitive : primitives)
        {
            maxX = std::max(maxX, primitive.X + std::max(primitive.SizeX, 0.0f));
            maxZ = std::max(maxZ, primitive.Y + std::max(primitive.SizeY, 0.0f));
        }

        scene.addGroundGrid(-1.0f, maxX + 1.0f, -1.0f, maxZ + 1.0f, 0.0f, 0.012f, sceneColor(0.12f, 0.58f, 0.74f, 0.32f));

        for (const auto& primitive : primitives)
        {
            const float x0 = primitive.X;
            const float z0 = primitive.Y;
            const float x1 = primitive.X + std::max(primitive.SizeX, 0.04f);
            const float z1 = primitive.Y + std::max(primitive.SizeY, 0.04f);
            const float y0 = std::max(0.0f, sceneY(primitive));
            const float h = std::max(0.03f, primitive.SizeZ);
            const auto c = sceneColor(primitive.R, primitive.G, primitive.B, primitive.A);

            switch (primitive.Kind)
            {
            case AceAqRenderPrimitiveKind::GridLine:
                break;

            case AceAqRenderPrimitiveKind::Tile:
                scene.addBox(sceneVec(x0, 0.0f, z0), sceneVec(x1, 0.035f, z1), c);
                break;

            case AceAqRenderPrimitiveKind::Block:
                scene.addBox(sceneVec(x0, y0, z0), sceneVec(x1, y0 + std::max(0.20f, h), z1), c);
                break;

            case AceAqRenderPrimitiveKind::Agent:
                scene.addBox(sceneVec(x0, 0.05f, z0), sceneVec(x1, 0.90f, z1), sceneColor(0.10f, 0.95f, 1.0f, 0.96f));
                break;

            case AceAqRenderPrimitiveKind::DirectionArrow:
            {
                const float ex = primitive.X + primitive.SizeX;
                const float ez = primitive.Y + primitive.SizeY;
                const float minX = std::min(primitive.X, ex) - 0.045f;
                const float maxX2 = std::max(primitive.X, ex) + 0.045f;
                const float minZ = std::min(primitive.Y, ez) - 0.045f;
                const float maxZ2 = std::max(primitive.Y, ez) + 0.045f;
                scene.addBox(sceneVec(minX, 0.82f, minZ), sceneVec(maxX2, 0.90f, maxZ2), sceneColor(1.0f, 0.90f, 0.20f, 0.95f));
                scene.addBox(sceneVec(ex - 0.10f, 0.88f, ez - 0.10f), sceneVec(ex + 0.10f, 1.04f, ez + 0.10f), sceneColor(1.0f, 0.90f, 0.20f, 0.95f));
                break;
            }

            case AceAqRenderPrimitiveKind::Highlight:
                scene.addBox(sceneVec(x0, 0.06f, z0), sceneVec(x1, 0.10f, z1), sceneColor(1.0f, 0.78f, 0.16f, 0.52f));
                break;

            case AceAqRenderPrimitiveKind::DebugLabel:
                if (debugTruthEnabled)
                {
                    scene.addBox(sceneVec(x0 - 0.08f, y0 + 1.0f, z0 - 0.08f), sceneVec(x0 + 0.08f, y0 + 1.10f, z0 + 0.08f), sceneColor(1.0f, 0.55f, 0.18f, 0.96f));
                }
                break;
            }
        }

        return scene;
    }
}
