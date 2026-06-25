#pragma once

#include "ArhqenCognitionEngine/Ui/D2D/D2DUiTypes.h"

#include <cstdint>

namespace am::ui
{
    enum class UiEventKind
    {
        None,
        MouseMove,
        MouseDown,
        MouseUp,
        MouseWheel,
        KeyDown,
        Character,
        FocusLost
    };

    enum class UiMouseButton
    {
        None,
        Left,
        Right,
        Middle
    };

    struct UiKeyboardModifiers
    {
        bool ctrl = false;
        bool shift = false;
        bool alt = false;
    };

    struct UiEvent
    {
        UiEventKind kind = UiEventKind::None;
        UiRect targetRect {};
        float x = 0.0f;
        float y = 0.0f;
        int wheelDelta = 0;
        std::uint32_t key = 0;
        wchar_t character = 0;
        UiMouseButton button = UiMouseButton::None;
        UiKeyboardModifiers modifiers {};
    };

    struct UiHitTestResult
    {
        bool hit = false;
        bool wantsCapture = false;
        int zOrder = 0;
    };
}
