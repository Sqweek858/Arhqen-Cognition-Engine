#include "ArhqenCognitionEngine/Renderer/Core/AceRenderer.h"

#include <iostream>
#include <string>

namespace
{
    int failures = 0;
    void pass(const std::string& n){ std::cout << "PASS|" << n << "\n"; }
    void fail(const std::string& n, const std::string& d={}){ ++failures; std::cout << "FAIL|" << n; if(!d.empty()) std::cout << "|" << d; std::cout << "\n"; }
}

int main()
{
    using namespace am::renderer::rhi;
    using namespace am::renderer::scene;
    using namespace am::renderer::core;

    std::string error;
    Registry reg;

    BufferDesc vbDesc; vbDesc.name=Name{"probe_vb"}; vbDesc.size=1024; vbDesc.stride=sizeof(Vertex); vbDesc.usage=Usage::Vertex|Usage::CopyDst;
    auto vb=reg.create(vbDesc,&error);
    if(vb.valid() && reg.liveBuffers()==1) pass("rhi_buffer_create_valid"); else fail("rhi_buffer_create_valid", error);

    BufferDesc bad; bad.name=Name{"bad"}; bad.size=0; bad.usage=Usage::Vertex;
    error.clear();
    auto badBuf=reg.create(bad,&error);
    if(!badBuf.valid() && !error.empty()) pass("rhi_rejects_invalid_buffer"); else fail("rhi_rejects_invalid_buffer");

    TextureDesc colorDesc; colorDesc.name=Name{"color"}; colorDesc.format=Format::BGRA8; colorDesc.extent={1280,720,1}; colorDesc.usage=Usage::RenderTarget|Usage::ShaderResource;
    TextureDesc depthDesc; depthDesc.name=Name{"depth"}; depthDesc.format=Format::D32F; depthDesc.extent={1280,720,1}; depthDesc.usage=Usage::DepthStencil|Usage::ShaderResource;
    auto color=reg.create(colorDesc,&error);
    auto depth=reg.create(depthDesc,&error);
    if(color.valid() && depth.valid() && reg.liveTextures()==2) pass("rhi_texture_create_color_depth"); else fail("rhi_texture_create_color_depth", error);

    ShaderDesc vs; vs.name=Name{"vs"}; vs.stage=ShaderStage::Vertex;
    ShaderDesc ps; ps.name=Name{"ps"}; ps.stage=ShaderStage::Pixel;
    auto vsh=reg.create(vs,&error); auto psh=reg.create(ps,&error);
    GraphicsPipelineDesc pd; pd.name=Name{"pipe"}; pd.vs=vsh; pd.ps=psh; pd.colorFormats.push_back(Format::BGRA8); pd.depthFormat=Format::D32F;
    auto pipe=reg.create(pd,&error);
    if(pipe.valid()) pass("rhi_pipeline_create_valid"); else fail("rhi_pipeline_create_valid", error);

    CommandList list;
    RenderPassDesc rp; rp.name=Name{"pass"}; rp.extent={1280,720}; rp.colors.push_back({color,LoadOp::Clear,StoreOp::Store,{}}); rp.depth=DepthAttachment{depth,LoadOp::Clear,StoreOp::Store,1,0};
    list.begin(rp); list.viewport({0,0,1280,720,0,1}); list.scissor({0,0,1280,720}); list.pipeline(pipe); list.vertexBuffer(0,vb); list.draw(3); list.end();
    error.clear();
    if(list.validate(reg,&error) && list.drawCalls()==1) pass("rhi_command_list_validates_draw"); else fail("rhi_command_list_validates_draw", error);

    RenderGraph graph; graph.reset();
    auto rgColor=graph.import(Name{"rg_color"}, color, colorDesc);
    auto rgDepth=graph.import(Name{"rg_depth"}, depth, depthDesc);
    auto passIndex=graph.addPass(Name{"base"}, Queue::Graphics, [&](CommandList& cmd){ cmd.begin(rp); cmd.pipeline(pipe); cmd.vertexBuffer(0,vb); cmd.draw(3); cmd.end(); });
    graph.write(passIndex, rgColor, Access::RenderTarget);
    graph.write(passIndex, rgDepth, Access::DepthWrite);
    auto compiled=graph.compile(reg);
    if(compiled.ok && compiled.passes.size()==1) pass("render_graph_compiles_passes"); else fail("render_graph_compiles_passes", compiled.error);

    auto device=CreateNullDevice();
    DeviceDesc dd; dd.backend=Backend::Null;
    if(!device->initialize(dd,&error))
    {
        fail("null_rhi_executes_graph", error);
    }
    else
    {
        auto& dr = device->resources();
        auto dcolor = dr.create(colorDesc, &error);
        auto ddepth = dr.create(depthDesc, &error);
        auto dvb = dr.create(vbDesc, &error);
        auto dvsh = dr.create(vs, &error);
        auto dpsh = dr.create(ps, &error);
        GraphicsPipelineDesc dpd = pd; dpd.vs = dvsh; dpd.ps = dpsh;
        auto dpipe = dr.create(dpd, &error);

        RenderGraph dgraph; dgraph.reset();
        auto drgColor = dgraph.import(Name{"device_rg_color"}, dcolor, colorDesc);
        auto drgDepth = dgraph.import(Name{"device_rg_depth"}, ddepth, depthDesc);
        auto dpass = dgraph.addPass(Name{"device_base"}, Queue::Graphics, [&](CommandList& cmd)
        {
            RenderPassDesc localPass; localPass.name=Name{"device_pass"}; localPass.extent={1280,720};
            localPass.colors.push_back({dcolor,LoadOp::Clear,StoreOp::Store,{}});
            localPass.depth=DepthAttachment{ddepth,LoadOp::Clear,StoreOp::Store,1,0};
            cmd.begin(localPass); cmd.pipeline(dpipe); cmd.vertexBuffer(0,dvb); cmd.draw(3); cmd.end();
        });
        dgraph.write(dpass, drgColor, Access::RenderTarget);
        dgraph.write(dpass, drgDepth, Access::DepthWrite);
        auto dcompiled = dgraph.compile(dr);
        if(device->beginFrame(1,1.0f/60.0f,&error) && dgraph.execute(*device,dcompiled,&error) && device->endFrame(&error))
            pass("null_rhi_executes_graph");
        else
            fail("null_rhi_executes_graph", error);
    }

    Renderer renderer;
    RendererDesc rd; rd.rhi.backend=Backend::Null;
    if(!renderer.initialize(rd,&error)){ fail("renderer_initialize", error); return failures?1:0; }

    auto& rr = renderer.device()->resources();
    auto rvb = rr.create(vbDesc, &error);
    auto rcolor = rr.create(colorDesc, &error);
    auto rdepth = rr.create(depthDesc, &error);
    auto rvsh = rr.create(vs, &error);
    auto rpsh = rr.create(ps, &error);
    GraphicsPipelineDesc rpd = pd; rpd.vs = rvsh; rpd.ps = rpsh;
    auto rpipe = rr.create(rpd, &error);

    RenderScene scene;
    MeshBatch batch; batch.name=Name{"batch"}; batch.mesh.vertices=rvb; batch.mesh.vertexCount=3; batch.mesh.bounds={{-1,-1,-1},{1,1,1},true}; batch.material.pipeline=rpipe; batch.localToWorld=Identity();
    scene.add(batch);
    View view; view.extent={1280,720}; view.color=rcolor; view.depth=rdepth; view.camera.position={0,0,-5}; view.camera.farPlane=500;
    if(renderer.begin({1.0f/60.0f,2},&error) && renderer.render(scene,view,&error) && renderer.end(&error))
    {
        auto stats=renderer.lastStats();
        if(stats.scene.submitted==1 && stats.graphPasses==1) pass("renderer_facade_builds_scene_graph"); else fail("renderer_facade_builds_scene_graph","bad stats");
    }
    else fail("renderer_facade_builds_scene_graph", error);

    if(reg.destroy(vb,&error) && reg.liveBuffers()==0) pass("rhi_destroy_releases_buffer"); else fail("rhi_destroy_releases_buffer", error);

    return failures == 0 ? 0 : 1;
}
