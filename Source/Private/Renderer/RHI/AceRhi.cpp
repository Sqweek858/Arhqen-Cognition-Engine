#include "ArhqenCognitionEngine/Renderer/RHI/AceRhi.h"

#include <algorithm>
#include <sstream>
#include <unordered_map>

namespace am::renderer::rhi
{
    const char* ToString(Backend v){switch(v){case Backend::Null:return"Null";case Backend::Dx12:return"Dx12";case Backend::Vulkan:return"Vulkan";case Backend::Metal:return"Metal";}return"Unknown";}
    const char* ToString(Format v){switch(v){case Format::Unknown:return"Unknown";case Format::R8:return"R8";case Format::RG8:return"RG8";case Format::RGBA8:return"RGBA8";case Format::BGRA8:return"BGRA8";case Format::RGBA16F:return"RGBA16F";case Format::R32F:return"R32F";case Format::RG32F:return"RG32F";case Format::RGB32F:return"RGB32F";case Format::RGBA32F:return"RGBA32F";case Format::R32U:return"R32U";case Format::D24S8:return"D24S8";case Format::D32F:return"D32F";}return"Unknown";}
    bool IsDepth(Format f){return f==Format::D24S8||f==Format::D32F;}
    bool IsColor(Format f){return f!=Format::Unknown&&!IsDepth(f);}
    U32 BytesPerPixel(Format f){switch(f){case Format::R8:return 1;case Format::RG8:return 2;case Format::RGBA8:case Format::BGRA8:case Format::R32F:case Format::R32U:case Format::D24S8:case Format::D32F:return 4;case Format::RGBA16F:case Format::RG32F:return 8;case Format::RGB32F:return 12;case Format::RGBA32F:return 16;case Format::Unknown:return 0;}return 0;}
    bool Validate(const BufferDesc& d,std::string* e){if(d.size==0){if(e)*e="buffer size must be non-zero";return false;}if(Has(d.usage,Usage::Vertex)&&d.stride==0){if(e)*e="vertex buffer requires stride";return false;}if(d.memory==Memory::Readback&&Has(d.usage,Usage::RenderTarget)){if(e)*e="readback buffer cannot be render target";return false;}return true;}
    bool Validate(const TextureDesc& d,std::string* e){if(d.format==Format::Unknown){if(e)*e="texture format must be known";return false;}if(d.extent.width==0||d.extent.height==0||d.extent.depth==0){if(e)*e="texture extent must be non-zero";return false;}if(d.mips==0||d.layers==0){if(e)*e="texture mips/layers must be non-zero";return false;}if(Has(d.usage,Usage::DepthStencil)&&!IsDepth(d.format)){if(e)*e="depth usage requires depth format";return false;}if(Has(d.usage,Usage::RenderTarget)&&!IsColor(d.format)){if(e)*e="render target usage requires color format";return false;}return true;}
    bool Validate(const GraphicsPipelineDesc& d,std::string* e){if(!d.vs.valid()){if(e)*e="pipeline requires vertex shader";return false;}if(d.colorFormats.empty()&&d.depthFormat==Format::Unknown){if(e)*e="pipeline requires color or depth target";return false;}for(auto f:d.colorFormats){if(!IsColor(f)){if(e)*e="pipeline color format invalid";return false;}}if(d.depthFormat!=Format::Unknown&&!IsDepth(d.depthFormat)){if(e)*e="pipeline depth format invalid";return false;}return true;}

