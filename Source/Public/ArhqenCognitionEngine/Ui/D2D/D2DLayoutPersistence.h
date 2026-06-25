#pragma once

#include "ArhqenCognitionEngine/Ui/D2D/D2DLayoutProfile.h"

#include <filesystem>
#include <optional>

namespace am::ui
{
    class D2DLayoutPersistence
    {
    public:
        static bool save(const std::filesystem::path& path, const D2DDockLayoutProfile& profile, std::string* error);
        static std::optional<D2DDockLayoutProfile> load(const std::filesystem::path& path, std::string* error);

    private:
        static std::string narrow(const std::wstring& text);
        static std::wstring widen(const std::string& text);
        static bool parseBool(const std::string& value);
        static std::string boolText(bool value);
    };
}
