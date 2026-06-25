#pragma once

#if !defined(_WIN32)
#error ArhqenCognitionEngine M3 NativeWindow is Windows-only.
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>

#include <functional>
#include <string>

namespace am::renderer
{
    class NativeWindow
    {
    public:
        NativeWindow() = default;
        ~NativeWindow();

        NativeWindow(const NativeWindow&) = delete;
        NativeWindow& operator=(const NativeWindow&) = delete;

        bool create(const std::wstring& title, int width, int height, std::string* error);
        bool pumpMessages();

        void setCommandHandler(std::function<bool(WPARAM, LPARAM)> handler);
        void setResizeHandler(std::function<void(int, int)> handler);
        void setMessageHandler(std::function<LRESULT(UINT, WPARAM, LPARAM, bool*)> handler);

        HWND hwnd() const;
        int width() const;
        int height() const;
        bool closeRequested() const;

    private:
        static LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
        LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam);

        HINSTANCE instance_ = nullptr;
        HWND hwnd_ = nullptr;
        int width_ = 0;
        int height_ = 0;
        bool closeRequested_ = false;
        std::wstring className_;
        std::function<bool(WPARAM, LPARAM)> commandHandler_;
        std::function<void(int, int)> resizeHandler_;
        std::function<LRESULT(UINT, WPARAM, LPARAM, bool*)> messageHandler_;
    };
}
