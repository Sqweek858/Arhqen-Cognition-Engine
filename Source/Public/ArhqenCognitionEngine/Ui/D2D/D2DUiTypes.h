#pragma once

#if !defined(_WIN32)
#error ArhqenCognitionEngine D2D UI is Windows-only.
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>
#include <d2d1.h>
#include <dwrite.h>

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace am::ui
{
    struct UiPoint
    {
        float x = 0.0f;
        float y = 0.0f;
    };

    struct UiSize
    {
        float width = 0.0f;
        float height = 0.0f;
    };

    struct UiPadding
    {
        float left = 0.0f;
        float top = 0.0f;
        float right = 0.0f;
        float bottom = 0.0f;
    };

    struct UiLayoutMetrics
    {
        float margin = 26.0f;
        float gap = 18.0f;
        float headerHeight = 92.0f;
        float sidebarWidth = 314.0f;
        float inputHeight = 96.0f;
        float statusHeight = 30.0f;
        float panelRadius = 24.0f;
        float bubbleRadius = 16.0f;
    };

    struct UiRect
    {
        float left = 0.0f;
        float top = 0.0f;
        float right = 0.0f;
        float bottom = 0.0f;

        float width() const { return right - left; }
        float height() const { return bottom - top; }
        bool empty() const { return width() <= 0.0f || height() <= 0.0f; }

        D2D1_RECT_F d2d() const
        {
            return D2D1::RectF(left, top, right, bottom);
        }

        bool contains(float x, float y) const
        {
            return x >= left && x <= right && y >= top && y <= bottom;
        }

        UiRect inset(float amount) const
        {
            return {left + amount, top + amount, right - amount, bottom - amount};
        }

        UiRect inset(UiPadding padding) const
        {
            return {left + padding.left, top + padding.top, right - padding.right, bottom - padding.bottom};
        }
    };

    enum class ChatMessageKind
    {
        Assistant,
        User,
        System,
        Tool,
        Warning
    };

    struct ChatMessage
    {
        std::wstring author;
        std::wstring text;
        bool fromUser = false;
        bool system = false;
        ChatMessageKind kind = ChatMessageKind::Assistant;
        std::uint64_t id = 0;
        std::wstring timestamp;
        std::wstring metadata;
    };

    struct ChatSubmitResult
    {
        std::vector<ChatMessage> messages;
        std::wstring status;
        bool ok = true;
    };

    inline UiRect makeUiRect(float left, float top, float right, float bottom)
    {
        return {left, top, right, bottom};
    }

    inline float clampFloat(float value, float minValue, float maxValue)
    {
        return std::max(minValue, std::min(value, maxValue));
    }

    inline std::size_t clampIndex(std::size_t value, std::size_t minValue, std::size_t maxValue)
    {
        return std::max(minValue, std::min(value, maxValue));
    }
}
