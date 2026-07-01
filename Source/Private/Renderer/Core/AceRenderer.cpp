#include "ArhqenCognitionEngine/Renderer/Core/AceRenderer.h"
#include "ArhqenCognitionEngine/Renderer/RHI/AceDx12Rhi.h"

namespace am::renderer::core
{
    Renderer::Renderer()=default;
    Renderer::~Renderer(){shutdown();}
    bool Renderer::initialize(RendererDesc d,std::string* e){if(initialized_){if(e)*e="renderer already initialized";return false;}desc_=std::move(d);device_ = desc_.rhi.backend == Backend::Dx12 ? CreateDx12Device() : CreateNullDevice();if(!device_){if(e)*e="failed to create RHI device";return false;}if(!device_->initialize(desc_.rhi,e)){device_.reset();return false;}initialized_=true;inside_=false;stats_={};return true;}
    void Renderer::shutdown(){if(device_){device_->shutdown();device_.reset();}initialized_=false;inside_=false;stats_={};}
    bool Renderer::begin(FrameInfo i,std::string* e){if(!initialized_||!device_){if(e)*e="begin before renderer initialize";return false;}if(inside_){if(e)*e="frame already active";return false;}if(!device_->beginFrame(i.frame,i.dt,e))return false;inside_=true;stats_={};return true;}
    bool Renderer::render(const RenderScene& s,const View& v,std::string* e){if(!inside_||!device_){if(e)*e="render outside frame";return false;}RenderGraph g;g.reset();auto build=sceneRenderer_.buildGraph(s,v,g);if(!build.ok){if(e)*e=build.error.empty()?"scene graph build failed":build.error;return false;}auto compiled=g.compile(device_->resources());if(!compiled.ok){if(e)*e=compiled.error.empty()?"render graph compile failed":compiled.error;return false;}if(!g.execute(*device_,compiled,e))return false;stats_.scene=build.stats;stats_.graphPasses=static_cast<U32>(compiled.passes.size());stats_.transientBuffers=compiled.transientBuffers;stats_.transientTextures=compiled.transientTextures;stats_.submitted=true;stats_.rhi=device_->stats();return true;}
    bool Renderer::end(std::string* e){if(!inside_||!device_){if(e)*e="end without frame";return false;}if(!device_->endFrame(e))return false;inside_=false;stats_.rhi=device_->stats();return true;}
}