    Registry::Registry(){reset();}
    void Registry::reset(){buffers_.assign(1,{});textures_.assign(1,{});samplers_.assign(1,{});shaders_.assign(1,{});pipelines_.assign(1,{});stats_={};}
    template<class T> Handle Registry::alloc(std::vector<Slot<T>>& s,T d,Access a){U32 idx=0;for(U32 i=1;i<static_cast<U32>(s.size());++i){if(!s[i].alive){idx=i;break;}}if(idx==0){idx=static_cast<U32>(s.size());s.push_back({});}auto& slot=s[idx];slot.desc=std::move(d);slot.access=a;slot.alive=true;slot.generation=slot.generation==0?1:slot.generation+1;if(slot.generation==0)slot.generation=1;return{idx,slot.generation};}
    template<class T> typename Registry::Slot<T>* Registry::get(std::vector<Slot<T>>& s,Handle h){if(!h.valid()||h.index>=s.size())return nullptr;auto& slot=s[h.index];return slot.alive&&slot.generation==h.generation?&slot:nullptr;}
    template<class T> const typename Registry::Slot<T>* Registry::get(const std::vector<Slot<T>>& s,Handle h) const{if(!h.valid()||h.index>=s.size())return nullptr;auto& slot=s[h.index];return slot.alive&&slot.generation==h.generation?&slot:nullptr;}
    template<class T> bool Registry::release(std::vector<Slot<T>>& s,Handle h,std::string* e){auto* slot=get(s,h);if(!slot){if(e)*e="invalid or stale RHI handle";++stats_.validationErrors;return false;}slot->alive=false;return true;}
    Buffer Registry::create(BufferDesc d,std::string* e){if(!Validate(d,e)){++stats_.validationErrors;return{};}++stats_.createdBuffers;return{alloc(buffers_,std::move(d),Access::Common)};}
    Texture Registry::create(TextureDesc d,std::string* e){if(!Validate(d,e)){++stats_.validationErrors;return{};}++stats_.createdTextures;Access a=Has(d.usage,Usage::Present)?Access::Present:Access::Common;return{alloc(textures_,std::move(d),a)};}
    Sampler Registry::create(SamplerDesc d,std::string*){return{alloc(samplers_,std::move(d),Access::Common)};}
    Shader Registry::create(ShaderDesc d,std::string* e){if(d.stage==ShaderStage::None){if(e)*e="shader stage required";++stats_.validationErrors;return{};}return{alloc(shaders_,std::move(d),Access::Common)};}
    Pipeline Registry::create(GraphicsPipelineDesc d,std::string* e){if(!Validate(d,e)){++stats_.validationErrors;return{};}const auto*vs=desc(d.vs);if(!vs||vs->stage!=ShaderStage::Vertex){if(e)*e="pipeline vertex shader handle/stage invalid";++stats_.validationErrors;return{};}if(d.ps.valid()){const auto*ps=desc(d.ps);if(!ps||ps->stage!=ShaderStage::Pixel){if(e)*e="pipeline pixel shader handle/stage invalid";++stats_.validationErrors;return{};}}++stats_.createdPipelines;return{alloc(pipelines_,std::move(d),Access::Common)};}
    bool Registry::destroy(Buffer h,std::string* e){if(release(buffers_,h.h,e)){++stats_.destroyedBuffers;return true;}return false;}
    bool Registry::destroy(Texture h,std::string* e){if(release(textures_,h.h,e)){++stats_.destroyedTextures;return true;}return false;}
    bool Registry::destroy(Sampler h,std::string* e){return release(samplers_,h.h,e);}
    bool Registry::destroy(Shader h,std::string* e){return release(shaders_,h.h,e);}
    bool Registry::destroy(Pipeline h,std::string* e){if(release(pipelines_,h.h,e)){++stats_.destroyedPipelines;return true;}return false;}
    const BufferDesc* Registry::desc(Buffer h)const{auto* s=get(buffers_,h.h);return s?&s->desc:nullptr;}
    const TextureDesc* Registry::desc(Texture h)const{auto* s=get(textures_,h.h);return s?&s->desc:nullptr;}
    const SamplerDesc* Registry::desc(Sampler h)const{auto* s=get(samplers_,h.h);return s?&s->desc:nullptr;}
    const ShaderDesc* Registry::desc(Shader h)const{auto* s=get(shaders_,h.h);return s?&s->desc:nullptr;}
    const GraphicsPipelineDesc* Registry::desc(Pipeline h)const{auto* s=get(pipelines_,h.h);return s?&s->desc:nullptr;}
    bool Registry::setAccess(Buffer h,Access a,std::string* e){auto* s=get(buffers_,h.h);if(!s){if(e)*e="invalid buffer";++stats_.validationErrors;return false;}s->access=a;return true;}
    bool Registry::setAccess(Texture h,Access a,std::string* e){auto* s=get(textures_,h.h);if(!s){if(e)*e="invalid texture";++stats_.validationErrors;return false;}s->access=a;return true;}
    Access Registry::access(Buffer h)const{auto* s=get(buffers_,h.h);return s?s->access:Access::Unknown;}
    Access Registry::access(Texture h)const{auto* s=get(textures_,h.h);return s?s->access:Access::Unknown;}
    U32 Registry::liveBuffers()const{U32 c=0;for(auto&s:buffers_)if(s.alive)++c;return c;}
    U32 Registry::liveTextures()const{U32 c=0;for(auto&s:textures_)if(s.alive)++c;return c;}
    U32 Registry::livePipelines()const{U32 c=0;for(auto&s:pipelines_)if(s.alive)++c;return c;}
    std::vector<std::string> Registry::liveResourceList()const{std::vector<std::string> out;auto add=[&](const char*k,const auto&v){for(size_t i=1;i<v.size();++i)if(v[i].alive){std::ostringstream ss;ss<<k<<"#"<<i<<"|"<<(v[i].desc.name.empty()?"<unnamed>":v[i].desc.name.value);out.push_back(ss.str());}};add("buffer",buffers_);add("texture",textures_);add("pipeline",pipelines_);return out;}

