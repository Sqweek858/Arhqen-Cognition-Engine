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
    const char* shellCpp = "Source/Private/Ui/AceShellUi.cpp";
    const char* docs = "Docs/ACE_VTBRIDGE1_D2D_TEXTURE_SAMPLING_BRIDGE.md";

    fails += check(contains(bridge, "AceViewportUiRendererKind"), "vtbridge1_ui_renderer_kind_exists");
    fails += check(contains(bridge, "LegacyD2DHwndRenderTarget"), "vtbridge1_legacy_hwnd_target_mode");
    fails += check(contains(bridge, "D2DDeviceContext"), "vtbridge1_required_d2d_device_context");
    fails += check(contains(bridge, "D3D11On12DxgiSurface"), "vtbridge1_required_d3d11on12_dxgi_surface");
    fails += check(contains(bridge, "ui_renderer_legacy_hwnd_render_target_needs_d2d_device_context_bridge"), "vtbridge1_explicit_current_blocker");
    fails += check(contains(shellCpp, "viewport_ui_renderer"), "vtbridge1_stat_reports_ui_renderer");
    fails += check(contains(shellCpp, "viewport_required_ui_renderer"), "vtbridge1_stat_reports_required_ui_renderer");
    fails += check(contains(shellCpp, "viewport_required_interop"), "vtbridge1_stat_reports_required_interop");
    fails += check(contains(docs, "ID2D1HwndRenderTarget"), "vtbridge1_docs_explain_legacy_target");
    fails += check(contains(docs, "D3D11On12"), "vtbridge1_docs_explain_next_bridge");
    fails += check(!contains(shellCpp, "gpu debug glyph"), "vtbridge1_no_gpu_debug_glyph_ui_added");
    fails += check(!contains(shellCpp, "CopyFromMemory viewport bridge"), "vtbridge1_no_cpu_copy_bridge_added");

    return fails == 0 ? 0 : 1;
}
