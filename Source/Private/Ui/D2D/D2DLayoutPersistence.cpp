#include "ArhqenCognitionEngine/Ui/D2D/D2DLayoutPersistence.h"

#include <fstream>
#include <sstream>
#include <unordered_map>

namespace am::ui
{
    bool D2DLayoutPersistence::save(const std::filesystem::path& path, const D2DDockLayoutProfile& profile, std::string* error)
    {
        try
        {
            std::filesystem::create_directories(path.parent_path());

            std::ofstream out(path, std::ios::binary);
            if (!out)
            {
                if (error) { *error = "Could not open layout profile for writing."; }
                return false;
            }

            out << "ARCHITECTMIND_LAYOUT_PROFILE_V1\n";
            out << "sidebarWidth=" << profile.sidebarWidth << "\n";
            out << "workspaceRatio=" << profile.workspaceRatio << "\n";
            out << "inspectorRatio=" << profile.inspectorRatio << "\n";
            out << "sidebarCollapsed=" << boolText(profile.sidebarCollapsed) << "\n";
            out << "workspaceCollapsed=" << boolText(profile.workspaceCollapsed) << "\n";
            out << "inspectorCollapsed=" << boolText(profile.inspectorCollapsed) << "\n";
            out << "diagnosticsPinned=" << boolText(profile.diagnosticsPinned) << "\n";
            out << "activeWorkspaceTab=" << narrow(profile.activeWorkspaceTab) << "\n";
            out << "aquariumDetailsWidth=" << profile.aquariumDetailsWidth << "\n";
            out << "aquariumDetailsHeight=" << profile.aquariumDetailsHeight << "\n";
            out << "aquariumLogsWidth=" << profile.aquariumLogsWidth << "\n";
            out << "aquariumLogsHeight=" << profile.aquariumLogsHeight << "\n";
            out << "aquariumDetailsVisible=" << boolText(profile.aquariumDetailsVisible) << "\n";
            out << "aquariumLogsVisible=" << boolText(profile.aquariumLogsVisible) << "\n";

            return true;
        }
        catch (const std::exception& ex)
        {
            if (error) { *error = ex.what(); }
            return false;
        }
    }

    std::optional<D2DDockLayoutProfile> D2DLayoutPersistence::load(const std::filesystem::path& path, std::string* error)
    {
        try
        {
            std::ifstream in(path, std::ios::binary);
            if (!in)
            {
                if (error) { *error = "Layout profile does not exist."; }
                return std::nullopt;
            }

            std::string header;
            std::getline(in, header);

            if (header != "ARCHITECTMIND_LAYOUT_PROFILE_V1")
            {
                if (error) { *error = "Layout profile has invalid header."; }
                return std::nullopt;
            }

            std::unordered_map<std::string, std::string> values;
            std::string line;

            while (std::getline(in, line))
            {
                const auto equals = line.find('=');
                if (equals == std::string::npos)
                {
                    continue;
                }

                values[line.substr(0, equals)] = line.substr(equals + 1);
            }

            D2DDockLayoutProfile profile = D2DDockLayoutProfile::defaults();

            if (values.count("sidebarWidth")) { profile.sidebarWidth = std::stof(values["sidebarWidth"]); }
            if (values.count("workspaceRatio")) { profile.workspaceRatio = std::stof(values["workspaceRatio"]); }
            if (values.count("inspectorRatio")) { profile.inspectorRatio = std::stof(values["inspectorRatio"]); }
            if (values.count("sidebarCollapsed")) { profile.sidebarCollapsed = parseBool(values["sidebarCollapsed"]); }
            if (values.count("workspaceCollapsed")) { profile.workspaceCollapsed = parseBool(values["workspaceCollapsed"]); }
            if (values.count("inspectorCollapsed")) { profile.inspectorCollapsed = parseBool(values["inspectorCollapsed"]); }
            if (values.count("diagnosticsPinned")) { profile.diagnosticsPinned = parseBool(values["diagnosticsPinned"]); }
            if (values.count("activeWorkspaceTab")) { profile.activeWorkspaceTab = widen(values["activeWorkspaceTab"]); }
            if (values.count("aquariumDetailsWidth")) { profile.aquariumDetailsWidth = std::stof(values["aquariumDetailsWidth"]); }
            if (values.count("aquariumDetailsHeight")) { profile.aquariumDetailsHeight = std::stof(values["aquariumDetailsHeight"]); }
            if (values.count("aquariumLogsWidth")) { profile.aquariumLogsWidth = std::stof(values["aquariumLogsWidth"]); }
            if (values.count("aquariumLogsHeight")) { profile.aquariumLogsHeight = std::stof(values["aquariumLogsHeight"]); }
            if (values.count("aquariumDetailsVisible")) { profile.aquariumDetailsVisible = parseBool(values["aquariumDetailsVisible"]); }
            if (values.count("aquariumLogsVisible")) { profile.aquariumLogsVisible = parseBool(values["aquariumLogsVisible"]); }

            profile.clamp();
            return profile;
        }
        catch (const std::exception& ex)
        {
            if (error) { *error = ex.what(); }
            return std::nullopt;
        }
    }

    std::string D2DLayoutPersistence::narrow(const std::wstring& text)
    {
        std::string result;
        result.reserve(text.size());

        for (wchar_t ch : text)
        {
            result.push_back(ch <= 127 ? static_cast<char>(ch) : '?');
        }

        return result;
    }

    std::wstring D2DLayoutPersistence::widen(const std::string& text)
    {
        return std::wstring(text.begin(), text.end());
    }

    bool D2DLayoutPersistence::parseBool(const std::string& value)
    {
        return value == "1" || value == "true" || value == "yes";
    }

    std::string D2DLayoutPersistence::boolText(bool value)
    {
        return value ? "1" : "0";
    }
}
