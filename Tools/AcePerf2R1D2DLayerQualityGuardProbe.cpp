#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

static std::string Read(const fs::path& p)
{
    std::ifstream in(p, std::ios::binary);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

static bool Has(const std::string& haystack, const std::string& needle)
{
    return haystack.find(needle) != std::string::npos;
}

static bool Check(const char* name, bool pass)
{
    std::cout << (pass ? "PASS|" : "FAIL|") << name << "\n";
    return pass;
}

int main()
{
    const std::string shell = Read("Source/Private/Ui/AceShellUi.cpp");
    const std::string shellH = Read("Source/Public/ArhqenCognitionEngine/Ui/AceShellUi.h");
    const std::string gpuH = Read("Source/Public/ArhqenCognitionEngine/Renderer/Scene/AceAquariumGpuViewportRenderer.h");
    const std::string gpuCpp = Read("Source/Private/Renderer/Scene/AceAquariumGpuViewportRenderer.cpp");
    const std::string telemetryCpp = Read("Source/Private/Ui/D2D/D2DAquariumTelemetryWidgets.cpp");
    const std::string consoleH = Read("Source/Public/ArhqenCognitionEngine/Ui/AceEngineConsole.h");
    const std::string docs = Read("Docs/ACE_PERF2R1_D2D_LAYER_QUALITY_GUARD.md");
    const std::string perf2Docs = Read("Docs/ACE_PERF2_GPU_COMPOSITED_VIEWPORT.md");

    bool ok = true;
    ok &= Check("perf2r1_docs_exist", fs::exists("Docs/ACE_PERF2R1_D2D_LAYER_QUALITY_GUARD.md") && Has(docs, "ACE-PERF2R1") && Has(docs, "D2D_RETAINED_OVERLAY"));
    ok &= Check("perf2r1_d2d_quality_gate", Has(shell, "ACE-PERF2R1: visual quality guard") && Has(shell, "return false;") && Has(shell, "D2D/DWrite UI"));
    ok &= Check("perf2r1_normal_render_passes_no_gpu_overlay", Has(shell, "nullptr,") && !Has(shell, "useDirectComposition ? &gpuOverlay : nullptr"));
    ok &= Check("perf2r1_d2d_log_console_preserved", Has(shell, "void AceShellUi::renderEngineLogOverlay") && Has(shell, "D2DWidgetUtils::drawText") && Has(shell, "engineLogOverlayInput_.render(ctx)"));
    ok &= Check("perf2r1_d2d_telemetry_preserved", Has(shell, "renderAquariumViewportHudLayer") && Has(telemetryCpp, "D2DAquariumTelemetryWidgets::RenderOverlay"));
    ok &= Check("perf2r1_stats_quality_marker", Has(shell, "ui_layer=D2D_RETAINED_OVERLAY") && Has(shell, "quality=preserved") && Has(shell, "gpu_text_overlay=false"));
    ok &= Check("perf2r1_gpu_overlay_not_default", Has(gpuH, "AceAquariumGpuViewportOverlay") && Has(gpuCpp, "AcePerf2_ViewportGpuOverlayPass") && Has(perf2Docs, "no longer the normal Environment 3D shell path"));
    ok &= Check("perf2r1_no_child_hwnd_clip_regression", !Has(shell, "computeAquariumNativeViewportRect(rect).inset") && Has(shell, "parent-composited"));
    ok &= Check("perf2r1_no_new_stat_frame", !Has(shell, "stat_frame") && Has(shell, "id == L\"stat_fps\""));
    ok &= Check("perf2r1_enum_kept_for_diagnostics", Has(consoleH, "DX12_GPU_COMPOSITED") && Has(consoleH, "DX12_COMBINED_READBACK"));

    std::cout << (ok ? "PASS|ace_perf2r1_d2d_layer_quality_guard_probe" : "FAIL|ace_perf2r1_d2d_layer_quality_guard_probe") << "\n";
    return ok ? 0 : 1;
}
