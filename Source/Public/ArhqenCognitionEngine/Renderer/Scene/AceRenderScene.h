#pragma once

#include "ArhqenCognitionEngine/Renderer/RHI/AceRhi.h"

#include <array>

namespace am::renderer::scene
{
    using namespace am::renderer::rhi;
    struct Vec3 { float x=0,y=0,z=0; };
    struct Vec4 { float x=0,y=0,z=0,w=1; };
    struct Mat4 { std::array<float,16> m{}; };
    struct Bounds { Vec3 min{}, max{}; bool valid=false; };
    struct Camera { Vec3 position{}, forward{0,0,1}, right{1,0,0}, up{0,1,0}; Mat4 view{}, projection{}, viewProjection{}; float fovY=1.0471975512f, nearPlane=0.05f, farPlane=1000; };
    struct Vertex { Vec3 position{}, normal{}; Vec4 color{}; float u=0,v=0; };
    struct MeshSection { Buffer vertices{}, indices{}; U32 vertexCount=0,indexCount=0; IndexFormat indexFormat=IndexFormat::UInt32; Bounds bounds{}; };
    struct Material { Pipeline pipeline{}; std::vector<Texture> textures; std::vector<Sampler> samplers; Buffer constants{}; };
    struct MeshBatch { Name name{}; MeshSection mesh{}; Material material{}; Mat4 localToWorld{}; Bounds worldBounds{}; bool transparent=false, castsShadow=true, receivesShadow=true; };
    struct View { Name name{Name{"MainView"}}; Camera camera{}; Extent2D extent{}; Texture color{}, depth{}; Format colorFormat=Format::BGRA8, depthFormat=Format::D32F; Color clear{0.005f,0.01f,0.02f,1}; };
    struct Stats { U32 submitted=0, visible=0, culled=0, opaque=0, transparent=0, draws=0; };

    Mat4 Identity();
    Mat4 Perspective(float fovY, float aspect, float znear, float zfar);
    Bounds Merge(Bounds a, Bounds b);
    Bounds Transform(Bounds b, Mat4 m);

    class RenderScene
    {
    public:
        void clear();
        void add(MeshBatch batch);
        const std::vector<MeshBatch>& batches() const { return batches_; }
        U32 count() const { return static_cast<U32>(batches_.size()); }
        Bounds bounds() const;
    private:
        std::vector<MeshBatch> batches_;
    };

    class SceneRenderer
    {
    public:
        struct Build { bool ok=false; std::string error; Stats stats{}; };
        Build buildGraph(const RenderScene& scene, const View& view, RenderGraph& graph);
        std::vector<const MeshBatch*> gather(const RenderScene& scene, const View& view, Stats& stats) const;
        bool recordBasePass(const std::vector<const MeshBatch*>& visible, const View& view, CommandList& cmd) const;
    private:
        bool visible(const MeshBatch& batch, const View& view) const;
    };
}