    CommandList::CommandList(Queue q):queue_(q){}
    void CommandList::reset(Queue q){queue_=q;commands_.clear();drawCalls_=0;}
    void CommandList::marker(std::string label,Color c){commands_.push_back(CmdMarker{std::move(label),c});}
    void CommandList::barrier(Buffer b,Access before,Access after){CmdBarrier c;c.buffers.push_back({b,before,after});commands_.push_back(std::move(c));}
    void CommandList::barrier(Texture t,Access before,Access after){CmdBarrier c;c.textures.push_back({t,before,after});commands_.push_back(std::move(c));}
    void CommandList::begin(RenderPassDesc d){commands_.push_back(CmdBeginPass{std::move(d)});}
    void CommandList::end(){commands_.push_back(CmdEndPass{});}
    void CommandList::viewport(Viewport v){commands_.push_back(CmdViewport{v});}
    void CommandList::scissor(Rect2D r){commands_.push_back(CmdScissor{r});}
    void CommandList::pipeline(Pipeline p){commands_.push_back(CmdPipeline{p});}
    void CommandList::wvp(const std::array<float, 16>& value){commands_.push_back(CmdWvpConstants{value});}
    void CommandList::vertexBuffer(U32 slot,Buffer b,U64 o){commands_.push_back(CmdVertexBuffer{slot,b,o});}
    void CommandList::indexBuffer(Buffer b,IndexFormat f,U64 o){commands_.push_back(CmdIndexBuffer{b,f,o});}
    void CommandList::draw(U32 v,U32 i,U32 f,U32 fi){++drawCalls_;commands_.push_back(CmdDraw{v,i,f,fi});}
    void CommandList::drawIndexed(U32 i,U32 inst,U32 first,int vo,U32 fi){++drawCalls_;commands_.push_back(CmdDrawIndexed{i,inst,first,vo,fi});}
    void CommandList::dispatch(U32 x,U32 y,U32 z){commands_.push_back(CmdDispatch{x,y,z});}
    void CommandList::copy(Buffer s,Buffer d,U64 so,U64 doff,U64 sz){commands_.push_back(CmdCopyBuffer{s,d,so,doff,sz});}
    bool CommandList::validate(const Registry& r,std::string* e)const{bool inPass=false,pipe=false;for(auto&c:commands_){if(auto*p=std::get_if<CmdBeginPass>(&c)){if(queue_!=Queue::Graphics){if(e)*e="render pass requires graphics queue";return false;}if(inPass){if(e)*e="nested pass";return false;}if(p->desc.colors.empty()&&!p->desc.depth){if(e)*e="render pass missing attachments";return false;}for(auto&a:p->desc.colors)if(!r.exists(a.texture)){if(e)*e="invalid color attachment";return false;}if(p->desc.depth&&!r.exists(p->desc.depth->texture)){if(e)*e="invalid depth attachment";return false;}inPass=true;pipe=false;continue;}if(std::holds_alternative<CmdEndPass>(c)){if(!inPass){if(e)*e="end without begin";return false;}inPass=false;continue;}if(auto*p=std::get_if<CmdPipeline>(&c)){if(!r.exists(p->pipeline)){if(e)*e="invalid pipeline";return false;}pipe=true;continue;}if(auto*p=std::get_if<CmdVertexBuffer>(&c)){if(!r.exists(p->buffer)){if(e)*e="invalid vertex buffer";return false;}continue;}if(auto*p=std::get_if<CmdIndexBuffer>(&c)){if(!r.exists(p->buffer)){if(e)*e="invalid index buffer";return false;}continue;}if(auto*p=std::get_if<CmdDraw>(&c)){if(!inPass||!pipe||p->vertices==0){if(e)*e="invalid draw";return false;}continue;}if(auto*p=std::get_if<CmdDrawIndexed>(&c)){if(!inPass||!pipe||p->indices==0){if(e)*e="invalid draw indexed";return false;}continue;}if(std::holds_alternative<CmdDispatch>(c)){if(e)*e="compute dispatch is unsupported by this RHI backend";return false;}if(auto*p=std::get_if<CmdCopyBuffer>(&c)){if(!r.exists(p->src)||!r.exists(p->dst)||p->size==0){if(e)*e="invalid copy buffer";return false;}continue;}}if(inPass){if(e)*e="render pass left open";return false;}return true;}

