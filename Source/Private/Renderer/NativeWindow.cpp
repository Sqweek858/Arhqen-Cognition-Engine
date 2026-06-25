#include "ArhqenCognitionEngine/Renderer/NativeWindow.h"

#include "ArhqenCognitionEngine/Resources/Resource.h"

#include <dwmapi.h>

#include <sstream>
#include <utility>

#pragma comment(lib, "dwmapi.lib")

namespace am::renderer
{
    namespace
    {
        constexpr wchar_t kWindowClassName[] = L"ArhqenCognitionEngineM3WindowClass";

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 reinterpret_cast<DPI_AWARENESS_CONTEXT>(-4)
#endif

        void enableDarkTitleBar(HWND hwnd)
        {
            if (!hwnd)
            {
                return;
            }

            BOOL darkMode = TRUE;
            DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));
        }

        void enablePerMonitorDpiAwareness()
        {
            // ACE-UI3: keep the custom D2D shell in per-monitor DPI mode when
            // Windows supports it. This mirrors the Slate-style assumption that
            // geometry is expressed in logical UI units while native windows track
            // monitor DPI changes explicitly.
            const HMODULE user32 = GetModuleHandleW(L"user32.dll");
            if (!user32)
            {
                return;
            }

            using SetProcessDpiAwarenessContextFn = BOOL (WINAPI*)(DPI_AWARENESS_CONTEXT);
            const auto setContext = reinterpret_cast<SetProcessDpiAwarenessContextFn>(
                GetProcAddress(user32, "SetProcessDpiAwarenessContext")
            );
            if (setContext)
            {
                setContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
            }
        }
    }

    NativeWindow::~NativeWindow()
    {
        if (hwnd_)
        {
            DestroyWindow(hwnd_);
            hwnd_ = nullptr;
        }

        if (instance_ && !className_.empty())
        {
            UnregisterClassW(className_.c_str(), instance_);
        }
    }

    bool NativeWindow::create(const std::wstring& title, int width, int height, std::string* error)
    {
        enablePerMonitorDpiAwareness();

        instance_ = GetModuleHandleW(nullptr);
        className_ = kWindowClassName;
        width_ = width;
        height_ = height;

        HICON appIcon = static_cast<HICON>(LoadImageW(instance_, MAKEINTRESOURCEW(IDI_ARHQEN_APP), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE));
        HICON smallIcon = static_cast<HICON>(LoadImageW(instance_, MAKEINTRESOURCEW(IDI_ARHQEN_APP), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0));

        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.style = 0; // ACE-AQ3D7: no class-level full-window resize invalidation.
        wc.lpfnWndProc = &NativeWindow::windowProc;
        wc.hInstance = instance_;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hIcon = appIcon ? appIcon : LoadIconW(nullptr, IDI_APPLICATION);
        wc.hIconSm = smallIcon ? smallIcon : wc.hIcon;
        wc.hbrBackground = nullptr; // ACE-AQ3D7: custom D2D/DX12 UI owns background erase.
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

        RECT rect{0, 0, width_, height_};
        AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

        const HWND createdWindow = CreateWindowExW(
            0,
            className_.c_str(),
            title.c_str(),
            // ACE-AQ3D10: do not use WS_CLIPCHILDREN on the top-level D2D shell.
            // During live resize the DX12 child viewport is hidden/off-screen and
            // the parent must be able to paint a D2D proxy into the same area.
            WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            rect.right - rect.left,
            rect.bottom - rect.top,
            nullptr,
            nullptr,
            instance_,
            this
        );

        if (!createdWindow)
        {
            if (error)
            {
                std::ostringstream msg;
                msg << "CreateWindowExW failed. GetLastError=" << GetLastError();
                *error = msg.str();
            }
            return false;
        }

        hwnd_ = createdWindow;

        if (appIcon)
        {
            SendMessageW(hwnd_, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(appIcon));
        }

        if (smallIcon)
        {
            SendMessageW(hwnd_, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(smallIcon));
        }

        enableDarkTitleBar(hwnd_);

        ShowWindow(hwnd_, SW_SHOWDEFAULT);
        UpdateWindow(hwnd_);
        return true;
    }

    bool NativeWindow::pumpMessages()
    {
        MSG msg{};
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                closeRequested_ = true;
                return false;
            }

            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        return !closeRequested_;
    }

    void NativeWindow::setCommandHandler(std::function<bool(WPARAM, LPARAM)> handler)
    {
        commandHandler_ = std::move(handler);
    }

    void NativeWindow::setResizeHandler(std::function<void(int, int)> handler)
    {
        resizeHandler_ = std::move(handler);
    }

    void NativeWindow::setMessageHandler(std::function<LRESULT(UINT, WPARAM, LPARAM, bool*)> handler)
    {
        messageHandler_ = std::move(handler);
    }

    HWND NativeWindow::hwnd() const
    {
        return hwnd_;
    }

    int NativeWindow::width() const
    {
        return width_;
    }

    int NativeWindow::height() const
    {
        return height_;
    }

    bool NativeWindow::closeRequested() const
    {
        return closeRequested_;
    }

    LRESULT CALLBACK NativeWindow::windowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        NativeWindow* self = nullptr;

        if (message == WM_NCCREATE)
        {
            const auto* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
            self = static_cast<NativeWindow*>(createStruct->lpCreateParams);

            if (self)
            {
                self->hwnd_ = hwnd;
                SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            }
        }
        else
        {
            self = reinterpret_cast<NativeWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        }

        if (self)
        {
            return self->handleMessage(message, wParam, lParam);
        }

        return DefWindowProcW(hwnd, message, wParam, lParam);
    }

    LRESULT NativeWindow::handleMessage(UINT message, WPARAM wParam, LPARAM lParam)
    {
        // ACE-AQ3D8: update cached window size before the UI message handler so
        // WM_SIZE can be handled transactionally without also firing the legacy
        // resizeHandler/layout path. This prevents duplicate live-resize churn.
        if (message == WM_SIZE && wParam != SIZE_MINIMIZED)
        {
            width_ = LOWORD(lParam);
            height_ = HIWORD(lParam);
        }

        if (messageHandler_)
        {
            bool handled = false;
            const LRESULT result = messageHandler_(message, wParam, lParam, &handled);
            if (handled)
            {
                return result;
            }
        }

        switch (message)
        {
        case WM_CLOSE:
            closeRequested_ = true;
            DestroyWindow(hwnd_);
            hwnd_ = nullptr;
            return 0;

        case WM_DESTROY:
            closeRequested_ = true;
            PostQuitMessage(0);
            return 0;

        case WM_COMMAND:
            if (commandHandler_ && commandHandler_(wParam, lParam))
            {
                return 0;
            }
            return DefWindowProcW(hwnd_, message, wParam, lParam);

        case WM_ERASEBKGND:
            // ACE-AQ3D7: prevent classic Windows background erase flicker before D2D/DX12 paints.
            return 1;

        case WM_SIZE:
            if (wParam != SIZE_MINIMIZED)
            {
                if (resizeHandler_)
                {
                    resizeHandler_(width_, height_);
                }
            }
            return 0;

        default:
            return DefWindowProcW(hwnd_, message, wParam, lParam);
        }
    }
}
