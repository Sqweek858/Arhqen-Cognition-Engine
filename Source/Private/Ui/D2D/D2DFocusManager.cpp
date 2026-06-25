#include "ArhqenCognitionEngine/Ui/D2D/D2DFocusManager.h"

namespace am::ui
{
    void D2DFocusManager::set(D2DFocusTarget target)
    {
        target_ = target;
    }

    D2DFocusTarget D2DFocusManager::target() const
    {
        return target_;
    }

    bool D2DFocusManager::is(D2DFocusTarget target) const
    {
        return target_ == target;
    }

    void D2DFocusManager::clear()
    {
        target_ = D2DFocusTarget::None;
    }

    D2DFocusTarget D2DFocusManager::next() const
    {
        switch (target_)
        {
        case D2DFocusTarget::MessageList:
            return D2DFocusTarget::TextInput;
        case D2DFocusTarget::TextInput:
            return D2DFocusTarget::SendButton;
        case D2DFocusTarget::SendButton:
            return D2DFocusTarget::MessageList;
        case D2DFocusTarget::CommandPalette:
            return D2DFocusTarget::TextInput;
        case D2DFocusTarget::None:
        default:
            return D2DFocusTarget::TextInput;
        }
    }

    D2DFocusTarget D2DFocusManager::previous() const
    {
        switch (target_)
        {
        case D2DFocusTarget::MessageList:
            return D2DFocusTarget::SendButton;
        case D2DFocusTarget::TextInput:
            return D2DFocusTarget::MessageList;
        case D2DFocusTarget::SendButton:
            return D2DFocusTarget::TextInput;
        case D2DFocusTarget::CommandPalette:
            return D2DFocusTarget::TextInput;
        case D2DFocusTarget::None:
        default:
            return D2DFocusTarget::TextInput;
        }
    }

    void D2DFocusManager::focusNext()
    {
        target_ = next();
    }

    void D2DFocusManager::focusPrevious()
    {
        target_ = previous();
    }

    const wchar_t* D2DFocusManager::name() const
    {
        switch (target_)
        {
        case D2DFocusTarget::None:
            return L"none";
        case D2DFocusTarget::MessageList:
            return L"message_list";
        case D2DFocusTarget::TextInput:
            return L"text_input";
        case D2DFocusTarget::SendButton:
            return L"send_button";
        case D2DFocusTarget::CommandPalette:
            return L"command_palette";
        default:
            return L"unknown";
        }
    }
}
