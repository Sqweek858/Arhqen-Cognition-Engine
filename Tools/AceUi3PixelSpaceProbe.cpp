#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

static std::string readText(const fs::path& p)
{
    std::ifstream in(p, std::ios::binary);
    if (!in) return {};
    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

static bool has(const std::string& text, const std::string& needle)
{
    return text.find(needle) != std::string::npos;
}

int main()
{
    const auto root = fs::current_path();
    const auto shellCpp = readText(root / "Source/Private/Ui/AceShellUi.cpp");
    const auto shellH = readText(root / "Source/Public/ArhqenCognitionEngine/Ui/AceShellUi.h");
    const auto docs = readText(root / "Docs/ACE_UI3F.md");

    bool ok = true;
    auto check = [&](const char* name, bool value)
    {
        std::cout << (value ? "PASS|" : "FAIL|") << name << "\n";
        ok = ok && value;
    };

    check("ui3f_pixel_space_d2d_helper_exists", has(shellH, "applyPixelAlignedD2DTargetDpi") && has(shellCpp, "AceShellUi::applyPixelAlignedD2DTargetDpi"));
    check("ui3f_hwnd_render_target_uses_96_dpi", has(shellCpp, "96.0f,\n                96.0f"));
    check("ui3f_setdpi_96_after_create_or_resize", has(shellCpp, "renderTarget_->SetDpi(96.0f, 96.0f)") && has(shellCpp, "applyPixelAlignedD2DTargetDpi();"));
    check("ui3f_layout_remains_pixel_space", has(shellCpp, "existing Arhqen UI layout remains pixel-space"));
    check("ui3f_dpi_metrics_still_available", has(shellCpp, "ctx.dpiScale = dpiScale()") && has(shellCpp, "displayMetrics_"));
    check("ui3f_cache_stats_reports_dpi_fix", has(shellCpp, "d2d pixel-dpi fix="));
    check("ui3f_docs_exist", has(docs, "ACE-UI3F"));

    bool rootArtifacts = false;
    for (const auto& entry : fs::directory_iterator(root))
    {
        if (!entry.is_regular_file()) continue;
        const auto ext = entry.path().extension().string();
        if (ext == ".obj" || ext == ".exe" || ext == ".pdb" || ext == ".ilk" || ext == ".log")
        {
            rootArtifacts = true;
        }
    }
    check("no_build_artifacts_in_repo_root", !rootArtifacts);

    if (ok)
    {
        std::cout << "PASS|ace_ui3f_pixel_space_dpi_probe\n";
        return 0;
    }
    std::cout << "FAIL|ace_ui3f_pixel_space_dpi_probe\n";
    return 1;
}
