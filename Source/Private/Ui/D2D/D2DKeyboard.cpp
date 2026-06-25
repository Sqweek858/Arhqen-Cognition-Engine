#include "ArhqenCognitionEngine/Ui/D2D/D2DKeyboard.h"

#include <cwctype>

namespace am::ui
{
    D2DKeyboardState D2DKeyboardState::current()
    {
        D2DKeyboardState state;
        state.ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        state.shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        state.alt = (GetKeyState(VK_MENU) & 0x8000) != 0;
        return state;
    }

    bool D2DKeyboardState::ctrlOnly() const
    {
        return ctrl && !shift && !alt;
    }

    bool D2DKeyboardState::noModifiers() const
    {
        return !ctrl && !shift && !alt;
    }

    bool D2DKeyboard::isCtrlChord(WPARAM key, wchar_t expected)
    {
        const auto state = D2DKeyboardState::current();
        if (!state.ctrl || state.alt)
        {
            return false;
        }

        const wchar_t keyChar = static_cast<wchar_t>(std::towupper(static_cast<wint_t>(key)));
        const wchar_t expectedChar = static_cast<wchar_t>(std::towupper(static_cast<wint_t>(expected)));
        return keyChar == expectedChar;
    }

    bool D2DKeyboard::isNavigationKey(WPARAM key)
    {
        return key == VK_LEFT || key == VK_RIGHT || key == VK_UP || key == VK_DOWN || key == VK_HOME || key == VK_END;
    }

    bool D2DKeyboard::isEditingKey(WPARAM key)
    {
        return key == VK_BACK || key == VK_DELETE || key == VK_RETURN || key == VK_TAB;
    }
}
