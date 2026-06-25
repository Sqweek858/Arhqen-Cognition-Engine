#include "ArhqenCognitionEngine/Ui/D2D/D2DDisplayMetrics.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace am::ui
{
    namespace
    {
        std::uint64_t gCaptureSerial = 0;

        UINT safeDpiForWindow(HWND hwnd)
        {
            if (hwnd)
            {
                using GetDpiForWindowFn = UINT (WINAPI*)(HWND);
                const HMODULE user32 = GetModuleHandleW(L"user32.dll");
                const auto getDpiForWindow = user32
                    ? reinterpret_cast<GetDpiForWindowFn>(GetProcAddress(user32, "GetDpiForWindow"))
                    : nullptr;
                if (getDpiForWindow)
                {
                    const UINT dpi = getDpiForWindow(hwnd);
                    if (dpi > 0)
                    {
                        return dpi;
                    }
                }
            }
            const HDC screen = GetDC(nullptr);
            if (!screen)
            {
                return 96;
            }
            const int dpi = GetDeviceCaps(screen, LOGPIXELSX);
            ReleaseDC(nullptr, screen);
            return dpi > 0 ? static_cast<UINT>(dpi) : 96U;
        }

        D2DMonitorMetrics metricsForMonitor(HMONITOR monitor, UINT dpi)
        {
            D2DMonitorMetrics metrics;
            metrics.dpiX = dpi > 0 ? dpi : 96U;
            metrics.dpiY = metrics.dpiX;
            metrics.dpiScaleX = static_cast<float>(metrics.dpiX) / 96.0f;
            metrics.dpiScaleY = static_cast<float>(metrics.dpiY) / 96.0f;

            MONITORINFOEXW info{};
            info.cbSize = sizeof(info);
            if (monitor && GetMonitorInfoW(monitor, &info))
            {
                metrics.monitorRect = info.rcMonitor;
                metrics.workRect = info.rcWork;
                metrics.primary = (info.dwFlags & MONITORINFOF_PRIMARY) != 0;
                metrics.deviceName = info.szDevice;
            }
            else
            {
                metrics.monitorRect = RECT{0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)};
                metrics.workRect = metrics.monitorRect;
                metrics.primary = true;
            }

            return metrics;
        }

        UiRect rectFromRECT(const RECT& r)
        {
            return makeUiRect(static_cast<float>(r.left), static_cast<float>(r.top), static_cast<float>(r.right), static_cast<float>(r.bottom));
        }

        RECT rectToRECT(UiRect r)
        {
            return RECT{
                static_cast<LONG>(std::floor(r.left)),
                static_cast<LONG>(std::floor(r.top)),
                static_cast<LONG>(std::ceil(r.right)),
                static_cast<LONG>(std::ceil(r.bottom))
            };
        }
    }

    D2DDisplayMetricsSnapshot D2DDisplayMetrics::capture(HWND hwnd)
    {
        D2DDisplayMetricsSnapshot snapshot;
        snapshot.hwnd = hwnd;
        snapshot.captureSerial = ++gCaptureSerial;
        snapshot.nearestMonitor = monitorFromWindow(hwnd);
        snapshot.uiScale = std::clamp(snapshot.nearestMonitor.dpiScaleX, 0.75f, 2.50f);
        snapshot.valid = hwnd != nullptr;
        if (hwnd)
        {
            GetClientRect(hwnd, &snapshot.clientRect);
            GetWindowRect(hwnd, &snapshot.windowRect);
        }
        return snapshot;
    }

    D2DMonitorMetrics D2DDisplayMetrics::monitorFromWindow(HWND hwnd)
    {
        const UINT dpi = safeDpiForWindow(hwnd);
        HMONITOR monitor = nullptr;
        if (hwnd)
        {
            monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
        }
        if (!monitor)
        {
            POINT pt{0, 0};
            monitor = MonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY);
        }
        return metricsForMonitor(monitor, dpi);
    }

    D2DMonitorMetrics D2DDisplayMetrics::monitorFromPoint(POINT point)
    {
        const HMONITOR monitor = MonitorFromPoint(point, MONITOR_DEFAULTTONEAREST);
        return metricsForMonitor(monitor, 96U);
    }

    UiRect D2DDisplayMetrics::clampToWorkArea(UiRect rect, const D2DMonitorMetrics& monitor, float margin)
    {
        const UiRect work = rectFromRECT(monitor.workRect).inset(margin);
        const float width = std::min(rect.width(), std::max(0.0f, work.width()));
        const float height = std::min(rect.height(), std::max(0.0f, work.height()));
        float left = rect.left;
        float top = rect.top;
        if (left + width > work.right)
        {
            left = work.right - width;
        }
        if (top + height > work.bottom)
        {
            top = work.bottom - height;
        }
        left = std::max(work.left, left);
        top = std::max(work.top, top);
        return makeUiRect(left, top, left + width, top + height);
    }

    UiRect D2DDisplayMetrics::clientRectToScreenRect(HWND hwnd, UiRect rect)
    {
        POINT topLeft{static_cast<LONG>(std::floor(rect.left)), static_cast<LONG>(std::floor(rect.top))};
        POINT bottomRight{static_cast<LONG>(std::ceil(rect.right)), static_cast<LONG>(std::ceil(rect.bottom))};
        if (hwnd)
        {
            ClientToScreen(hwnd, &topLeft);
            ClientToScreen(hwnd, &bottomRight);
        }
        return makeUiRect(static_cast<float>(topLeft.x), static_cast<float>(topLeft.y), static_cast<float>(bottomRight.x), static_cast<float>(bottomRight.y));
    }

    UiRect D2DDisplayMetrics::screenRectToClientRect(HWND hwnd, UiRect rect)
    {
        POINT topLeft{static_cast<LONG>(std::floor(rect.left)), static_cast<LONG>(std::floor(rect.top))};
        POINT bottomRight{static_cast<LONG>(std::ceil(rect.right)), static_cast<LONG>(std::ceil(rect.bottom))};
        if (hwnd)
        {
            ScreenToClient(hwnd, &topLeft);
            ScreenToClient(hwnd, &bottomRight);
        }
        return makeUiRect(static_cast<float>(topLeft.x), static_cast<float>(topLeft.y), static_cast<float>(bottomRight.x), static_cast<float>(bottomRight.y));
    }

    float D2DDisplayMetrics::dpiScaleForWindow(HWND hwnd)
    {
        return std::clamp(static_cast<float>(safeDpiForWindow(hwnd)) / 96.0f, 0.75f, 2.50f);
    }

    std::wstring D2DDisplayMetrics::describe(const D2DDisplayMetricsSnapshot& snapshot)
    {
        std::wostringstream ss;
        ss << L"dpi=" << snapshot.nearestMonitor.dpiX
           << L" scale=" << snapshot.uiScale
           << L" primary=" << (snapshot.nearestMonitor.primary ? L"yes" : L"no")
           << L" monitor=" << snapshot.nearestMonitor.deviceName
           << L" work=" << snapshot.nearestMonitor.workRect.left << L"," << snapshot.nearestMonitor.workRect.top
           << L"-" << snapshot.nearestMonitor.workRect.right << L"," << snapshot.nearestMonitor.workRect.bottom;
        return ss.str();
    }
}
