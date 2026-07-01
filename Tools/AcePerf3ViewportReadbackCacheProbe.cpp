#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace
{
    std::string ReadFile(const std::filesystem::path& path)
    {
        std::ifstream file(path, std::ios::binary);
        return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    }

    bool Has(const std::string& haystack, const std::string& needle)
    {
        return haystack.find(needle) != std::string::npos;
    }

    bool Check(const char* name, bool ok)
    {
        std::cout << (ok ? "PASS|" : "FAIL|") << name << '\n';
        return ok;
    }
}

int main()
{
    const auto root = std::filesystem::current_path();
    const auto console = ReadFile(root / "Source" / "Public" / "ArhqenCognitionEngine" / "Ui" / "AceEngineConsole.h");
    const auto shellHeader = ReadFile(root / "Source" / "Public" / "ArhqenCognitionEngine" / "Ui" / "AceShellUi.h");
    const auto shellCpp = ReadFile(root / "Source" / "Private" / "Ui" / "AceShellUi.cpp");
    const auto docs = ReadFile(root / "Docs" / "ACE_PERF3_VIEWPORT_READBACK_CACHE.md");

    bool ok = true;
    ok &= Check("perf3_cached_render_path_enum", Has(console, "Dx12CachedReadback") && Has(console, "DX12_CACHED_READBACK"));
    ok &= Check("perf3_cache_key_state", Has(shellHeader, "AquariumViewportCacheKey") && Has(shellHeader, "aquariumViewportCacheKeyValid_"));
    ok &= Check("perf3_cache_key_tracks_camera_step_extent", Has(shellCpp, "StepIndex()") && Has(shellCpp, "CurrentScenarioName()") && Has(shellCpp, "Position()") && Has(shellCpp, "Yaw()") && Has(shellCpp, "Pitch()"));
    ok &= Check("perf3_cached_draw_path", Has(shellCpp, "drawCachedAquariumSlateViewportElement") && Has(shellCpp, "Dx12CachedReadback"));
    ok &= Check("perf3_cache_skips_rhi_timing", Has(shellCpp, "SetLastRhiRenderMs(0.0)") && Has(shellCpp, "SetLastAquariumBuildMs(0.0)"));
    ok &= Check("perf3_cache_invalidates_while_running_resize", Has(shellCpp, "!aquariumController_.IsRunning()") && Has(shellCpp, "!windowLiveResizeActive_"));
    ok &= Check("perf3_cache_updated_after_fresh_readback", Has(shellCpp, "aquariumViewportCacheKey_ = key") && Has(shellCpp, "++aquariumViewportCacheMissCount_"));
    ok &= Check("perf3_stats_surface_cache", Has(shellCpp, "viewport_cache_hits") && Has(shellCpp, "viewportCacheHits"));
    ok &= Check("perf3_no_rhi_or_renderer_file_dependency", !Has(shellCpp, "submitAndPresentBgra8ToComposition(") && !Has(shellCpp, "gpuOverlayBaked = true"));
    ok &= Check("perf3_docs", Has(docs, "ACE-PERF3") && Has(docs, "DX12_CACHED_READBACK") && Has(docs, "No RHI rewrite"));

    if (!ok)
    {
        return 1;
    }

    std::cout << "PASS|ace_perf3_viewport_readback_cache_probe\n";
    return 0;
}
