#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

static std::string Read(const fs::path& p)
{
    std::ifstream in(p, std::ios::binary);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

static bool Contains(const fs::path& p, const std::string& needle)
{
    return Read(p).find(needle) != std::string::npos;
}

int main()
{
    struct Check { const char* name; fs::path path; const char* marker; };
    const std::vector<Check> checks = {
        {"ui5_text_layout_foundation_exists", "Source/Public/ArhqenCognitionEngine/Ui/D2D/D2DTextLayoutFoundation.h", "D2DTextLayoutFoundation"},
        {"ui5_ellipsis_exists", "Source/Private/Ui/D2D/D2DTextLayoutFoundation.cpp", "EllipsizeToFit"},
        {"ui5_clip_stack_exists", "Source/Private/Ui/D2D/D2DTextLayoutFoundation.cpp", "PushAxisAlignedClip"},
        {"ui6_retained_layout_tree_exists", "Source/Public/ArhqenCognitionEngine/Ui/Core/AceUiRetainedLayout.h", "AceUiRetainedLayoutTree"},
        {"ui6_measure_arrange_paint_shape", "Source/Public/ArhqenCognitionEngine/Ui/Core/AceUiRetainedLayout.h", "allocated"},
        {"ui7_draw_command_buffer_exists", "Source/Public/ArhqenCognitionEngine/Ui/D2D/D2DDrawCommandBuffer.h", "D2DDrawCommandBuffer"},
        {"ui7_draw_command_layers_exist", "Source/Public/ArhqenCognitionEngine/Ui/D2D/D2DDrawCommandBuffer.h", "layer"},
        {"ui8_invalidation_root_exists", "Source/Public/ArhqenCognitionEngine/Ui/Core/AceUiInvalidationRoot.h", "AceUiInvalidationRoot"},
        {"ui8_dirty_flags_exist", "Source/Public/ArhqenCognitionEngine/Ui/Core/AceUiInvalidationRoot.h", "Viewport"},
        {"ui9_debug_overlay_exists", "Source/Public/ArhqenCognitionEngine/Ui/D2D/D2DUiDebugOverlay.h", "D2DUiDebugOverlay"},
        {"ui9_f9_toggle_wired", "Source/Private/Ui/AceShellUi.cpp", "VK_F9"},
        {"aqui1_telemetry_widgets_exist", "Source/Public/ArhqenCognitionEngine/Ui/D2D/D2DAquariumTelemetryWidgets.h", "D2DAquariumTelemetryWidgets"},
        {"aqui1_telemetry_render_wired", "Source/Private/Ui/AceShellUi.cpp", "RenderOverlay(ctx, viewportSurface, snapshot)"},
        {"ui11_style_set_exists", "Source/Public/ArhqenCognitionEngine/Ui/Core/AceUiStyleSet.h", "AceUiStyleSet"},
        {"ui11_default_arhqen_style_exists", "Source/Private/Ui/Core/AceUiStyleSet.cpp", "MakeDefaultArhqen"},
        {"ui_command_stats_wired", "Source/Private/Ui/AceShellUi.cpp", "ui_stats"},
        {"project_references_new_sources", "Source/ArhqenCognitionEngine.vcxproj", "D2DTextLayoutFoundation.cpp"},
        {"cmake_references_new_sources", "CMakeLists.txt", "AceUiRetainedLayout.cpp"},
        {"docs_ui5_exists", "Docs/ACE_UI5.md", "ACE-UI5"},
        {"docs_ui6_exists", "Docs/ACE_UI6.md", "ACE-UI6"},
        {"docs_ui7_exists", "Docs/ACE_UI7.md", "ACE-UI7"},
        {"docs_ui8_exists", "Docs/ACE_UI8.md", "ACE-UI8"},
        {"docs_ui9_exists", "Docs/ACE_UI9.md", "ACE-UI9"},
        {"docs_aqui1_exists", "Docs/ACE_AQUI1.md", "ACE-AQUI1"},
        {"docs_ui11_exists", "Docs/ACE_UI11.md", "ACE-UI11"}
    };

    bool ok = true;
    for (const auto& check : checks)
    {
        const bool pass = fs::exists(check.path) && Contains(check.path, check.marker);
        std::cout << (pass ? "PASS|" : "FAIL|") << check.name << "\n";
        ok = ok && pass;
    }

    const std::vector<std::string> forbidden = {".obj", ".exe", ".pdb", ".ilk", ".log"};
    for (const auto& entry : fs::directory_iterator(fs::current_path()))
    {
        if (!entry.is_regular_file()) continue;
        const auto ext = entry.path().extension().string();
        for (const auto& bad : forbidden)
        {
            if (ext == bad)
            {
                std::cout << "FAIL|no_build_artifacts_in_repo_root\n";
                return 1;
            }
        }
    }
    std::cout << "PASS|no_build_artifacts_in_repo_root\n";
    std::cout << (ok ? "PASS|ace_ui5_ui11_aqui1_probe\n" : "FAIL|ace_ui5_ui11_aqui1_probe\n");
    return ok ? 0 : 1;
}
