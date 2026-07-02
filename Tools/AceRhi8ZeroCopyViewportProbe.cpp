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
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

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
    AceAquariumGpuViewportOverlay overlay{};
    overlay.enabled = true;
    overlay.rects.push_back({12.0f, 246.0f, 210.0f, 58.0f, 0.02f, 0.08f, 0.12f, 0.94f});
    overlay.texts.push_back({24.0f, 258.0f, 1.8f, 0.62f, 0.96f, 1.0f, 1.0f, "AQUARIUM TELEMETRY"});
    if (renderer.render(primitives, 512, 320, false, &snapshot, &error, &overlay, hwnd, 20.0f, 20.0f, false) &&
        snapshot.valid &&
        snapshot.gpuRendered &&
        snapshot.zeroCopyPresented &&
        snapshot.overlayBaked)
    {
        pass("rhi8_directcomposition_zero_copy_overlay_presented");
    }
    else
    {
        fail("rhi8_directcomposition_zero_copy_overlay_presented", error.empty() ? snapshot.status : error);
    }

    Microsoft::WRL::ComPtr<ID3D11Device> overlayDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> overlayContext;
    D3D_FEATURE_LEVEL overlayFeatureLevel{};
    HRESULT overlayHr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        overlayDevice.GetAddressOf(),
        &overlayFeatureLevel,
        overlayContext.GetAddressOf());
    Microsoft::WRL::ComPtr<IDXGISwapChain1> overlaySwapChain;
    if (SUCCEEDED(overlayHr) && overlayDevice)
    {
        Microsoft::WRL::ComPtr<IDXGIDevice> overlayDxgiDevice;
        Microsoft::WRL::ComPtr<IDXGIAdapter> overlayAdapter;
        Microsoft::WRL::ComPtr<IDXGIFactory2> overlayFactory;
        overlayHr = overlayDevice.As(&overlayDxgiDevice);
        if (SUCCEEDED(overlayHr)) { overlayHr = overlayDxgiDevice->GetAdapter(overlayAdapter.GetAddressOf()); }
        if (SUCCEEDED(overlayHr)) { overlayHr = overlayAdapter->GetParent(IID_PPV_ARGS(overlayFactory.GetAddressOf())); }
        if (SUCCEEDED(overlayHr))
        {
            DXGI_SWAP_CHAIN_DESC1 overlayDesc{};
            overlayDesc.Width = 240;
            overlayDesc.Height = 80;
            overlayDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            overlayDesc.SampleDesc.Count = 1;
            overlayDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            overlayDesc.BufferCount = 2;
            overlayDesc.Scaling = DXGI_SCALING_STRETCH;
            overlayDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
            overlayDesc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;
            overlayHr = overlayFactory->CreateSwapChainForComposition(
                overlayDevice.Get(), &overlayDesc, nullptr, overlaySwapChain.GetAddressOf());
        }
    }

    if (SUCCEEDED(overlayHr) && overlaySwapChain &&
        renderer.setCompositionOverlay(overlaySwapChain.Get(), 30.0f, 350.0f, {240, 80}, &error))
    {
        pass("rhi8_separate_d3d11_ui_visual_above_dx12_scene");
    }
    else
    {
        fail("rhi8_separate_d3d11_ui_visual_above_dx12_scene", error);
    }

    const auto throughputBefore = renderer.gpuStats();
    for (int frame = 0; frame < 64; ++frame)
    {
        AceAquariumGpuViewportSnapshot pacingSnapshot{};
        if (!renderer.render(primitives, 512, 320, false, &pacingSnapshot, &error, nullptr, hwnd, 20.0f, 20.0f, false))
        {
            fail("rhi8_render_throughput_decoupled_from_present_gate", error);
            break;
        }
    }
    const auto pacingStats = renderer.gpuStats();
    const auto throughputDraws = pacingStats.drawCallsExecuted - throughputBefore.drawCallsExecuted;
    if (pacingStats.compositionPacingSkips > 0 &&
        pacingStats.compositionRenderOnlyFrames > 0 &&
        throughputDraws >= 64)
    {
        pass("rhi8_render_throughput_decoupled_from_present_gate");
    }
    else
    {
        fail("rhi8_render_throughput_decoupled_from_present_gate", "DXGI pacing discarded scene draw submissions");
    }

    const auto stats = renderer.stats();
    if (stats.framesRendered >= 1 && stats.zeroCopyFrames >= 1 && stats.lastVertexCount > 0 &&
        stats.gpuOverlayBakedFrames >= 1 && stats.gpuOverlayVertexCount > 0)
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