    bool NullDevice::initialize(DeviceDesc d,std::string* e){if(initialized_){if(e)*e="device already initialized";return false;}if(d.framesInFlight==0){if(e)*e="framesInFlight zero";return false;}desc_=std::move(d);registry_.reset();deviceStats_={};initialized_=true;insideFrame_=false;return true;}
    void NullDevice::shutdown(){registry_.reset();initialized_=false;insideFrame_=false;deviceStats_={};}
    bool NullDevice::beginFrame(U64 f,float,std::string* e){if(!initialized_){if(e)*e="begin before init";return false;}if(insideFrame_){if(e)*e="frame already active";return false;}insideFrame_=true;frame_=f;return true;}
    bool NullDevice::submit(SubmitInfo info,std::string* e){if(!initialized_||!insideFrame_){if(e)*e="submit outside frame";return false;}if(!info.list){if(e)*e="missing command list";return false;}if(info.queue!=info.list->queue()){if(e)*e="queue mismatch";return false;}if(!info.list->validate(registry_,e)){++deviceStats_.validationErrors;return false;}++deviceStats_.submissions;++deviceStats_.commandLists;return true;}
    bool NullDevice::endFrame(std::string* e){if(!insideFrame_){if(e)*e="end without frame";return false;}insideFrame_=false;return true;}
    Stats NullDevice::stats()const{auto s=registry_.stats();s.submissions+=deviceStats_.submissions;s.commandLists+=deviceStats_.commandLists;s.graphPasses+=deviceStats_.graphPasses;s.validationErrors+=deviceStats_.validationErrors;return s;}
    std::unique_ptr<IDevice> CreateNullDevice(){return std::make_unique<NullDevice>();}

