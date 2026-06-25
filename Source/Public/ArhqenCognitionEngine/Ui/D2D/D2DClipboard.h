#pragma once

#include "ArhqenCognitionEngine/Ui/D2D/D2DUiTypes.h"

#include <optional>
#include <string>

namespace am::ui
{
    class D2DClipboard
    {
    public:
        static bool writeText(HWND owner, const std::wstring& text, std::string* error);
        static std::optional<std::wstring> readText(HWND owner, std::string* error);

    private:
        static std::string lastErrorToString(const char* label);
    };
}
