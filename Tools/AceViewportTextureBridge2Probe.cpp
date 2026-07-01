#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

static bool contains(const std::string& path, const std::string& needle)
{
    std::ifstream in(path, std::ios::binary);
    if (!in) { return false; }
    const std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    return text.find(needle) != std::string::npos;
}

static int pass(const char* name)
{
    std::cout << "PASS|" << name << "\n";
    return 0;
}

static int fail(const char* name)
{
    std::cout << "FAIL|" << name << "\n";
    return 1;
}

int main()
{
    int failures = 0;
    auto check = [&](const char* name, bool ok) { failures += ok ? pass(name) : fail(name); };

    const std::string shellCpp = "Source/Private/Ui/AceShellUi.cpp";
    const std::string shellH = "Source/Public/ArhqenCognitionEngine/Ui/AceShellUi.h";
    const std::string rhiH = "Source/Public/ArhqenCognitionEngine/Renderer/RHI/AceDx12Rhi.h";
    const std::string rhiCpp = "Source/Private/Renderer/RHI/AceDx12Rhi.cpp";
    const std::string sceneCpp = "Source/Private/Renderer/Scene/AceAquariumGpuViewportRenderer.cpp";
    const std::string consoleH = "Source/Public/ArhqenCognitionEngine/Ui/AceEngineConsole.h";

    check("vtbridge2_d3d11on12_used", contains(shellCpp, "D3D11On12CreateDevice"));
    check("vtbridge2_create_shared_bitmap_used", contains(shellCpp, "CreateSharedBitmap"));
    check("vtbridge2_dxgi_surface_used", contains(shellCpp, "IDXGISurface"));
    check("vtbridge2_no_gpu_debug_glyph_ui", !contains(shellCpp, "GpuOverlayText") && !contains(shellCpp, "block glyph"));
    check("vtbridge2_render_texture_without_readback", contains(sceneCpp, "preferD2DTextureBridge") && contains(sceneCpp, "no CPU readback requested"));
    check("vtbridge2_native_dx12_resource_accessors", contains(rhiH, "nativeD3D12TextureResource") && contains(rhiCpp, "nativeD3D12TextureResource"));
    check("vtbridge2_path_enum", contains(consoleH, "Dx12D2DTextureBridge") && contains(consoleH, "DX12_D2D_TEXTURE_BRIDGE"));
    check("vtbridge2_gpu_sampled_mode", contains("Source/Public/ArhqenCognitionEngine/Renderer/RHI/AceViewportTextureBridge.h", "GPU_SAMPLED_D2D"));
    check("vtbridge2_stats_counter", contains(rhiH, "d2dTextureBridgeFrames") && contains(shellCpp, "d2d_texture_bridge_frames"));
    check("vtbridge2_d3d11_linked", contains("Source/ArhqenCognitionEngine.vcxproj", "d3d11.lib"));
    check("vtbridge2_does_not_touch_log_selection", contains(shellH, "EngineLogTextPosition") && !contains(shellCpp, "engineLogSelectionPaletteRewrite"));

    return failures == 0 ? 0 : 1;
}
