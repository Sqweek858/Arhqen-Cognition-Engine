#include "ArhqenCognitionEngine/Ui/D2D/D2DClipboard.h"

#include <sstream>
#include <cstring>

namespace am::ui
{
    bool D2DClipboard::writeText(HWND owner, const std::wstring& text, std::string* error)
    {
        if (!OpenClipboard(owner))
        {
            if (error) { *error = lastErrorToString("OpenClipboard"); }
            return false;
        }

        const auto closeClipboard = []() { CloseClipboard(); };

        if (!EmptyClipboard())
        {
            if (error) { *error = lastErrorToString("EmptyClipboard"); }
            closeClipboard();
            return false;
        }

        const SIZE_T bytes = (text.size() + 1) * sizeof(wchar_t);
        HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, bytes);

        if (!memory)
        {
            if (error) { *error = lastErrorToString("GlobalAlloc"); }
            closeClipboard();
            return false;
        }

        void* locked = GlobalLock(memory);
        if (!locked)
        {
            if (error) { *error = lastErrorToString("GlobalLock"); }
            GlobalFree(memory);
            closeClipboard();
            return false;
        }

        std::memcpy(locked, text.c_str(), bytes);
        GlobalUnlock(memory);

        if (!SetClipboardData(CF_UNICODETEXT, memory))
        {
            if (error) { *error = lastErrorToString("SetClipboardData"); }
            GlobalFree(memory);
            closeClipboard();
            return false;
        }

        // Clipboard owns memory after successful SetClipboardData.
        closeClipboard();
        return true;
    }

    std::optional<std::wstring> D2DClipboard::readText(HWND owner, std::string* error)
    {
        if (!OpenClipboard(owner))
        {
            if (error) { *error = lastErrorToString("OpenClipboard"); }
            return std::nullopt;
        }

        HANDLE data = GetClipboardData(CF_UNICODETEXT);
        if (!data)
        {
            if (error) { *error = "Clipboard does not contain CF_UNICODETEXT."; }
            CloseClipboard();
            return std::nullopt;
        }

        const wchar_t* text = static_cast<const wchar_t*>(GlobalLock(data));
        if (!text)
        {
            if (error) { *error = lastErrorToString("GlobalLock"); }
            CloseClipboard();
            return std::nullopt;
        }

        std::wstring result(text);
        GlobalUnlock(data);
        CloseClipboard();
        return result;
    }

    std::string D2DClipboard::lastErrorToString(const char* label)
    {
        std::ostringstream out;
        out << label << " failed. GetLastError=" << GetLastError();
        return out.str();
    }
}
