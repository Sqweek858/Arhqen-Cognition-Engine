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
    const std::string shellH = readFile("Source/Public/ArhqenCognitionEngine/Ui/AceShellUi.h");
    const std::string shellCpp = readFile("Source/Private/Ui/AceShellUi.cpp");
    const std::string bridgeH = readFile("Source/Public/ArhqenCognitionEngine/Renderer/RHI/AceViewportTextureBridge.h");
    const std::string doc = readFile("Docs/ACE_VTBRIDGE4R1_D2D_SHARED_INTERMEDIATE.md");

    int fails = 0;
    fails += check("vtbridge4r1_has_shared_intermediate_fields",
        contains(shellH, "aquariumBridgeUiSharedTexture_") &&
        contains(shellH, "aquariumBridgeInteropSharedTexture_") &&
        contains(shellH, "aquariumBridgeUsesSharedIntermediate_"));
    fails += check("vtbridge4r1_logs_surface_descriptors",
        contains(shellCpp, "aceD3D12ResourceDescText") &&
        contains(shellCpp, "aceDxgiSurfaceDescText") &&
        contains(shellCpp, "d2d_texture_bridge_surface_diagnostics"));
    fails += check("vtbridge4r1_direct_surface_variants",
        contains(shellCpp, "direct_exact_ignore") &&
        contains(shellCpp, "direct_unknown_ignore") &&
        contains(shellCpp, "direct_bgra_premul"));
    fails += check("vtbridge4r1_shared_texture_path",
        contains(shellCpp, "D3D11_RESOURCE_MISC_SHARED") &&
        contains(shellCpp, "GetSharedHandle") &&
        contains(shellCpp, "OpenSharedResource") &&
        contains(shellCpp, "CopyResource"));
    fails += check("vtbridge4r1_shared_surface_becomes_d2d_bitmap",
        contains(shellCpp, "ui_shared_bgra_ignore") &&
        contains(shellCpp, "ui_shared_unknown_ignore") &&
        contains(shellCpp, "selected=shared_ui_d3d11_texture"));
    fails += check("vtbridge4r1_success_counters_are_ui_side",
        contains(shellH, "aquariumD2DBridgeAttemptCount_") &&
        contains(shellH, "aquariumD2DBridgeSuccessCount_") &&
        contains(shellCpp, "d2d_texture_bridge_attempts") &&
        contains(shellCpp, "d2d_texture_bridge_successes"));
    fails += check("vtbridge4r1_no_closehandle_for_legacy_shared_handle",
        contains(shellCpp, "not an NT handle") &&
        !contains(shellCpp, "CloseHandle(aquariumBridgeSharedHandle_)"));
    fails += check("vtbridge4r1_interop_kind_names_shared_texture",
        contains(bridgeH, "D3D11On12SharedD3D11Texture") &&
        contains(bridgeH, "D3D11ON12_SHARED_D3D11_TEXTURE"));
    fails += check("vtbridge4r1_docs",
        contains(doc, "shared intermediate") &&
        contains(doc, "no CPU pixel copy fallback"));
    return fails == 0 ? 0 : 1;
}
