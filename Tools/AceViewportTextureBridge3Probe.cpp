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
    const std::string doc = readFile("Docs/ACE_VTBRIDGE3_D2D_DEVICE_CONTEXT_COMPAT_BITMAP.md");
    int fails = 0;
    fails += check("vtbridge3_d2d1_1_included", contains(shellH, "<d2d1_1.h>") && contains(shellCpp, "<d2d1_1.h>"));
    fails += check("vtbridge3_d2d_device_context_fields", contains(shellH, "ID2D1DeviceContext") && contains(shellH, "aquariumBridgeSurfaceBitmap_"));
    fails += check("vtbridge3_interop_device_helper", contains(shellCpp, "ensureAquariumD2DInteropDevices"));
    fails += check("vtbridge3_create_bitmap_from_dxgi_surface", contains(shellCpp, "CreateBitmapFromDxgiSurface"));
    fails += check("vtbridge3_create_shared_bitmap_from_d2d_bitmap", contains(shellCpp, "__uuidof(ID2D1Bitmap)") && contains(shellCpp, "CreateSharedBitmap from D2D device-context bitmap"));
    fails += check("vtbridge3_last_error_forwarded_to_stats", contains(shellCpp, "aquariumD2DBridgeLastError_") && contains(shellCpp, "D3D11On12SharedBitmap"));
    fails += check("vtbridge3_no_gpu_debug_glyph_ui", !contains(shellCpp, "GPU debug glyph"));
    fails += check("vtbridge3_docs", contains(doc, "D2D DeviceContext Compatibility Bitmap Bridge"));
    return fails == 0 ? 0 : 1;
}
