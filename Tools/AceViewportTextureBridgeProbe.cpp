#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace
{
    bool contains(const std::string& path, const std::string& needle)
    {
        std::ifstream in(path, std::ios::binary);
        if (!in) { return false; }
        std::ostringstream ss;
        ss << in.rdbuf();
        return ss.str().find(needle) != std::string::npos;
    }

    int check(bool ok, const char* name)
    {
        std::cout << (ok ? "PASS|" : "FAIL|") << name << "\n";
        return ok ? 0 : 1;
    }
}

int main()
{
    int fails = 0;
    const char* bridge = "Source/Public/ArhqenCognitionEngine/Renderer/RHI/AceViewportTextureBridge.h";
    const char* rendererH = "Source/Public/ArhqenCognitionEngine/Renderer/Scene/AceAquariumGpuViewportRenderer.h";
    const char* rendererCpp = "Source/Private/Renderer/Scene/AceAquariumGpuViewportRenderer.cpp";
    const char* consoleH = "Source/Public/ArhqenCognitionEngine/Ui/AceEngineConsole.h";
    const char* shellCpp = "Source/Private/Ui/AceShellUi.cpp";

    fails += check(contains(bridge, "IAceViewportTextureSource"), "vtbridge_interface_exists");
    fails += check(contains(bridge, "AceViewportTextureResource"), "vtbridge_resource_descriptor_exists");
    fails += check(contains(bridge, "EvaluateAceViewportTextureBridge"), "vtbridge_status_evaluator_exists");
    fails += check(contains(rendererH, "viewportTextureResource() const"), "aquarium_renderer_exposes_viewport_texture");
    fails += check(contains(rendererCpp, "MakeAceRhiViewportTextureResource"), "aquarium_scene_color_exported_as_rhi_texture");
    fails += check(contains(consoleH, "AceViewportTextureBridgeStatus viewportBridge"), "engine_stats_carry_bridge_status");
    fails += check(contains(shellCpp, "viewport_texture_bridge"), "stat_output_reports_bridge_mode");
    fails += check(contains(bridge, "ui_renderer_cannot_sample_gpu_viewport_texture_yet"), "fallback_reason_is_explicit");
    fails += check(!contains(shellCpp, "gpu debug glyph"), "no_gpu_debug_glyph_ui_added");
    fails += check(!contains(rendererCpp, "CopyFromMemory viewport bridge"), "no_new_cpu_copy_bridge_path");

    return fails == 0 ? 0 : 1;
}
