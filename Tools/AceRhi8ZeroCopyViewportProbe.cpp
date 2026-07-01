#include "ArhqenCognitionEngine/Renderer/Scene/AceAquariumGpuViewportRenderer.h"

#include <iostream>
#include <string>
#include <vector>

namespace
{
    int failures = 0;

    void pass(const std::string& name)
    {
        std::cout << "PASS|" << name << "\n";
    }

    void fail(const std::string& name, const std::string& detail = {})
    {
        ++failures;
        std::cout << "FAIL|" << name;
        if (!detail.empty()) { std::cout << "|" << detail; }
        std::cout << "\n";
    }
}

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace
{
    LRESULT CALLBACK ProbeWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
    {
        return DefWindowProcW(hwnd, msg, wp, lp);
    }

    HWND createProbeWindow()
    {
        HINSTANCE instance = GetModuleHandleW(nullptr);
        const wchar_t* cls = L"AceRhi8ZeroCopyViewportProbeWindow";

        WNDCLASSW wc{};
        wc.lpfnWndProc = ProbeWndProc;
        wc.hInstance = instance;
        wc.lpszClassName = cls;
        RegisterClassW(&wc);

        return CreateWindowExW(
            0,
            cls,
            L"ACE RHI8 Zero Copy Probe",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            800,
            600,
            nullptr,
            nullptr,
            instance,
            nullptr);
    }
}
#endif

int main()
{
    using namespace am::renderer::scene;
    using ace::aquarium_render::AceAqRenderPrimitive;
    using ace::aquarium_render::AceAqRenderPrimitiveKind;

#if !defined(_WIN32)
    AceAquariumGpuViewportRenderer renderer;
    renderer.setWorldToClipMatrix({
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    });
    pass("rhi7_rhi8_probe_compiles_non_windows");
    return 0;
#else
    HWND hwnd = createProbeWindow();
    if (!hwnd)
    {
        fail("rhi8_probe_window_created", "CreateWindowExW failed.");
        return 1;
    }
    pass("rhi8_probe_window_created");

    std::vector<AceAqRenderPrimitive> primitives;
    for (int z = 0; z < 6; ++z)
    {
        for (int x = 0; x < 6; ++x)
        {
            AceAqRenderPrimitive tile{};
            tile.Kind = AceAqRenderPrimitiveKind::Tile;
            tile.X = static_cast<float>(x);
            tile.Y = static_cast<float>(z);
            tile.SizeX = 0.92f;
            tile.SizeY = 0.92f;
            tile.R = 0.05f;
            tile.G = 0.18f;
            tile.B = 0.26f;
            tile.A = 0.75f;
            primitives.push_back(tile);
        }
    }

    AceAqRenderPrimitive block{};
    block.Kind = AceAqRenderPrimitiveKind::Block;
    block.X = 2.0f;
    block.Y = 2.0f;
    block.SizeX = 1.0f;
    block.SizeY = 1.0f;
    block.SizeZ = 1.0f;
    block.R = 0.75f;
    block.G = 0.16f;
    block.B = 0.18f;
    block.A = 0.94f;
    primitives.push_back(block);

    AceAqRenderPrimitive agent{};
    agent.Kind = AceAqRenderPrimitiveKind::Agent;
    agent.X = 3.0f;
    agent.Y = 3.0f;
    agent.SizeX = 0.72f;
    agent.SizeY = 0.72f;
    primitives.push_back(agent);

    AceAquariumGpuViewportRenderer renderer;

    // Simple top-down-ish matrix for the probe. The app path uses the real camera.
    renderer.setWorldToClipMatrix({
        0.16f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.15f, 0.0f,
        0.0f, -0.20f, 0.0f, 0.0f,
       -1.30f, 0.50f, 0.50f, 1.0f
    });

    std::string error;
    AceAquariumGpuViewportSnapshot snapshot{};
    if (renderer.render(primitives, 512, 320, false, &snapshot, &error, hwnd, 20.0f, 20.0f) &&
        snapshot.valid &&
        snapshot.gpuRendered &&
        snapshot.zeroCopyPresented)
    {
        pass("rhi8_directcomposition_zero_copy_presented");
    }
    else
    {
        fail("rhi8_directcomposition_zero_copy_presented", error.empty() ? snapshot.status : error);
    }

    const auto stats = renderer.stats();
    if (stats.framesRendered >= 1 && stats.zeroCopyFrames >= 1 && stats.lastVertexCount > 0)
    {
        pass("rhi7_rhi8_gpu_3d_zero_copy_stats_valid");
    }
    else
    {
        fail("rhi7_rhi8_gpu_3d_zero_copy_stats_valid", "unexpected renderer stats");
    }

    renderer.shutdown();
    DestroyWindow(hwnd);
    return failures == 0 ? 0 : 1;
#endif
}
