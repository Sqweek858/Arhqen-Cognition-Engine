#include "ArhqenCognitionEngine/Renderer/Core/AceRenderer.h"
#include "ArhqenCognitionEngine/Renderer/RHI/AceDx12Rhi.h"

#include <iostream>
#include <string>

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
        if (!detail.empty())
        {
            std::cout << "|" << detail;
        }
        std::cout << "\n";
    }
}

int main()
{
    using namespace am::renderer::rhi;
    using namespace am::renderer::scene;
    using namespace am::renderer::core;

#if !defined(_WIN32)
    pass("rhi3_rhi4_probe_skipped_non_windows");
    return 0;
#else
    if (!Dx12RuntimeAvailable())
    {
        fail("dx12_runtime_available", "No hardware D3D12 adapter found.");
        return 1;
    }
    pass("dx12_runtime_available");

    std::string error;
    auto device = CreateDx12DeviceConcrete();
    DeviceDesc desc{};
    desc.backend = Backend::Dx12;
    desc.name = Name{"AceRhi4Dx12OffscreenSceneProbe"};

    if (!device->initialize(desc, &error))
    {
        fail("dx12_device_initializes", error);
        return 1;
    }
    pass("dx12_device_initializes");

    Texture color{};
    Texture depth{};
    if (device->createOffscreenSceneTargets({640, 360}, &color, &depth, &error) && color.valid() && depth.valid())
    {
        pass("rhi4_offscreen_scene_targets_created");
    }
    else
    {
        fail("rhi4_offscreen_scene_targets_created", error);
    }

    BufferDesc vbDesc{};
    vbDesc.name = Name{"rhi3_triangle_vb"};
    vbDesc.size = 21 * sizeof(float);
    vbDesc.stride = 7 * sizeof(float);
    vbDesc.usage = Usage::Vertex | Usage::CopyDst;
    vbDesc.memory = Memory::GpuOnly;
    auto vb = device->resources().create(vbDesc, &error);

    const float verts[21] = {
         0.0f,  0.70f, 0.0f, 1.0f, 0.15f, 0.10f, 1.0f,
         0.70f, -0.70f, 0.0f, 0.10f, 0.85f, 1.0f, 1.0f,
        -0.70f, -0.70f, 0.0f, 1.0f, 0.90f, 0.20f, 1.0f
    };

    if (vb.valid() && device->upload(vb, verts, sizeof(verts), &error))
    {
        pass("rhi3_vertex_upload_to_gpu_buffer");
    }
    else
    {
        fail("rhi3_vertex_upload_to_gpu_buffer", error);
    }

    ShaderDesc vs{};
    vs.name = Name{"rhi3_basic_vs"};
    vs.stage = ShaderStage::Vertex;
    auto vsh = device->resources().create(vs, &error);

    GraphicsPipelineDesc pso{};
    pso.name = Name{"rhi3_basic_color_pso"};
    pso.vs = vsh;
    pso.topology = Topology::TriangleList;
    pso.colorFormats.push_back(Format::BGRA8);
    pso.depthFormat = Format::D32F;
    auto pipeline = device->resources().create(pso, &error);

    if (pipeline.valid())
    {
        pass("rhi3_pipeline_handle_created");
    }
    else
    {
        fail("rhi3_pipeline_handle_created", error);
    }

    CommandList list;
    RenderPassDesc rp{};
    rp.name = Name{"rhi3_draw_pass"};
    rp.extent = {640, 360};
    rp.colors.push_back({color, LoadOp::Clear, StoreOp::Store, {{0.015f, 0.020f, 0.035f, 1.0f}, 1.0f, 0}});
    rp.depth = DepthAttachment{depth, LoadOp::Clear, StoreOp::Store, 1.0f, 0};
    list.begin(rp);
    list.viewport({0.0f, 0.0f, 640.0f, 360.0f, 0.0f, 1.0f});
    list.scissor({0, 0, 640, 360});
    list.pipeline(pipeline);
    list.vertexBuffer(0, vb, 0);
    list.draw(3);
    list.end();

    if (device->beginFrame(1, 1.0f / 60.0f, &error) &&
        device->submit({Queue::Graphics, &list}, &error) &&
        device->endFrame(&error))
    {
        pass("rhi3_dx12_draw_instanced_executes");
    }
    else
    {
        fail("rhi3_dx12_draw_instanced_executes", error);
    }

    auto gpuStats = device->gpuStats();
    if (gpuStats.compiledShaders >= 2 && gpuStats.nativePipelines >= 1 && gpuStats.drawCallsExecuted >= 1)
    {
        pass("rhi3_gpu_stats_confirm_shader_pso_draw");
    }
    else
    {
        fail("rhi3_gpu_stats_confirm_shader_pso_draw", "shader/pso/draw stats did not increment");
    }

    device->shutdown();

    Renderer renderer;
    RendererDesc rendererDesc{};
    rendererDesc.rhi.backend = Backend::Dx12;
    rendererDesc.rhi.name = Name{"AceRhi4RendererFacadeProbe"};

    if (!renderer.initialize(rendererDesc, &error))
    {
        fail("rhi4_renderer_initializes_dx12", error);
        return failures == 0 ? 0 : 1;
    }
    pass("rhi4_renderer_initializes_dx12");

    auto* dx = dynamic_cast<Dx12Device*>(renderer.device());
    if (!dx)
    {
        fail("rhi4_renderer_exposes_dx12_device");
        return failures == 0 ? 0 : 1;
    }
    pass("rhi4_renderer_exposes_dx12_device");

    Texture sceneColor{};
    Texture sceneDepth{};
    if (!dx->createOffscreenSceneTargets({320, 240}, &sceneColor, &sceneDepth, &error))
    {
        fail("rhi4_renderer_offscreen_targets", error);
        return failures == 0 ? 0 : 1;
    }
    pass("rhi4_renderer_offscreen_targets");

    BufferDesc sceneVbDesc{};
    sceneVbDesc.name = Name{"rhi4_scene_triangle_vb"};
    sceneVbDesc.size = sizeof(verts);
    sceneVbDesc.stride = 7 * sizeof(float);
    sceneVbDesc.usage = Usage::Vertex | Usage::CopyDst;
    sceneVbDesc.memory = Memory::GpuOnly;
    auto sceneVb = dx->resources().create(sceneVbDesc, &error);
    if (!sceneVb.valid() || !dx->upload(sceneVb, verts, sizeof(verts), &error))
    {
        fail("rhi4_scene_vertex_upload", error);
        return failures == 0 ? 0 : 1;
    }
    pass("rhi4_scene_vertex_upload");

    ShaderDesc sceneVs{};
    sceneVs.name = Name{"rhi4_scene_vs"};
    sceneVs.stage = ShaderStage::Vertex;
    auto sceneVsh = dx->resources().create(sceneVs, &error);

    GraphicsPipelineDesc scenePso{};
    scenePso.name = Name{"rhi4_scene_basic_color_pso"};
    scenePso.vs = sceneVsh;
    scenePso.topology = Topology::TriangleList;
    scenePso.colorFormats.push_back(Format::BGRA8);
    scenePso.depthFormat = Format::D32F;
    auto scenePipeline = dx->resources().create(scenePso, &error);

    RenderScene scene;
    MeshBatch batch{};
    batch.name = Name{"rhi4_triangle_batch"};
    batch.mesh.vertices = sceneVb;
    batch.mesh.vertexCount = 3;
    batch.mesh.bounds.valid = true;
    batch.mesh.bounds.min = {-1.0f, -1.0f, -1.0f};
    batch.mesh.bounds.max = {1.0f, 1.0f, 1.0f};
    batch.material.pipeline = scenePipeline;
    batch.localToWorld = Identity();
    scene.add(batch);

    View view{};
    view.name = Name{"rhi4_offscreen_view"};
    view.extent = {320, 240};
    view.color = sceneColor;
    view.depth = sceneDepth;
    view.colorFormat = Format::BGRA8;
    view.depthFormat = Format::D32F;
    view.camera.position = {0.0f, 0.0f, -5.0f};
    view.camera.farPlane = 500.0f;

    if (renderer.begin({1.0f / 60.0f, 2}, &error) &&
        renderer.render(scene, view, &error) &&
        renderer.end(&error))
    {
        pass("rhi4_scene_renderer_draws_to_offscreen_targets");
    }
    else
    {
        fail("rhi4_scene_renderer_draws_to_offscreen_targets", error);
    }

    const auto frameStats = renderer.lastStats();
    if (frameStats.scene.submitted == 1 && frameStats.scene.draws == 1 && frameStats.graphPasses == 1)
    {
        pass("rhi4_scene_renderer_stats_valid");
    }
    else
    {
        fail("rhi4_scene_renderer_stats_valid", "unexpected scene/render-graph stats");
    }

    const auto rendererGpuStats = dx->gpuStats();
    if (rendererGpuStats.drawCallsExecuted >= 1 && rendererGpuStats.offscreenSceneTargets >= 1)
    {
        pass("rhi4_gpu_stats_confirm_offscreen_scene_draw");
    }
    else
    {
        fail("rhi4_gpu_stats_confirm_offscreen_scene_draw", "GPU scene draw stats did not increment");
    }

    renderer.shutdown();
    return failures == 0 ? 0 : 1;
#endif
}
