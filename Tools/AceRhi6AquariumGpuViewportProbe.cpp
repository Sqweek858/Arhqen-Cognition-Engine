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

int main()
{
    using namespace am::renderer::scene;
    using ace::aquarium_render::AceAqRenderPrimitive;
    using ace::aquarium_render::AceAqRenderPrimitiveKind;

#if !defined(_WIN32)
    AceAquariumGpuViewportRenderer renderer;
    if (!renderer.initialized())
    {
        pass("rhi6_gpu_viewport_type_compiles_non_windows");
    }
    return failures == 0 ? 0 : 1;
#else
    std::string error;
    std::vector<AceAqRenderPrimitive> primitives;

    for (int y = 0; y < 8; ++y)
    {
        for (int x = 0; x < 8; ++x)
        {
            AceAqRenderPrimitive tile{};
            tile.Kind = AceAqRenderPrimitiveKind::Tile;
            tile.X = static_cast<float>(x);
            tile.Y = static_cast<float>(y);
            tile.SizeX = 0.92f;
            tile.SizeY = 0.92f;
            tile.R = 0.05f;
            tile.G = 0.22f;
            tile.B = 0.28f;
            tile.A = 0.74f;
            primitives.push_back(tile);
        }
    }

    AceAqRenderPrimitive block{};
    block.Kind = AceAqRenderPrimitiveKind::Block;
    block.X = 2.0f;
    block.Y = 3.0f;
    block.SizeX = 1.0f;
    block.SizeY = 1.0f;
    block.R = 0.15f;
    block.G = 0.85f;
    block.B = 1.0f;
    block.A = 0.92f;
    primitives.push_back(block);

    AceAqRenderPrimitive agent{};
    agent.Kind = AceAqRenderPrimitiveKind::Agent;
    agent.X = 4.0f;
    agent.Y = 4.0f;
    agent.SizeX = 0.72f;
    agent.SizeY = 0.72f;
    primitives.push_back(agent);

    AceAqRenderPrimitive arrow{};
    arrow.Kind = AceAqRenderPrimitiveKind::DirectionArrow;
    arrow.X = 4.34f;
    arrow.Y = 4.34f;
    arrow.SizeX = 1.2f;
    arrow.SizeY = -0.4f;
    primitives.push_back(arrow);

    AceAquariumGpuViewportRenderer renderer;
    AceAquariumGpuViewportSnapshot snapshot{};
    if (renderer.render(primitives, 512, 320, false, &snapshot, &error) &&
        snapshot.valid &&
        snapshot.gpuRendered &&
        snapshot.extent.width >= 64 &&
        snapshot.extent.height >= 64 &&
        snapshot.bgraPixels.size() == static_cast<std::size_t>(snapshot.extent.width) * snapshot.extent.height)
    {
        pass("rhi6_aquarium_gpu_viewport_renders_snapshot");
    }
    else
    {
        fail("rhi6_aquarium_gpu_viewport_renders_snapshot", error);
    }

    const auto stats = renderer.stats();
    if (stats.framesRendered >= 1 && stats.lastVertexCount > 0 && stats.readbackFrames >= 1)
    {
        pass("rhi6_aquarium_gpu_viewport_stats_valid");
    }
    else
    {
        fail("rhi6_aquarium_gpu_viewport_stats_valid", "unexpected GPU viewport stats");
    }

    renderer.shutdown();
    return failures == 0 ? 0 : 1;
#endif
}
