#pragma once

#include "ArhqenCognitionEngine/Renderer/Scene/AceRenderScene.h"

#include <memory>

namespace am::renderer::core
{
    using namespace am::renderer::rhi;
    using namespace am::renderer::scene;

    struct RendererDesc { DeviceDesc rhi{}; Extent2D defaultExtent{1280,720}; Format color=Format::BGRA8, depth=Format::D32F; };
    struct FrameInfo { float dt=0; U64 frame=0; };
    struct FrameStats { am::renderer::scene::Stats scene{}; am::renderer::rhi::Stats rhi{}; U32 graphPasses=0, transientBuffers=0, transientTextures=0; bool submitted=false; };

    class Renderer
    {
    public:
        Renderer();
        ~Renderer();
        bool initialize(RendererDesc d, std::string* e=nullptr);
        void shutdown();
        bool begin(FrameInfo info, std::string* e=nullptr);
        bool render(const RenderScene& scene, const View& view, std::string* e=nullptr);
        bool end(std::string* e=nullptr);
        IDevice* device(){return device_.get();}
        const IDevice* device()const{return device_.get();}
        bool initialized()const{return initialized_;}
        bool insideFrame()const{return inside_;}
        FrameStats lastStats()const{return stats_;}
    private:
        RendererDesc desc_{};
        std::unique_ptr<IDevice> device_;
        SceneRenderer sceneRenderer_{};
        FrameStats stats_{};
        bool initialized_=false, inside_=false;
    };
}
