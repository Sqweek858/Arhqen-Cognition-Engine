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
    const std::string dx12H = Read("Source/Public/ArhqenCognitionEngine/Renderer/RHI/AceDx12Rhi.h");
    const std::string dx12Cpp = Read("Source/Private/Renderer/RHI/AceDx12Rhi.cpp");
    const std::string consoleH = Read("Source/Public/ArhqenCognitionEngine/Ui/AceEngineConsole.h");
    const std::string docs = Read("Docs/ACE_PERF2_GPU_COMPOSITED_VIEWPORT.md");

    bool ok = true;
    ok &= Check("perf2_docs_exist", fs::exists("Docs/ACE_PERF2_GPU_COMPOSITED_VIEWPORT.md") && Has(docs, "ACE-PERF2") && Has(docs, "SViewport") && Has(docs, "FSlateDrawElement::MakeViewport"));
    ok &= Check("perf2_render_path_enum", Has(consoleH, "Dx12GpuComposited") && Has(consoleH, "DX12_GPU_COMPOSITED"));
    ok &= Check("perf2_gpu_overlay_struct", Has(gpuH, "AceAquariumGpuViewportOverlay") && Has(gpuH, "AceAquariumGpuOverlayRect") && Has(gpuH, "AceAquariumGpuOverlayText"));
    ok &= Check("perf2_gpu_overlay_builder", Has(shellH, "buildAquariumGpuViewportOverlay") && Has(shell, "buildAquariumGpuViewportOverlay") && Has(shell, "engineLogOverlayVisible_"));
    ok &= Check("perf2_overlay_gpu_pass", Has(gpuCpp, "AcePerf2_ViewportGpuOverlayPass") && Has(gpuCpp, "LoadOp::Load") && Has(gpuCpp, "overlayPipeline_"));
    ok &= Check("perf2_block_glyph_overlay", Has(gpuCpp, "aceGpuGlyph") && Has(gpuCpp, "pushOverlayTextPx") && Has(gpuCpp, "pushOverlayRectPx"));
    ok &= Check("perf2_gpu_composition_experiment_kept", Has(gpuCpp, "submitAndPresentBgra8ToComposition") && Has(gpuCpp, "gpuComposited = true") && Has(gpuCpp, "one combined GPU composition command list"));
    ok &= Check("perf2_one_command_composition_api", Has(dx12H, "submitAndPresentBgra8ToComposition") && Has(dx12Cpp, "submitAndPresentBgra8ToComposition") && Has(dx12Cpp, "DirectComposition swapchain copy into one GPU command list"));
    ok &= Check("perf2_composition_stats", Has(dx12H, "gpuCompositedFrames") && Has(dx12H, "combinedGpuCompositionFrames") && Has(dx12H, "gpuOverlayBakedFrames") && Has(dx12H, "gpuOverlayVertices"));
    ok &= Check("perf2_stats_visible", Has(shell, "gpuCompositedFrames=") && Has(shell, "combinedGpuCompositionFrames=") && Has(shell, "readback=") && Has(shell, "combined_gpu_composition_frames="));
    ok &= Check("perf2r1_visual_quality_guard", Has(shell, "ACE-PERF2R1: visual quality guard") && Has(shell, "ui_layer=D2D_RETAINED_OVERLAY") && Has(shell, "gpu_text_overlay=false"));
    ok &= Check("perf2r1_no_default_gpu_overlay_param", !Has(shell, "useDirectComposition ? &gpuOverlay : nullptr") && Has(shell, "nullptr,"));
    ok &= Check("perf2_fallback_kept", Has(gpuCpp, "submitAndReadbackBgra8") && Has(gpuCpp, "readback fallback"));
    ok &= Check("perf2_no_stale_readback_fallback", Has(gpuCpp, "do not read stale pixels") && Has(gpuCpp, "fallbackReadbackOk"));
    ok &= Check("perf2_no_new_stat_frame", !Has(shell, "stat_frame") && Has(shell, "id == L\"stat_fps\""));

    std::cout << (ok ? "PASS|ace_perf2_gpu_composited_viewport_probe" : "FAIL|ace_perf2_gpu_composited_viewport_probe") << "\n";
    return ok ? 0 : 1;
}
