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

static bool Has(const std::string& haystack, const std::string& needle)
{
    return haystack.find(needle) != std::string::npos;
}

static bool FileHas(const fs::path& p, const std::string& needle)
{
    return fs::exists(p) && Has(Read(p), needle);
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
    const std::string consoleH = Read("Source/Public/ArhqenCognitionEngine/Ui/AceEngineConsole.h");
    const std::string gpuH = Read("Source/Public/ArhqenCognitionEngine/Renderer/Scene/AceAquariumGpuViewportRenderer.h");
    const std::string gpuCpp = Read("Source/Private/Renderer/Scene/AceAquariumGpuViewportRenderer.cpp");
    const std::string dx12H = Read("Source/Public/ArhqenCognitionEngine/Renderer/RHI/AceDx12Rhi.h");
    const std::string dx12Cpp = Read("Source/Private/Renderer/RHI/AceDx12Rhi.cpp");
    const std::string docs = Read("Docs/ACE_PERF0_STATS_CONSOLE.md");
    const std::string layeringDocs = Read("Docs/ACE_AQ3D15_VIEWPORT_LAYERING.md");
    const std::string ui12Docs = Read("Docs/ACE_UI12_VIEWPORT_OVERLAY_POLISH.md");

    bool ok = true;
    ok &= Check("perf0_stat_coords_recognized", Has(shell, "id == L\"stat_coords\"") && Has(shell, "formatStatCoords"));
    ok &= Check("perf0_slash_stat_coords_supported", Has(shell, "aceTrimCommand") && Has(shell, "value.front() == L'/'"));
    ok &= Check("perf0_stat_rhi_recognized", Has(shell, "id == L\"stat_rhi\"") && Has(shell, "formatStatRhi"));
    ok &= Check("perf0_slash_stat_rhi_supported", Has(shell, "executeEngineStatsCommand(normalized)"));
    ok &= Check("perf0_stat_fps_recognized", Has(shell, "id == L\"stat_fps\"") && Has(shell, "formatStatFps"));
    ok &= Check("perf0_unknown_command_warning_no_backend", Has(shell, "Unknown local command") && Has(shell, "Not submitted to backend") && Has(shell, "UNKNOWN_COMMAND"));
    ok &= Check("perf0_rename_marker_preserved", Has(shell, "const std::wstring prefix = L\"/rename \""));
    ok &= Check("perf0_palette_has_stat_coords", Has(shell, "{L\"stat_coords\"") && Has(shell, "Engine coords"));
    ok &= Check("perf0_palette_has_stat_rhi", Has(shell, "{L\"stat_rhi\"") && Has(shell, "RHI stats"));
    ok &= Check("perf0_palette_has_stat_fps", Has(shell, "{L\"stat_fps\"") && Has(shell, "FPS stats"));
    ok &= Check("perf0_docs_exist", fs::exists("Docs/ACE_PERF0_STATS_CONSOLE.md") && Has(docs, "ACE-PERF0") && fs::exists("Docs/ACE_AQ3D15_VIEWPORT_LAYERING.md") && Has(layeringDocs, "UE-style Viewport Layering") && fs::exists("Docs/ACE_UI12_VIEWPORT_OVERLAY_POLISH.md") && Has(ui12Docs, "Stable Viewport Overlay Layering"));
    ok &= Check("perf0_log_helper_exists", Has(consoleH, "AceEngineAppendLog") && Has(consoleH, "Build") && Has(consoleH, "ace_engine.log"));
    ok &= Check("perf0_log_tail_reader_exists", Has(consoleH, "AceEngineReadLogTail") && Has(consoleH, "std::getline"));
    ok &= Check("perf0_log_overlay_backtick_toggle", Has(shell, "VK_OEM_3") && Has(shell, "toggleEngineLogOverlay") && Has(shell, "renderEngineLogOverlay"));
    ok &= Check("perf0_log_overlay_not_read_every_frame", Has(shell, "Build/Logs/ace_engine.log | local-only commands") && Has(shell, "engineLogOverlayVisible_"));
    ok &= Check("perf0_log_overlay_docked_to_viewport", Has(shell, "anchor = aquariumEmbeddedViewportRect_") && Has(shell, "ACE Engine Log Console"));
    ok &= Check("perf0_log_overlay_input_keyboard", Has(shellH, "engineLogOverlayInput_") && Has(shell, "submitEngineLogOverlayInput") && Has(shell, "Enter = run local command"));
    ok &= Check("perf0_log_overlay_scrollbar", Has(shellH, "engineLogOverlayScroll_") && Has(shell, "renderAquariumScrollbar(ctx, engineLogOverlayScroll_)") && Has(shell, "handleEngineLogOverlayWheel"));
    ok &= Check("perf0_log_overlay_unknown_no_backend", Has(shell, "source=engine_log_overlay") && Has(shell, "backend_submit=false"));
    ok &= Check("perf0_ue_style_viewport_layering", Has(shell, "shouldUseDirectCompositionForAquariumViewport") && Has(shell, "Slate/SViewport layering") && Has(shell, "parent-composited viewport"));
    ok &= Check("perf0_dcomp_reset_for_ui_overlay", Has(shell, "resetAquariumDirectCompositionIfActive") && Has(gpuH, "resetCompositionHost") && Has(dx12H, "resetCompositionHost") && Has(dx12Cpp, "impl_->resetComposition()"));
    ok &= Check("perf0_cached_viewport_bitmap", Has(shellH, "aquariumSlateViewportBitmap_") && Has(shell, "CopyFromMemory") && Has(shell, "drawAquariumGpuSnapshotAsSlateViewportElement"));
    ok &= Check("perf0_child_hwnd_clip_removed", Has(shell, "do not solve UI-over-viewport by resizing/clipping a child") && !Has(shell, "DX12 child viewport clipped by engine log UI") && !Has(shell, "overlay.top - 8.0f"));
    ok &= Check("perf0_ui12_deferred_dcomp_reset", Has(shellH, "aquariumResetDirectCompositionAfterPaint_") && Has(shell, "defer DirectComposition teardown") && Has(shell, "after EndDraw"));
    ok &= Check("perf0_ui12_parent_composited_hold", Has(shellH, "aquariumParentCompositedHoldFrames_") && Has(shell, "requestParentCompositedViewportHold") && Has(shell, "parent-layered-ui"));
    ok &= Check("perf0_ui12_stable_telemetry_layer", Has(shell, "renderAquariumViewportHudLayer") && Has(shell, "computeAquariumTelemetryOverlayRect") && Has(shell, "avoidOverlapsBottomHud"));
    ok &= Check("perf0_ui12_r1_no_zero_copy_over_hud", Has(shell, "ACE-PERF2") && Has(shell, "buildAquariumGpuViewportOverlay") && Has(ui12Docs, "GPU-composited viewport overlays"));
    ok &= Check("perf0_active_render_path_enum_string_exists", Has(consoleH, "AceEngineRenderPath") && Has(consoleH, "DX12_ZERO_COPY") && Has(consoleH, "CPU_D2D_FALLBACK"));
    ok &= Check("perf0_shell_tracks_active_render_path", Has(shellH, "aquariumActiveRenderPath_") && Has(shell, "AceEngineRenderPath::Dx12ZeroCopy") && Has(shell, "AceEngineRenderPath::Dx12Readback"));
    ok &= Check("perf0_gpu_stats_accessor_exists", Has(gpuH, "gpuStats() const") && Has(gpuCpp, "device_->gpuStats()"));

    const std::vector<std::string> rhiFields = {
        "zeroCopyFrames", "readbackFrames", "compositionFrames", "compositionResizes", "targetResizes",
        "framesRendered", "lastPrimitiveCount", "lastVertexCount", "lastExtent", "uploadBytesAllocated",
        "uploadAllocations", "nativeBuffers", "nativeTextures", "nativePipelines", "compiledShaders",
        "descriptorAllocations", "drawCallsExecuted", "submittedGpuCommandLists", "completedFenceValue",
        "readbackBytes", "offscreenSceneTargets", "wvpConstantsUploaded"
    };
    bool fields = true;
    for (const auto& field : rhiFields)
    {
        fields = fields && Has(shell, field);
    }
    ok &= Check("perf0_rhi_stats_fields_in_formatter", fields);

    ok &= Check("perf0_perf_stats_rolling_uses_chrono", Has(consoleH, "AceEnginePerfStats") && Has(consoleH, "kMaxSamples = 240") && Has(shell, "std::chrono::steady_clock"));
    ok &= Check("perf0_commands_not_submitted_before_backend", Has(shell, "if (handleLocalInputCommand(submittedText))") && shell.find("handleLocalInputCommand(submittedText)") < shell.find("submitHandler_(submittedText)"));
    ok &= Check("perf0_stat_ui_bonus_present", Has(shell, "id == L\"stat_ui\"") && Has(shell, "effect_cache_hits"));
    ok &= Check("perf0_clear_log_bonus_present", Has(shell, "id == L\"clear_log\"") && Has(shell, "std::ios::trunc"));

    std::cout << (ok ? "PASS|ace_perf0_stats_console_probe" : "FAIL|ace_perf0_stats_console_probe") << "\n";
    return ok ? 0 : 1;
}
