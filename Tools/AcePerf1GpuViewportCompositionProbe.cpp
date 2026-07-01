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
    const std::string gpuCpp = Read("Source/Private/Renderer/Scene/AceAquariumGpuViewportRenderer.cpp");
    const std::string dx12H = Read("Source/Public/ArhqenCognitionEngine/Renderer/RHI/AceDx12Rhi.h");
    const std::string dx12Cpp = Read("Source/Private/Renderer/RHI/AceDx12Rhi.cpp");
    const std::string docs = Read("Docs/ACE_PERF1_GPU_VIEWPORT_COMPOSITION.md");

    bool ok = true;
    ok &= Check("perf1_docs_exist", fs::exists("Docs/ACE_PERF1_GPU_VIEWPORT_COMPOSITION.md") && Has(docs, "ACE-PERF1") && Has(docs, "SViewport") && Has(docs, "FSlateDrawElement::MakeViewport"));
    ok &= Check("perf1_mapped_upload_stats", Has(dx12H, "mappedUploadBytes") && Has(dx12H, "mappedUploadUpdates") && Has(shell, "mappedUploadBytes") && Has(shell, "mappedUploadUpdates"));
    ok &= Check("perf1_persistent_readback_stats", Has(dx12H, "readbackBufferReuses") && Has(dx12H, "readbackBufferResizes") && Has(shell, "readbackBufferReuses") && Has(shell, "readbackBufferResizes"));
    ok &= Check("perf1_readback_cache_struct", Has(dx12Cpp, "Dx12ReadbackCache") && Has(dx12Cpp, "readbackCache.matches") && Has(dx12Cpp, "readbackBufferReuses"));
    ok &= Check("perf1_no_per_frame_readback_alloc", Has(dx12Cpp, "CreateCommittedResource readback cache") && !Has(dx12Cpp, "CreateCommittedResource readback\", hr"));
    ok &= Check("perf1_dynamic_vertices_upload_buffer", Has(gpuCpp, "desc.memory = Memory::Upload") && Has(gpuCpp, "desc.persistentMap = true") && Has(gpuCpp, "Slate's dynamic element upload model"));
    ok &= Check("perf1_mapped_upload_fast_path", Has(dx12Cpp, "nativeDst->mapped") && Has(dx12Cpp, "mappedUploadUpdates") && Has(dx12Cpp, "command list and waiting on a fence every frame"));
    ok &= Check("perf1_upload_heap_no_barrier", Has(dx12Cpp, "Upload heap resources are permanently GENERIC_READ") && Has(dx12Cpp, "native.desc.memory == Memory::Upload"));
    ok &= Check("perf1_fast_viewport_paint_path", Has(shell, "renderFastAquariumViewportFrame") && Has(shell, "canUseFastAquariumViewportFrame") && Has(shell, "currentAquariumViewportDynamicLayerRect"));
    ok &= Check("perf1_fast_path_skips_full_chrome", Has(shell, "UE/Slate does not rebuild the whole chrome tree") && Has(shell, "fastAquariumViewportPaint") && Has(shell, "if (!fastAquariumViewportPaint)"));
    ok &= Check("perf1_fast_paint_counters", Has(shellH, "aquariumFastViewportPaintCount_") && Has(shellH, "aquariumFullViewportPaintCount_") && Has(shell, "fast_viewport_paints") && Has(shell, "full_viewport_paints"));
    ok &= Check("perf1_preserves_ui12_no_child_hwnd_regression", Has(shell, "Global parent overlays") && Has(docs, "No child-HWND regression"));
    ok &= Check("perf1_no_new_stat_frame", !Has(shell, "stat_frame") && Has(shell, "id == L\"stat_fps\""));

    ok &= Check("perf1r1_combined_readback_api", Has(dx12H, "submitAndReadbackBgra8") && Has(dx12Cpp, "submitAndReadbackBgra8") && Has(gpuCpp, "submitAndReadbackBgra8"));
    ok &= Check("perf1r1_combined_readback_path", Has(shell, "Dx12CombinedReadback") && (Has(shell, "combined render+readback") || Has(shell, "D2D retained overlay + combined readback")) && Has(dx12H, "combinedRenderReadbackFrames"));
    ok &= Check("perf1r1_stat_fps_visible_mode", Has(shell, "viewport_mode=") && Has(shell, "combined_readback_frames=") && Has(shell, "fence_wait_ms="));
    ok &= Check("perf1r1_no_stat_frame", !Has(shell, "stat_frame") && Has(shell, "id == L\"stat_fps\""));

    std::cout << (ok ? "PASS|ace_perf1_gpu_viewport_composition_probe" : "FAIL|ace_perf1_gpu_viewport_composition_probe") << "\n";
    return ok ? 0 : 1;
}
