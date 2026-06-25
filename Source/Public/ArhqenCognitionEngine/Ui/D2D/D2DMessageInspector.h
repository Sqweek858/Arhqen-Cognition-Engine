#pragma once

#include "ArhqenCognitionEngine/Core/AceUiModel.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DUiTypes.h"

namespace am::ui
{
    class D2DMessageInspector
    {
    public:
        static am::core::AceUiInspectorRecord makeRecord(const ChatMessage& message);
        static std::wstring kindName(ChatMessageKind kind);
    };
}
