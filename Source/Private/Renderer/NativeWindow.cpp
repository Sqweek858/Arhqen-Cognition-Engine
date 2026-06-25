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

        void enableDarkTitleBar(HWND hwnd)
        {
            if (!hwnd)
            {
                return;
            }

            BOOL darkMode = TRUE;
            DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));
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
        instance_ = GetModuleHandleW(nullptr);
        className_ = kWindowClassName;
        width_ = width;
        height_ = height;

        HICON appIcon = static_cast<HICON>(LoadImageW(instance_, MAKEINTRESOURCEW(IDI_ARHQEN_APP), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE));
        HICON smallIcon = static_cast<HICON>(LoadImageW(instance_, MAKEINTRESOURCEW(IDI_ARHQEN_APP), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0));

        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = &NativeWindow::windowProc;
        wc.hInstance = instance_;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hIcon = appIcon ? appIcon : LoadIconW(nullptr, IDI_APPLICATION);
        wc.hIconSm = smallIcon ? smallIcon : wc.hIcon;
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
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
            WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
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

        case WM_SIZE:
            if (wParam != SIZE_MINIMIZED)
            {
                width_ = LOWORD(lParam);
                height_ = HIWORD(lParam);

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