    namespace { constexpr U32 kTexBit=0x80000000u; bool isTex(RgHandle h){return(h.value&kTexBit)!=0;} U32 idx(RgHandle h){return h.value&~kTexBit;} RgHandle bh(U32 i){return{i};} RgHandle th(U32 i){return{kTexBit|i};} }
    void RenderGraph::reset(){buffers_.assign(1,{});textures_.assign(1,{});passes_.clear();}
    RgHandle RenderGraph::create(BufferDesc d){buffers_.push_back({d.name,std::move(d),{}, {},false});return bh(static_cast<U32>(buffers_.size()-1));}
    RgHandle RenderGraph::create(TextureDesc d){textures_.push_back({d.name,std::move(d),{}, {},false});return th(static_cast<U32>(textures_.size()-1));}
    RgHandle RenderGraph::import(Name n,Buffer b){buffers_.push_back({std::move(n),{},b,b,true});return bh(static_cast<U32>(buffers_.size()-1));}
    RgHandle RenderGraph::import(Name n,Texture t,TextureDesc d){textures_.push_back({std::move(n),std::move(d),t,t,true});return th(static_cast<U32>(textures_.size()-1));}
    U32 RenderGraph::addPass(Name n,Queue q,std::function<void(CommandList&)> rec){passes_.push_back({std::move(n),q,{},std::move(rec)});return static_cast<U32>(passes_.size()-1);}
    void RenderGraph::read(U32 p,RgHandle h,Access a){if(p<passes_.size())passes_[p].uses.push_back({isTex(h)?RgKind::Texture:RgKind::Buffer,h,a,false});}
    void RenderGraph::write(U32 p,RgHandle h,Access a){if(p<passes_.size())passes_[p].uses.push_back({isTex(h)?RgKind::Texture:RgKind::Buffer,h,a,true});}
    bool RenderGraph::isTexture(RgHandle h)const{return isTex(h)&&idx(h)>0&&idx(h)<textures_.size();}
    U32 RenderGraph::decode(RgHandle h)const{return idx(h);}
    RgCompileResult RenderGraph::compile(Registry& r)
    {
        RgCompileResult out;
        auto rollback = [&]()
        {
            for(std::size_t i=1;i<buffers_.size();++i){auto& b=buffers_[i];if(!b.external&&b.realized.valid()){r.destroy(b.realized,nullptr);b.realized={};}}
            for(std::size_t i=1;i<textures_.size();++i){auto& t=textures_[i];if(!t.external&&t.realized.valid()){r.destroy(t.realized,nullptr);t.realized={};}}
            out.transientBuffers=0;
            out.transientTextures=0;
        };
        auto fail = [&](std::string error)
        {
            out.error=std::move(error);
            rollback();
            return out;
        };

        for(auto& b:buffers_)
        {
            if(b.external){if(!r.exists(b.imported))return fail("invalid imported buffer");b.realized=b.imported;}
            else if(b.desc.size){std::string error;b.realized=r.create(b.desc,&error);if(!b.realized.valid())return fail(error);++out.transientBuffers;}
        }
        for(auto& t:textures_)
        {
            if(t.external){if(!r.exists(t.imported))return fail("invalid imported texture");t.realized=t.imported;}
            else if(t.desc.format!=Format::Unknown){std::string error;t.realized=r.create(t.desc,&error);if(!t.realized.valid())return fail(error);++out.transientTextures;}
        }

        std::unordered_map<U32,Access> bufferAccess,textureAccess;
        for(U32 passIndex=0;passIndex<passes_.size();++passIndex)
        {
            RgCompiledPass compiledPass;
            compiledPass.pass=passIndex;
            for(const auto& use:passes_[passIndex].uses)
            {
                const U32 resourceIndex=decode(use.resource);
                if(use.kind==RgKind::Texture)
                {
                    if(!isTexture(use.resource)||!r.exists(textures_[resourceIndex].realized))return fail("invalid graph texture");
                    const Access before=textureAccess.count(resourceIndex)?textureAccess[resourceIndex]:Access::Common;
                    if(before!=use.access)compiledPass.textures.push_back({textures_[resourceIndex].realized,before,use.access});
                    textureAccess[resourceIndex]=use.access;
                }
                else
                {
                    if(resourceIndex==0||resourceIndex>=buffers_.size()||!r.exists(buffers_[resourceIndex].realized))return fail("invalid graph buffer");
                    const Access before=bufferAccess.count(resourceIndex)?bufferAccess[resourceIndex]:Access::Common;
                    if(before!=use.access)compiledPass.buffers.push_back({buffers_[resourceIndex].realized,before,use.access});
                    bufferAccess[resourceIndex]=use.access;
                }
            }
            out.passes.push_back(std::move(compiledPass));
        }
        out.ok=true;
        return out;
    }

    bool RenderGraph::execute(IDevice& d,const RgCompileResult& c,std::string* e)
    {
        if(!c.ok){if(e)*e=c.error;return false;}
        bool ok=true;
        for(const auto& compiledPass:c.passes)
        {
            if(compiledPass.pass>=passes_.size()){if(e)*e="compiled pass out of range";ok=false;break;}
            auto& pass=passes_[compiledPass.pass];
            CommandList list(pass.queue);
            for(const auto& barrier:compiledPass.buffers)list.barrier(barrier.buffer,barrier.before,barrier.after);
            for(const auto& barrier:compiledPass.textures)list.barrier(barrier.texture,barrier.before,barrier.after);
            list.marker(pass.name.empty()?"pass":pass.name.value,{0.2f,0.7f,1,1});
            if(pass.record)pass.record(list);
            if(!d.submit({pass.queue,&list},e)){ok=false;break;}
        }
        for(std::size_t i=1;i<buffers_.size();++i){auto& b=buffers_[i];if(!b.external&&b.realized.valid()){d.destroy(b.realized,nullptr);b.realized={};}}
        for(std::size_t i=1;i<textures_.size();++i){auto& t=textures_[i];if(!t.external&&t.realized.valid()){d.destroy(t.realized,nullptr);t.realized={};}}
        return ok;
    }
}
