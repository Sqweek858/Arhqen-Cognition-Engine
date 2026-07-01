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
    const std::string doc = readFile("Docs/ACE_VTBRIDGE4R2_SHARED_TEXTURE_RING_SYNC.md");

    int fails = 0;
    fails += check("vtbridge4r2_has_shared_slot_ring",
        contains(shellH, "AquariumD2DSharedBridgeSlot") &&
        contains(shellH, "kAquariumD2DSharedBridgeSlotCount") &&
        contains(shellH, "aquariumBridgeSharedSlots_"));
    fails += check("vtbridge4r2_uses_keyed_mutex_when_available",
        contains(shellCpp, "D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX") &&
        contains(shellCpp, "IDXGIKeyedMutex::AcquireSync") &&
        contains(shellCpp, "ReleaseSync"));
    fails += check("vtbridge4r2_keeps_gpu_only_legacy_shared_fallback",
        contains(shellCpp, "keyed_mutex_fallback=legacy_shared_ring") &&
        contains(shellCpp, "D3D11_RESOURCE_MISC_SHARED") &&
        !contains(shellCpp, "renderGpuViewport(false)"));
    fails += check("vtbridge4r2_ring_copy_and_draw_indices",
        contains(shellCpp, "aquariumBridgeSharedWriteIndex_") &&
        contains(shellCpp, "aquariumBridgeSharedReadyIndex_") &&
        contains(shellCpp, "d2d_shared_write_index") &&
        contains(shellCpp, "d2d_shared_draw_index"));
    fails += check("vtbridge4r2_flushes_d2d_keyed_draw",
        contains(shellCpp, "ID2D1DeviceContext::Flush D2D shared viewport bridge") &&
        contains(shellCpp, "aquariumD2DBridgeSharedD2DFlushCount_"));
    fails += check("vtbridge4r2_stats",
        contains(shellCpp, "d2d_shared_buffer_count") &&
        contains(shellCpp, "d2d_shared_bitmap_recreates") &&
        contains(shellCpp, "d2d_shared_mutex_contentions"));
    fails += check("vtbridge4r2_docs",
        contains(doc, "shared texture ring") &&
        contains(doc, "no-fallback rule") &&
        contains(doc, "selected=shared_ui_d3d11_texture_ring_keyed"));
    return fails == 0 ? 0 : 1;
}
