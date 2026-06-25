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
    const auto nativeCpp = readText(root / "Source/Private/Renderer/NativeWindow.cpp");
    const auto metricsH = readText(root / "Source/Public/ArhqenCognitionEngine/Ui/D2D/D2DDisplayMetrics.h");
    const auto metricsCpp = readText(root / "Source/Private/Ui/D2D/D2DDisplayMetrics.cpp");
    const auto cacheH = readText(root / "Source/Public/ArhqenCognitionEngine/Ui/D2D/D2DCachedEffects.h");
    const auto cacheCpp = readText(root / "Source/Private/Ui/D2D/D2DCachedEffects.cpp");
    const auto blurCpp = readText(root / "Source/Private/Ui/D2D/D2DBlurRuntime.cpp");
    const auto glassCpp = readText(root / "Source/Private/Ui/D2D/D2DGlassEffects.cpp");
    const auto renderContext = readText(root / "Source/Public/ArhqenCognitionEngine/Ui/D2D/D2DRenderContext.h");

    bool ok = true;
    auto check = [&](const char* name, bool value)
    {
        std::cout << (value ? "PASS|" : "FAIL|") << name << "\n";
        ok = ok && value;
    };

    check("ui3_display_metrics_class_exists", has(metricsH, "class D2DDisplayMetrics"));
    check("ui3_monitor_work_area_clamp_helper_exists", has(metricsH, "clampToWorkArea"));
    check("ui3_client_screen_transform_helpers_exist", has(metricsH, "clientRectToScreenRect") && has(metricsH, "screenRectToClientRect"));
    check("ui3_wm_dpichanged_handled", has(shellCpp, "WM_DPICHANGED"));
    check("ui3_wm_displaychange_handled", has(shellCpp, "WM_DISPLAYCHANGE"));
    check("ui3_per_monitor_dpi_awareness_requested", has(nativeCpp, "SetProcessDpiAwarenessContext") && has(nativeCpp, "PER_MONITOR_AWARE_V2"));
    check("ui3_context_exposes_dpi_scale", has(renderContext, "dpiScale") && has(shellCpp, "ctx.dpiScale"));
    check("ui3_display_metrics_refreshed_on_resize", has(shellCpp, "refreshDisplayMetrics(\"resize\")"));
    check("ui3_project_references_metrics", has(readText(root / "Source/ArhqenCognitionEngine.vcxproj"), "D2DDisplayMetrics.cpp"));

    check("ui4_cached_effects_class_exists", has(cacheH, "class D2DCachedEffects"));
    check("ui4_cached_blur_fallback_used", has(glassCpp, "D2DCachedEffects::drawCachedBlurFallback"));
    check("ui4_blur_runtime_reports_cached_mode", has(blurCpp, "CachedFrostedFallback") && has(blurCpp, "cached_frosted_fallback"));
    check("ui4_effect_cache_stats_exist", has(cacheH, "D2DEffectCacheStats") && has(cacheCpp, "hitCount"));
    check("ui4_cache_stats_command_reports_effect_cache", has(shellCpp, "effect cache: entries="));
    check("ui4_effect_cache_resets_with_device_resources", has(shellCpp, "D2DCachedEffects::reset()"));
    check("ui4_project_references_cached_effects", has(readText(root / "Source/ArhqenCognitionEngine.vcxproj"), "D2DCachedEffects.cpp"));

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
        std::cout << "PASS|ace_ui3_ui4_probe\n";
        return 0;
    }
    std::cout << "FAIL|ace_ui3_ui4_probe\n";
    return 1;
}
