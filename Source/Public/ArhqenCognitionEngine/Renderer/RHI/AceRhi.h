#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace am::renderer::rhi
{
    using U8 = std::uint8_t;
    using U32 = std::uint32_t;
    using U64 = std::uint64_t;

    enum class Backend { Null, Dx12, Vulkan, Metal };
    enum class Queue { Graphics, Compute, Copy };
    enum class Format { Unknown, R8, RG8, RGBA8, BGRA8, RGBA16F, R32F, RG32F, RGB32F, RGBA32F, R32U, D24S8, D32F };
    enum class Memory { GpuOnly, Upload, Readback, Transient };
    enum class Dimension { Tex1D, Tex2D, Tex3D, Cube };
    enum class Topology { TriangleList, TriangleStrip, LineList, LineStrip, PointList };
    enum class IndexFormat { None, UInt16, UInt32 };
    enum class LoadOp { Load, Clear, DontCare };
    enum class StoreOp { Store, DontCare };
    enum class Access : U32 { Unknown=0, Common=1, Vertex=2, Index=4, Constant=8, ShaderRead=16, ShaderWrite=32, RenderTarget=64, DepthRead=128, DepthWrite=256, CopyRead=512, CopyWrite=1024, Present=2048, Indirect=4096 };
    enum class Usage : U32 { None=0, Vertex=1, Index=2, Constant=4, Structured=8, Raw=16, ShaderResource=32, UnorderedAccess=64, RenderTarget=128, DepthStencil=256, CopySrc=512, CopyDst=1024, IndirectArgs=2048, Present=4096 };
    enum class ShaderStage : U32 { None=0, Vertex=1, Pixel=2, Compute=4, Geometry=8, Mesh=16, AllGraphics=27, All=31 };

    constexpr Access operator|(Access a, Access b) { return static_cast<Access>(static_cast<U32>(a) | static_cast<U32>(b)); }
    constexpr Usage operator|(Usage a, Usage b) { return static_cast<Usage>(static_cast<U32>(a) | static_cast<U32>(b)); }
    constexpr ShaderStage operator|(ShaderStage a, ShaderStage b) { return static_cast<ShaderStage>(static_cast<U32>(a) | static_cast<U32>(b)); }
    constexpr bool Has(Usage value, Usage flag) { return (static_cast<U32>(value) & static_cast<U32>(flag)) != 0; }
    constexpr bool Has(Access value, Access flag) { return (static_cast<U32>(value) & static_cast<U32>(flag)) != 0; }
    constexpr bool Has(ShaderStage value, ShaderStage flag) { return (static_cast<U32>(value) & static_cast<U32>(flag)) != 0; }

    struct Extent2D { U32 width=1, height=1; };
    struct Extent3D { U32 width=1, height=1, depth=1; };
    struct Rect2D { int x=0, y=0; U32 width=1, height=1; };
    struct Viewport { float x=0, y=0, width=1, height=1, minDepth=0, maxDepth=1; };
    struct Color { float r=0, g=0, b=0, a=1; };
    struct ClearValue { Color color{}; float depth=1; U8 stencil=0; };
    struct Name { std::string value; Name()=default; explicit Name(std::string v):value(std::move(v)){} bool empty()const{return value.empty();} };

    struct Handle { U32 index=0, generation=0; bool valid()const{return index!=0 && generation!=0;} friend bool operator==(Handle a, Handle b){return a.index==b.index&&a.generation==b.generation;} };
    struct Buffer { Handle h{}; bool valid()const{return h.valid();} };
    struct Texture { Handle h{}; bool valid()const{return h.valid();} };
    struct Sampler { Handle h{}; bool valid()const{return h.valid();} };
    struct Shader { Handle h{}; bool valid()const{return h.valid();} };
    struct Pipeline { Handle h{}; bool valid()const{return h.valid();} };

    struct BufferDesc { Name name{}; U64 size=0; U32 stride=0; Usage usage=Usage::None; Memory memory=Memory::GpuOnly; bool persistentMap=false; };
    struct TextureDesc { Name name{}; Dimension dimension=Dimension::Tex2D; Extent3D extent{}; U32 mips=1; U32 layers=1; Format format=Format::Unknown; Usage usage=Usage::ShaderResource; Memory memory=Memory::GpuOnly; ClearValue clear{}; };
    struct SamplerDesc { Name name{}; int minFilter=1, magFilter=1, mipFilter=1; int addressU=2, addressV=2, addressW=2; float maxAnisotropy=1; };
    struct ShaderDesc { Name name{}; ShaderStage stage=ShaderStage::None; std::string entry="main"; std::string profile; std::vector<U8> bytecode; std::string debugSource; };
    struct VertexBinding { U32 binding=0, stride=0; bool perInstance=false; };
    struct VertexAttribute { std::string semantic; Format format=Format::Unknown; U32 location=0, binding=0, offset=0; };
    struct BlendTarget { bool enabled=false; U8 writeMask=0x0F; };
    struct RasterState { int cull=2; int fill=0; bool frontCCW=false; bool depthClip=true; };
    struct DepthState { bool test=true; bool write=true; int compare=3; };
    struct GraphicsPipelineDesc { Name name{}; Shader vs{}, ps{}; std::vector<VertexBinding> bindings; std::vector<VertexAttribute> attributes; Topology topology=Topology::TriangleList; RasterState raster{}; DepthState depth{}; std::vector<BlendTarget> blends; std::vector<Format> colorFormats; Format depthFormat=Format::Unknown; };
    struct ColorAttachment { Texture texture{}; LoadOp load=LoadOp::Clear; StoreOp store=StoreOp::Store; ClearValue clear{}; };
    struct DepthAttachment { Texture texture{}; LoadOp load=LoadOp::Clear; StoreOp store=StoreOp::Store; float clearDepth=1; U8 clearStencil=0; };
    struct RenderPassDesc { Name name{}; std::vector<ColorAttachment> colors; std::optional<DepthAttachment> depth; Extent2D extent{}; };
    struct DeviceDesc { Backend backend=Backend::Null; Name name{Name{"ArhqenRHI"}}; bool validation=true; bool markers=true; U32 framesInFlight=2; };
    struct Stats { U64 createdBuffers=0, destroyedBuffers=0, createdTextures=0, destroyedTextures=0, createdPipelines=0, destroyedPipelines=0, commandLists=0, submissions=0, graphPasses=0, validationErrors=0; };

    const char* ToString(Backend v);
    const char* ToString(Format v);
    bool IsDepth(Format f);
    bool IsColor(Format f);
    U32 BytesPerPixel(Format f);
    bool Validate(const BufferDesc& d, std::string* e=nullptr);
    bool Validate(const TextureDesc& d, std::string* e=nullptr);
    bool Validate(const GraphicsPipelineDesc& d, std::string* e=nullptr);

    class Registry
    {
    public:
        Registry();
        Buffer create(BufferDesc d, std::string* e=nullptr);
        Texture create(TextureDesc d, std::string* e=nullptr);
        Sampler create(SamplerDesc d, std::string* e=nullptr);
        Shader create(ShaderDesc d, std::string* e=nullptr);
        Pipeline create(GraphicsPipelineDesc d, std::string* e=nullptr);
        bool destroy(Buffer h, std::string* e=nullptr);
        bool destroy(Texture h, std::string* e=nullptr);
        bool destroy(Sampler h, std::string* e=nullptr);
        bool destroy(Shader h, std::string* e=nullptr);
        bool destroy(Pipeline h, std::string* e=nullptr);
        const BufferDesc* desc(Buffer h) const;
        const TextureDesc* desc(Texture h) const;
        const GraphicsPipelineDesc* desc(Pipeline h) const;
        bool exists(Buffer h) const { return desc(h)!=nullptr; }
        bool exists(Texture h) const { return desc(h)!=nullptr; }
        bool exists(Pipeline h) const { return desc(h)!=nullptr; }
        bool setAccess(Buffer h, Access a, std::string* e=nullptr);
        bool setAccess(Texture h, Access a, std::string* e=nullptr);
        Access access(Buffer h) const;
        Access access(Texture h) const;
        U32 liveBuffers() const;
        U32 liveTextures() const;
        U32 livePipelines() const;
        Stats stats() const { return stats_; }
        std::vector<std::string> liveResourceList() const;
        void reset();
    private:
        template<class T> struct Slot { T desc{}; Access access=Access::Common; bool alive=false; U32 generation=0; };
        template<class T> Handle alloc(std::vector<Slot<T>>& slots, T desc, Access access);
        template<class T> Slot<T>* get(std::vector<Slot<T>>& slots, Handle h);
        template<class T> const Slot<T>* get(const std::vector<Slot<T>>& slots, Handle h) const;
        template<class T> bool release(std::vector<Slot<T>>& slots, Handle h, std::string* e);
        std::vector<Slot<BufferDesc>> buffers_;
        std::vector<Slot<TextureDesc>> textures_;
        std::vector<Slot<SamplerDesc>> samplers_;
        std::vector<Slot<ShaderDesc>> shaders_;
        std::vector<Slot<GraphicsPipelineDesc>> pipelines_;
        Stats stats_{};
    };

    struct BarrierBuffer { Buffer buffer{}; Access before=Access::Unknown, after=Access::Unknown; };
    struct BarrierTexture { Texture texture{}; Access before=Access::Unknown, after=Access::Unknown; };
    struct CmdBeginPass { RenderPassDesc desc{}; };
    struct CmdEndPass {};
    struct CmdViewport { Viewport viewport{}; };
    struct CmdScissor { Rect2D rect{}; };
    struct CmdPipeline { Pipeline pipeline{}; };
    struct CmdWvpConstants { std::array<float, 16> value{}; };
    struct CmdVertexBuffer { U32 slot=0; Buffer buffer{}; U64 offset=0; };
    struct CmdIndexBuffer { Buffer buffer{}; IndexFormat format=IndexFormat::UInt32; U64 offset=0; };
    struct CmdDraw { U32 vertices=0, instances=1, firstVertex=0, firstInstance=0; };
    struct CmdDrawIndexed { U32 indices=0, instances=1, firstIndex=0; int vertexOffset=0; U32 firstInstance=0; };
    struct CmdDispatch { U32 x=1,y=1,z=1; };
    struct CmdCopyBuffer { Buffer src{}, dst{}; U64 srcOffset=0, dstOffset=0, size=0; };
    struct CmdMarker { std::string label; Color color{}; };
    struct CmdBarrier { std::vector<BarrierBuffer> buffers; std::vector<BarrierTexture> textures; };
    using Command = std::variant<CmdBeginPass,CmdEndPass,CmdViewport,CmdScissor,CmdPipeline,CmdWvpConstants,CmdVertexBuffer,CmdIndexBuffer,CmdDraw,CmdDrawIndexed,CmdDispatch,CmdCopyBuffer,CmdMarker,CmdBarrier>;

    class CommandList
    {
    public:
        explicit CommandList(Queue q=Queue::Graphics);
        void reset(Queue q=Queue::Graphics);
        void marker(std::string label, Color c={});
        void barrier(Buffer b, Access before, Access after);
        void barrier(Texture t, Access before, Access after);
        void begin(RenderPassDesc d);
        void end();
        void viewport(Viewport v);
        void scissor(Rect2D r);
        void pipeline(Pipeline p);
        void wvp(const std::array<float, 16>& value);
        void vertexBuffer(U32 slot, Buffer b, U64 offset=0);
        void indexBuffer(Buffer b, IndexFormat fmt=IndexFormat::UInt32, U64 offset=0);
        void draw(U32 vertices, U32 instances=1, U32 first=0, U32 firstInstance=0);
        void drawIndexed(U32 indices, U32 instances=1, U32 first=0, int vertexOffset=0, U32 firstInstance=0);
        void dispatch(U32 x, U32 y=1, U32 z=1);
        void copy(Buffer src, Buffer dst, U64 srcOffset, U64 dstOffset, U64 size);
        bool validate(const Registry& registry, std::string* e=nullptr) const;
        Queue queue() const { return queue_; }
        const std::vector<Command>& commands() const { return commands_; }
        U32 commandCount() const { return static_cast<U32>(commands_.size()); }
        U32 drawCalls() const { return drawCalls_; }
    private:
        Queue queue_=Queue::Graphics;
        std::vector<Command> commands_;
        U32 drawCalls_=0;
    };

    struct SubmitInfo { Queue queue=Queue::Graphics; const CommandList* list=nullptr; };
    class IDevice
    {
    public:
        virtual ~IDevice()=default;
        virtual bool initialize(DeviceDesc d, std::string* e)=0;
        virtual void shutdown()=0;
        virtual bool beginFrame(U64 frame, float dt, std::string* e)=0;
        virtual bool submit(SubmitInfo info, std::string* e)=0;
        virtual bool endFrame(std::string* e)=0;
        virtual void waitIdle()=0;
        virtual Registry& resources()=0;
        virtual const Registry& resources() const=0;
        virtual Stats stats() const=0;
        virtual bool initialized() const=0;
        virtual Backend backend() const=0;
    };

    class NullDevice final : public IDevice
    {
    public:
        bool initialize(DeviceDesc d, std::string* e) override;
        void shutdown() override;
        bool beginFrame(U64 frame, float dt, std::string* e) override;
        bool submit(SubmitInfo info, std::string* e) override;
        bool endFrame(std::string* e) override;
        void waitIdle() override {}
        Registry& resources() override { return registry_; }
        const Registry& resources() const override { return registry_; }
        Stats stats() const override;
        bool initialized() const override { return initialized_; }
        Backend backend() const override { return Backend::Null; }
    private:
        DeviceDesc desc_{};
        Registry registry_{};
        Stats deviceStats_{};
        bool initialized_=false, insideFrame_=false;
        U64 frame_=0;
    };
    std::unique_ptr<IDevice> CreateNullDevice();

    enum class RgKind { Buffer, Texture };
    struct RgHandle { U32 value=0; bool valid()const{return value!=0;} };
    struct RgUse { RgKind kind=RgKind::Texture; RgHandle resource{}; Access access=Access::ShaderRead; bool write=false; };
    struct RgPass { Name name{}; Queue queue=Queue::Graphics; std::vector<RgUse> uses; std::function<void(CommandList&)> record; };
    struct RgCompiledPass { U32 pass=0; std::vector<BarrierBuffer> buffers; std::vector<BarrierTexture> textures; };
    struct RgCompileResult { bool ok=false; std::string error; std::vector<RgCompiledPass> passes; U32 transientBuffers=0, transientTextures=0; };

    class RenderGraph
    {
    public:
        void reset();
        RgHandle create(BufferDesc d);
        RgHandle create(TextureDesc d);
        RgHandle import(Name name, Buffer b);
        RgHandle import(Name name, Texture t, TextureDesc desc);
        U32 addPass(Name name, Queue q, std::function<void(CommandList&)> record);
        void read(U32 pass, RgHandle h, Access a);
        void write(U32 pass, RgHandle h, Access a);
        RgCompileResult compile(Registry& registry);
        bool execute(IDevice& device, const RgCompileResult& compiled, std::string* e=nullptr);
        const std::vector<RgPass>& passes() const { return passes_; }
    private:
        struct RgBuffer { Name name{}; BufferDesc desc{}; Buffer imported{}, realized{}; bool external=false; };
        struct RgTexture { Name name{}; TextureDesc desc{}; Texture imported{}, realized{}; bool external=false; };
        bool isTexture(RgHandle h) const;
        U32 decode(RgHandle h) const;
        std::vector<RgBuffer> buffers_;
        std::vector<RgTexture> textures_;
        std::vector<RgPass> passes_;
    };
}
