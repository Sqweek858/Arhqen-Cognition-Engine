#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

static std::string readFile(const char* path)
{
    std::ifstream f(path, std::ios::binary);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static bool contains(const std::string& s, const std::string& needle)
{
    return s.find(needle) != std::string::npos;
}

static int check(const char* name, bool ok)
{
    std::cout << (ok ? "PASS|" : "FAIL|") << name << "\n";
    return ok ? 0 : 1;
}

int main()
{
    const std::string d2dCtx = readFile("Source/Public/ArhqenCognitionEngine/Ui/D2D/D2DRenderContext.h");
    const std::string shellH = readFile("Source/Public/ArhqenCognitionEngine/Ui/AceShellUi.h");
    const std::string shellCpp = readFile("Source/Private/Ui/AceShellUi.cpp");
    const std::string bridgeH = readFile("Source/Public/ArhqenCognitionEngine/Renderer/RHI/AceViewportTextureBridge.h");
    const std::string consoleH = readFile("Source/Public/ArhqenCognitionEngine/Ui/AceEngineConsole.h");
    const std::string doc = readFile("Docs/ACE_VTBRIDGE4_D2D_DEVICE_CONTEXT_TARGET.md");

    int fails = 0;
    fails += check("vtbridge4_context_accepts_modern_target", contains(d2dCtx, "ID2D1RenderTarget* target") && !contains(d2dCtx, "ID2D1HwndRenderTarget* target"));
    fails += check("vtbridge4_ui_device_context_fields", contains(shellH, "IDXGISwapChain1") && contains(shellH, "ID2D1DeviceContext") && contains(shellH, "uiD2DTargetBitmap_"));
    fails += check("vtbridge4_backbuffer_target_created_from_dxgi_surface", contains(shellCpp, "CreateSwapChainForHwnd") && contains(shellCpp, "createD2DDeviceContextBackbufferTarget") && contains(shellCpp, "CreateBitmapFromDxgiSurface"));
    fails += check("vtbridge4_present_uses_ui_swapchain", contains(shellCpp, "uiSwapChain_->Present"));
    fails += check("vtbridge4_draws_viewport_with_device_context", contains(shellCpp, "drawAquariumGpuTextureWithD2DDeviceContext") && contains(shellCpp, "ID2D1DeviceContext::CreateBitmapFromDxgiSurface viewport bridge"));
    fails += check("vtbridge4_no_legacy_shared_bitmap_call", !contains(shellCpp, "CreateSharedBitmap("));
    fails += check("vtbridge4_no_combined_readback_fallback_call", !contains(shellCpp, "renderGpuViewport(false)"));
    fails += check("vtbridge4_cached_readback_disabled", contains(shellCpp, "if (false && drawCachedAquariumSlateViewportElement"));
    fails += check("vtbridge4_failure_mode_enum_and_stats", contains(consoleH, "FailedD2DDeviceContext") && contains(consoleH, "FAILED_D2D_DEVICE_CONTEXT") && contains(shellCpp, "fatal_bridge_step") && contains(shellCpp, "fatal_bridge_hresult"));
    fails += check("vtbridge4_bridge_failure_status", contains(bridgeH, "AceViewportTextureBridgeMode::Failed") && contains(bridgeH, "MakeAceViewportD2DDeviceContextFailure") && contains(bridgeH, "legacyFallbackUsed"));
    fails += check("vtbridge4_no_legacy_fallback_stats", contains(shellCpp, "legacy_fallback") && contains(shellCpp, "readbackActive") && contains(shellCpp, "FailedD2DDeviceContext"));
    fails += check("vtbridge4_docs", contains(doc, "D2D DeviceContext Target") && contains(doc, "no legacy fallback"));
    return fails == 0 ? 0 : 1;
}
