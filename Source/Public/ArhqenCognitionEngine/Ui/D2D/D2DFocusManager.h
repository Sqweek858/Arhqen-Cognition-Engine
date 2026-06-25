#pragma once

#include "ArhqenCognitionEngine/Ui/D2D/D2DUiTypes.h"

namespace am::ui
{
    enum class D2DFocusTarget
    {
        None,
        MessageList,
        TextInput,
        SendButton,
        CommandPalette
    };

    class D2DFocusManager
    {
    public:
        void set(D2DFocusTarget target);
        D2DFocusTarget target() const;
        bool is(D2DFocusTarget target) const;
        void clear();

        D2DFocusTarget next() const;
        D2DFocusTarget previous() const;
        void focusNext();
        void focusPrevious();

        const wchar_t* name() const;

    private:
        D2DFocusTarget target_ = D2DFocusTarget::TextInput;
    };
}
