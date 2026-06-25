#pragma once

#include "ArhqenCognitionEngine/Ui/D2D/D2DUiTypes.h"

namespace am::ui
{
    struct D2DKeyboardState
    {
        bool ctrl = false;
        bool shift = false;
        bool alt = false;

        static D2DKeyboardState current();
        bool ctrlOnly() const;
        bool noModifiers() const;
    };

    class D2DKeyboard
    {
    public:
        static bool isCtrlChord(WPARAM key, wchar_t expected);
        static bool isNavigationKey(WPARAM key);
        static bool isEditingKey(WPARAM key);
    };
}
