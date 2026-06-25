#pragma once

#include "ArhqenCognitionEngine/Ui/D2D/D2DUiTypes.h"

#include <string>

namespace am::ui
{
    struct D2DMonitorMetrics
    {
        RECT monitorRect{};
        RECT workRect{};
        UINT dpiX = 96;
        UINT dpiY = 96;
        float dpiScaleX = 1.0f;
        float dpiScaleY = 1.0f;
        bool primary = false;
        std::wstring deviceName;
    };

    struct D2DDisplayMetricsSnapshot
    {
        HWND hwnd = nullptr;
        D2DMonitorMetrics nearestMonitor{};
        RECT clientRect{};
        RECT windowRect{};
        float uiScale = 1.0f;
        std::uint64_t captureSerial = 0;
        bool valid = false;
    };

    class D2DDisplayMetrics
    {
    public:
        static D2DDisplayMetricsSnapshot capture(HWND hwnd);
        static D2DMonitorMetrics monitorFromWindow(HWND hwnd);
        static D2DMonitorMetrics monitorFromPoint(POINT point);
        static UiRect clampToWorkArea(UiRect rect, const D2DMonitorMetrics& monitor, float margin = 8.0f);
        static UiRect clientRectToScreenRect(HWND hwnd, UiRect rect);
        static UiRect screenRectToClientRect(HWND hwnd, UiRect rect);
        static float dpiScaleForWindow(HWND hwnd);
        static std::wstring describe(const D2DDisplayMetricsSnapshot& snapshot);
    };
}
