#include "ArhqenCognitionEngine/Renderer/RHI/AceDx12Rhi.h"

#if defined(_WIN32)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <d3d12.h>
#include <d2d1.h>
#include <d3dcompiler.h>
#include <dcomp.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <cstddef>
#include <sstream>
#include <unordered_map>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dcomp.lib")

namespace am::renderer::rhi
{
    namespace
    {
        using Microsoft::WRL::ComPtr;

        std::string Hr(const char* what, HRESULT hr)
        {
            std::ostringstream ss;
            ss << what << " failed, hr=0x" << std::hex << static_cast<unsigned long>(hr);
            return ss.str();
        }

        U64 Key(Handle h)
        {
            return (static_cast<U64>(h.generation) << 32U) | static_cast<U64>(h.index);
        }

        DXGI_FORMAT ToDxgi(Format f)
        {
            switch (f)
            {
            case Format::R8: return DXGI_FORMAT_R8_UNORM;
            case Format::RG8: return DXGI_FORMAT_R8G8_UNORM;
            case Format::RGBA8: return DXGI_FORMAT_R8G8B8A8_UNORM;
            case Format::BGRA8: return DXGI_FORMAT_B8G8R8A8_UNORM;
            case Format::RGBA16F: return DXGI_FORMAT_R16G16B16A16_FLOAT;
            case Format::R32F: return DXGI_FORMAT_R32_FLOAT;
            case Format::RG32F: return DXGI_FORMAT_R32G32_FLOAT;
            case Format::RGB32F: return DXGI_FORMAT_R32G32B32_FLOAT;
            case Format::RGBA32F: return DXGI_FORMAT_R32G32B32A32_FLOAT;
            case Format::R32U: return DXGI_FORMAT_R32_UINT;
            case Format::D24S8: return DXGI_FORMAT_D24_UNORM_S8_UINT;
            case Format::D32F: return DXGI_FORMAT_D32_FLOAT;
            case Format::Unknown: return DXGI_FORMAT_UNKNOWN;
            }
            return DXGI_FORMAT_UNKNOWN;
        }


        D3D12_PRIMITIVE_TOPOLOGY_TYPE ToTopologyType(Topology topology)
        {
            switch (topology)
            {
            case Topology::TriangleList:
            case Topology::TriangleStrip:
                return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            case Topology::LineList:
            case Topology::LineStrip:
                return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
            case Topology::PointList:
                return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
            }
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        }

        D3D12_PRIMITIVE_TOPOLOGY ToDxTopology(Topology topology)
        {
            switch (topology)
            {
            case Topology::TriangleList: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
            case Topology::TriangleStrip: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
            case Topology::LineList: return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
            case Topology::LineStrip: return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
            case Topology::PointList: return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
            }
            return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        }

        D3D12_RESOURCE_STATES ToState(Access access)
        {
            switch (access)
            {
            case Access::Common: return D3D12_RESOURCE_STATE_COMMON;
            case Access::Vertex: return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
            case Access::Index: return D3D12_RESOURCE_STATE_INDEX_BUFFER;
            case Access::Constant: return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
            case Access::ShaderRead: return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
            case Access::ShaderWrite: return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            case Access::RenderTarget: return D3D12_RESOURCE_STATE_RENDER_TARGET;
            case Access::DepthRead: return D3D12_RESOURCE_STATE_DEPTH_READ;
            case Access::DepthWrite: return D3D12_RESOURCE_STATE_DEPTH_WRITE;
            case Access::CopyRead: return D3D12_RESOURCE_STATE_COPY_SOURCE;
            case Access::CopyWrite: return D3D12_RESOURCE_STATE_COPY_DEST;
            case Access::Present: return D3D12_RESOURCE_STATE_PRESENT;
            case Access::Indirect: return D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
            case Access::Unknown: return D3D12_RESOURCE_STATE_COMMON;
            }
            return D3D12_RESOURCE_STATE_COMMON;
        }

        D3D12_RESOURCE_FLAGS ToResourceFlags(Usage usage)
        {
            D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
            if (Has(usage, Usage::RenderTarget))
            {
                flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
            }
            if (Has(usage, Usage::DepthStencil))
            {
                flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
            }
            if (Has(usage, Usage::UnorderedAccess))
            {
                flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            }
            return flags;
        }

        D3D12_HEAP_TYPE ToHeapType(Memory memory)
        {
            switch (memory)
            {
            case Memory::Upload: return D3D12_HEAP_TYPE_UPLOAD;
            case Memory::Readback: return D3D12_HEAP_TYPE_READBACK;
            case Memory::GpuOnly:
            case Memory::Transient:
            default:
                return D3D12_HEAP_TYPE_DEFAULT;
            }
        }

        D3D12_RESOURCE_STATES InitialState(Memory memory, Usage usage)
        {
            if (memory == Memory::Upload)
            {
                return D3D12_RESOURCE_STATE_GENERIC_READ;
            }
            if (memory == Memory::Readback)
            {
                return D3D12_RESOURCE_STATE_COPY_DEST;
            }
            if (Has(usage, Usage::RenderTarget))
            {
                return D3D12_RESOURCE_STATE_RENDER_TARGET;
            }
            if (Has(usage, Usage::DepthStencil))
            {
                return D3D12_RESOURCE_STATE_DEPTH_WRITE;
            }
            return D3D12_RESOURCE_STATE_COMMON;
        }

        D3D12_RESOURCE_DESC BufferResourceDesc(U64 size)
        {
            D3D12_RESOURCE_DESC desc{};
            desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            desc.Alignment = 0;
            desc.Width = std::max<U64>(1, size);
            desc.Height = 1;
            desc.DepthOrArraySize = 1;
            desc.MipLevels = 1;
            desc.Format = DXGI_FORMAT_UNKNOWN;
            desc.SampleDesc.Count = 1;
            desc.SampleDesc.Quality = 0;
            desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            desc.Flags = D3D12_RESOURCE_FLAG_NONE;
            return desc;
        }

        D3D12_RESOURCE_DESC TextureResourceDesc(const TextureDesc& desc)
        {
            D3D12_RESOURCE_DESC out{};
            out.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            out.Alignment = 0;
            out.Width = std::max<U32>(1, desc.extent.width);
            out.Height = std::max<U32>(1, desc.extent.height);
            out.DepthOrArraySize = static_cast<UINT16>(std::max<U32>(1, desc.layers));
            out.MipLevels = static_cast<UINT16>(std::max<U32>(1, desc.mips));
            out.Format = ToDxgi(desc.format);
            out.SampleDesc.Count = 1;
            out.SampleDesc.Quality = 0;
            out.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
            out.Flags = ToResourceFlags(desc.usage);
            return out;
        }

        D3D12_CLEAR_VALUE MakeDx12ClearValue(const TextureDesc& textureDesc)
        {
            D3D12_CLEAR_VALUE clear{};
            clear.Format = ToDxgi(textureDesc.format);
            if (IsDepth(textureDesc.format))
            {
                clear.DepthStencil.Depth = textureDesc.clear.depth;
                clear.DepthStencil.Stencil = textureDesc.clear.stencil;
            }
            else
            {
                clear.Color[0] = textureDesc.clear.color.r;
                clear.Color[1] = textureDesc.clear.color.g;
                clear.Color[2] = textureDesc.clear.color.b;
                clear.Color[3] = textureDesc.clear.color.a;
            }
            return clear;
        }

        D3D12_HEAP_PROPERTIES HeapProps(D3D12_HEAP_TYPE type)
        {
            D3D12_HEAP_PROPERTIES props{};
            props.Type = type;
            props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
            props.CreationNodeMask = 1;
            props.VisibleNodeMask = 1;
            return props;
        }

        D3D12_RESOURCE_BARRIER Transition(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
        {
            D3D12_RESOURCE_BARRIER b{};
            b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            b.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            b.Transition.pResource = resource;
            b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            b.Transition.StateBefore = before;
            b.Transition.StateAfter = after;
            return b;
        }

        U64 AlignUp(U64 v, U64 alignment)
        {
            return (v + alignment - 1ULL) & ~(alignment - 1ULL);
        }


        std::array<float, 16> IdentityWvp()
        {
            return {
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f
            };
        }
    }

    struct Dx12Descriptor
    {
        UINT index = 0;
        D3D12_CPU_DESCRIPTOR_HANDLE cpu{};
        D3D12_GPU_DESCRIPTOR_HANDLE gpu{};
    };

    struct Dx12DescriptorHeap
    {
        ComPtr<ID3D12DescriptorHeap> heap;
        D3D12_DESCRIPTOR_HEAP_TYPE type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        UINT capacity = 0;
        UINT used = 0;
        UINT increment = 0;
        bool shaderVisible = false;

        bool initialize(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT count, bool visible, std::string* error)
        {
            type = heapType;
            capacity = count;
            used = 0;
            shaderVisible = visible;

            D3D12_DESCRIPTOR_HEAP_DESC desc{};
            desc.Type = heapType;
            desc.NumDescriptors = count;
            desc.Flags = visible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

            const HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap));
            if (FAILED(hr))
            {
                if (error) { *error = Hr("CreateDescriptorHeap", hr); }
                return false;
            }

            increment = device->GetDescriptorHandleIncrementSize(heapType);
            return true;
        }

        Dx12Descriptor allocate(std::string* error)
        {
            if (used >= capacity)
            {
                if (error) { *error = "DX12 descriptor heap exhausted."; }
                return {};
            }

            Dx12Descriptor out{};
            out.index = used++;
            out.cpu = heap->GetCPUDescriptorHandleForHeapStart();
            out.cpu.ptr += static_cast<SIZE_T>(out.index) * increment;
            if (shaderVisible)
            {
                out.gpu = heap->GetGPUDescriptorHandleForHeapStart();
                out.gpu.ptr += static_cast<UINT64>(out.index) * increment;
            }
            return out;
        }
    };

    struct Dx12NativeBuffer
    {
        Buffer handle{};
        BufferDesc desc{};
        ComPtr<ID3D12Resource> resource;
        D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;
        void* mapped = nullptr;
    };

    struct Dx12NativeTexture
    {
        Texture handle{};
        TextureDesc desc{};
        ComPtr<ID3D12Resource> resource;
        D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;
        bool hasRtv = false;
        bool hasDsv = false;
        Dx12Descriptor rtv{};
        Dx12Descriptor dsv{};
    };


    struct Dx12NativePipeline
    {
        Pipeline handle{};
        GraphicsPipelineDesc desc{};
        ComPtr<ID3D12RootSignature> rootSignature;
        ComPtr<ID3D12PipelineState> state;
        D3D12_PRIMITIVE_TOPOLOGY topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    };

    namespace
    {
        constexpr const char* kAceRhi3BasicColorHlsl = R"(
cbuffer AceFrame : register(b0)
{
    row_major float4x4 gWvp;
};

struct VSIn
{
    float3 pos : POSITION;
    float4 color : COLOR0;
};

struct VSOut
{
    float4 pos : SV_Position;
    float4 color : COLOR0;
};

VSOut VSMain(VSIn input)
{
    VSOut output;
    output.pos = mul(float4(input.pos, 1.0f), gWvp);
    output.color = input.color;
    return output;
}

float4 PSMain(VSOut input) : SV_Target0
{
    return input.color;
}
)";
    }

    struct Dx12UploadArena
    {
        ComPtr<ID3D12Resource> resource;
        U8* mapped = nullptr;
        U64 capacity = 0;
        U64 offset = 0;

        bool initialize(ID3D12Device* device, U64 bytes, std::string* error)
        {
            capacity = std::max<U64>(bytes, 1024 * 1024);
            offset = 0;

            auto props = HeapProps(D3D12_HEAP_TYPE_UPLOAD);
            auto desc = BufferResourceDesc(capacity);
            const HRESULT hr = device->CreateCommittedResource(
                &props,
                D3D12_HEAP_FLAG_NONE,
                &desc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&resource));

            if (FAILED(hr))
            {
                if (error) { *error = Hr("CreateCommittedResource upload arena", hr); }
                return false;
            }

            D3D12_RANGE noRead{0, 0};
            const HRESULT mapHr = resource->Map(0, &noRead, reinterpret_cast<void**>(&mapped));
            if (FAILED(mapHr))
            {
                if (error) { *error = Hr("Map upload arena", mapHr); }
                return false;
            }

            return true;
        }

        void reset()
        {
            offset = 0;
        }

        bool allocate(U64 bytes, U64 alignment, U64* outOffset, void** outCpu, std::string* error)
        {
            const U64 aligned = AlignUp(offset, alignment);
            if (aligned + bytes > capacity)
            {
                if (error) { *error = "DX12 upload arena exhausted."; }
                return false;
            }

            *outOffset = aligned;
            *outCpu = mapped + aligned;
            offset = aligned + bytes;
            return true;
        }

        void shutdown()
        {
            if (resource && mapped)
            {
                resource->Unmap(0, nullptr);
            }
            resource.Reset();
            mapped = nullptr;
            capacity = 0;
            offset = 0;
        }
    };


    struct Dx12CompositionHost
    {
        HWND hwnd = nullptr;
        Extent2D extent{};
        float left = 0.0f;
        float top = 0.0f;
        bool ready = false;
        std::vector<D3D12_RESOURCE_STATES> backBufferStates;
        ComPtr<IDCompositionDevice> dcompDevice;
        ComPtr<IDCompositionTarget> dcompTarget;
        ComPtr<IDCompositionVisual> dcompVisual;
        ComPtr<IDXGISwapChain3> swapChain;
    };

    struct Dx12ReadbackCache
    {
        ComPtr<ID3D12Resource> resource;
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
        UINT rowCount = 0;
        UINT64 rowSizeBytes = 0;
        UINT64 totalBytes = 0;
        DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
        Extent2D extent{};

        bool matches(const D3D12_RESOURCE_DESC& desc) const
        {
            return resource &&
                extent.width == static_cast<U32>(desc.Width) &&
                extent.height == static_cast<U32>(desc.Height) &&
                format == desc.Format;
        }

        void reset()
        {
            resource.Reset();
            footprint = {};
            rowCount = 0;
            rowSizeBytes = 0;
            totalBytes = 0;
            format = DXGI_FORMAT_UNKNOWN;
            extent = {};
        }
    };

    struct Dx12Device::Impl
    {
        DeviceDesc desc{};
        Registry registry{};
        Stats deviceStats{};
        Dx12GpuAllocationStats gpuStats{};

        ComPtr<IDXGIFactory6> factory;
        ComPtr<IDXGIAdapter1> adapter;
        ComPtr<ID3D12Device> device;
        ComPtr<ID3D12CommandQueue> graphicsQueue;
        ComPtr<ID3D12CommandAllocator> allocator;
        ComPtr<ID3D12GraphicsCommandList> commandList;
        ComPtr<ID3D12Fence> fence;
        HANDLE fenceEvent = nullptr;
        U64 nextFenceValue = 1;
        bool isInitialized = false;
        bool insideFrame = false;
        U64 frameIndex = 0;

        Dx12DescriptorHeap rtvHeap;
        Dx12DescriptorHeap dsvHeap;
        Dx12DescriptorHeap srvHeap;
        Dx12UploadArena uploadArena;

        std::unordered_map<U64, Dx12NativeBuffer> buffers;
        std::unordered_map<U64, Dx12NativeTexture> textures;
        std::unordered_map<U64, Dx12NativePipeline> pipelines;
        Dx12CompositionHost composition;
        Dx12ReadbackCache readbackCache;

        bool initializeFactory(std::string* error)
        {
            UINT flags = 0;
#if defined(_DEBUG)
            ComPtr<ID3D12Debug> debug;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))
            {
                debug->EnableDebugLayer();
                flags |= DXGI_CREATE_FACTORY_DEBUG;
            }
#endif
            const HRESULT hr = CreateDXGIFactory2(flags, IID_PPV_ARGS(&factory));
            if (FAILED(hr))
            {
                if (error) { *error = Hr("CreateDXGIFactory2", hr); }
                return false;
            }
            return true;
        }

        bool chooseAdapter(std::string* error)
        {
            for (UINT i = 0; ; ++i)
            {
                ComPtr<IDXGIAdapter1> candidate;
                if (factory->EnumAdapters1(i, &candidate) == DXGI_ERROR_NOT_FOUND)
                {
                    break;
                }

                DXGI_ADAPTER_DESC1 desc1{};
                candidate->GetDesc1(&desc1);
                if ((desc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0)
                {
                    continue;
                }

                if (SUCCEEDED(D3D12CreateDevice(candidate.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)))
                {
                    adapter = candidate;
                    return true;
                }
            }

            if (error) { *error = "No hardware D3D12 adapter found."; }
            return false;
        }

        bool initializeDevice(std::string* error)
        {
            const HRESULT hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device));
            if (FAILED(hr))
            {
                if (error) { *error = Hr("D3D12CreateDevice", hr); }
                return false;
            }

            D3D12_COMMAND_QUEUE_DESC queueDesc{};
            queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
            queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
            queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
            queueDesc.NodeMask = 0;

            HRESULT qhr = device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&graphicsQueue));
            if (FAILED(qhr))
            {
                if (error) { *error = Hr("CreateCommandQueue", qhr); }
                return false;
            }

            HRESULT ahr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator));
            if (FAILED(ahr))
            {
                if (error) { *error = Hr("CreateCommandAllocator", ahr); }
                return false;
            }

            HRESULT lhr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr, IID_PPV_ARGS(&commandList));
            if (FAILED(lhr))
            {
                if (error) { *error = Hr("CreateCommandList", lhr); }
                return false;
            }
            commandList->Close();

            HRESULT fhr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
            if (FAILED(fhr))
            {
                if (error) { *error = Hr("CreateFence", fhr); }
                return false;
            }

            fenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
            if (!fenceEvent)
            {
                if (error) { *error = "CreateEventW for DX12 fence failed."; }
                return false;
            }

            if (!rtvHeap.initialize(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2048, false, error)) { return false; }
            if (!dsvHeap.initialize(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 512, false, error)) { return false; }
            if (!srvHeap.initialize(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 4096, true, error)) { return false; }
            if (!uploadArena.initialize(device.Get(), 16ULL * 1024ULL * 1024ULL, error)) { return false; }

            return true;
        }

        bool resetCommandList(std::string* error)
        {
            // ACE-PERF2: executeCommandList() is synchronous in this single-threaded
            // debug RHI and already waits for the submitted fence. Waiting again before
            // every allocator reset doubled the CPU-side stall count, especially on
            // render+composition frames. UE-style command submission does not insert
            // decorative waits just to make frame pacing sad.
            const HRESULT ahr = allocator->Reset();
            if (FAILED(ahr))
            {
                if (error) { *error = Hr("CommandAllocator::Reset", ahr); }
                return false;
            }

            const HRESULT lhr = commandList->Reset(allocator.Get(), nullptr);
            if (FAILED(lhr))
            {
                if (error) { *error = Hr("GraphicsCommandList::Reset", lhr); }
                return false;
            }

            return true;
        }

        bool executeCommandList(std::string* error)
        {
            const HRESULT closeHr = commandList->Close();
            if (FAILED(closeHr))
            {
                if (error) { *error = Hr("GraphicsCommandList::Close", closeHr); }
                return false;
            }

            ID3D12CommandList* lists[] = { commandList.Get() };
            graphicsQueue->ExecuteCommandLists(1, lists);
            ++gpuStats.submittedGpuCommandLists;

            const U64 signalValue = nextFenceValue++;
            const HRESULT signalHr = graphicsQueue->Signal(fence.Get(), signalValue);
            if (FAILED(signalHr))
            {
                if (error) { *error = Hr("CommandQueue::Signal", signalHr); }
                return false;
            }

            if (fence->GetCompletedValue() < signalValue)
            {
                const HRESULT eventHr = fence->SetEventOnCompletion(signalValue, fenceEvent);
                if (FAILED(eventHr))
                {
                    if (error) { *error = Hr("Fence::SetEventOnCompletion", eventHr); }
                    return false;
                }
                const auto waitStart = std::chrono::steady_clock::now();
                WaitForSingleObject(fenceEvent, INFINITE);
                const auto waitEnd = std::chrono::steady_clock::now();
                ++gpuStats.blockingFenceWaits;
                gpuStats.blockingFenceWaitMs += std::chrono::duration<double, std::milli>(waitEnd - waitStart).count();
            }

            gpuStats.completedFenceValue = fence->GetCompletedValue();
            return true;
        }

        void waitIdle()
        {
            if (!graphicsQueue || !fence)
            {
                return;
            }

            const U64 signalValue = nextFenceValue++;
            if (SUCCEEDED(graphicsQueue->Signal(fence.Get(), signalValue)))
            {
                if (fence->GetCompletedValue() < signalValue)
                {
                    fence->SetEventOnCompletion(signalValue, fenceEvent);
                    const auto waitStart = std::chrono::steady_clock::now();
                    WaitForSingleObject(fenceEvent, INFINITE);
                    const auto waitEnd = std::chrono::steady_clock::now();
                    ++gpuStats.blockingFenceWaits;
                    gpuStats.blockingFenceWaitMs += std::chrono::duration<double, std::milli>(waitEnd - waitStart).count();
                }
                gpuStats.completedFenceValue = fence->GetCompletedValue();
            }
        }

        bool ensureBuffer(Buffer buffer, std::string* error)
        {
            if (!buffer.valid())
            {
                if (error) { *error = "Cannot ensure native DX12 buffer for invalid handle."; }
                return false;
            }

            const U64 key = Key(buffer.h);
            if (buffers.find(key) != buffers.end())
            {
                return true;
            }

            const auto* bufferDesc = registry.desc(buffer);
            if (!bufferDesc)
            {
                if (error) { *error = "Cannot ensure native DX12 buffer: missing RHI desc."; }
                return false;
            }

            const auto heapType = ToHeapType(bufferDesc->memory);
            const auto initialState = InitialState(bufferDesc->memory, bufferDesc->usage);
            auto props = HeapProps(heapType);
            auto resourceDesc = BufferResourceDesc(bufferDesc->size);

            ComPtr<ID3D12Resource> resource;
            const HRESULT hr = device->CreateCommittedResource(
                &props,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                initialState,
                nullptr,
                IID_PPV_ARGS(&resource));

            if (FAILED(hr))
            {
                if (error) { *error = Hr("CreateCommittedResource buffer", hr); }
                return false;
            }

            if (!bufferDesc->name.empty())
            {
                const std::wstring wide(bufferDesc->name.value.begin(), bufferDesc->name.value.end());
                resource->SetName(wide.c_str());
            }

            Dx12NativeBuffer native{};
            native.handle = buffer;
            native.desc = *bufferDesc;
            native.resource = resource;
            native.state = initialState;

            if (bufferDesc->persistentMap || bufferDesc->memory == Memory::Upload)
            {
                D3D12_RANGE noRead{0, 0};
                resource->Map(0, &noRead, &native.mapped);
            }

            buffers.emplace(key, std::move(native));
            ++gpuStats.nativeBuffers;
            return true;
        }

        bool ensureTexture(Texture texture, std::string* error)
        {
            if (!texture.valid())
            {
                if (error) { *error = "Cannot ensure native DX12 texture for invalid handle."; }
                return false;
            }

            const U64 key = Key(texture.h);
            if (textures.find(key) != textures.end())
            {
                return true;
            }

            const auto* textureDesc = registry.desc(texture);
            if (!textureDesc)
            {
                if (error) { *error = "Cannot ensure native DX12 texture: missing RHI desc."; }
                return false;
            }

            const auto heapType = ToHeapType(textureDesc->memory);
            const auto initialState = InitialState(textureDesc->memory, textureDesc->usage);
            auto props = HeapProps(heapType);
            auto resourceDesc = TextureResourceDesc(*textureDesc);
            auto clear = MakeDx12ClearValue(*textureDesc);
            const bool wantsClear = Has(textureDesc->usage, Usage::RenderTarget) || Has(textureDesc->usage, Usage::DepthStencil);

            ComPtr<ID3D12Resource> resource;
            const HRESULT hr = device->CreateCommittedResource(
                &props,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                initialState,
                wantsClear ? &clear : nullptr,
                IID_PPV_ARGS(&resource));

            if (FAILED(hr))
            {
                if (error) { *error = Hr("CreateCommittedResource texture", hr); }
                return false;
            }

            if (!textureDesc->name.empty())
            {
                const std::wstring wide(textureDesc->name.value.begin(), textureDesc->name.value.end());
                resource->SetName(wide.c_str());
            }

            Dx12NativeTexture native{};
            native.handle = texture;
            native.desc = *textureDesc;
            native.resource = resource;
            native.state = initialState;

            if (Has(textureDesc->usage, Usage::RenderTarget))
            {
                native.rtv = rtvHeap.allocate(error);
                if (native.rtv.cpu.ptr == 0) { return false; }
                D3D12_RENDER_TARGET_VIEW_DESC rtv{};
                rtv.Format = ToDxgi(textureDesc->format);
                rtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
                device->CreateRenderTargetView(resource.Get(), &rtv, native.rtv.cpu);
                native.hasRtv = true;
                ++gpuStats.descriptorAllocations;
            }

            if (Has(textureDesc->usage, Usage::DepthStencil))
            {
                native.dsv = dsvHeap.allocate(error);
                if (native.dsv.cpu.ptr == 0) { return false; }
                D3D12_DEPTH_STENCIL_VIEW_DESC dsv{};
                dsv.Format = ToDxgi(textureDesc->format);
                dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
                dsv.Flags = D3D12_DSV_FLAG_NONE;
                device->CreateDepthStencilView(resource.Get(), &dsv, native.dsv.cpu);
                native.hasDsv = true;
                ++gpuStats.descriptorAllocations;
            }

            textures.emplace(key, std::move(native));
            ++gpuStats.nativeTextures;
            return true;
        }

        Dx12NativeBuffer* native(Buffer b)
        {
            const auto it = buffers.find(Key(b.h));
            return it == buffers.end() ? nullptr : &it->second;
        }

        Dx12NativeTexture* native(Texture t)
        {
            const auto it = textures.find(Key(t.h));
            return it == textures.end() ? nullptr : &it->second;
        }

        Dx12NativePipeline* native(Pipeline p)
        {
            const auto it = pipelines.find(Key(p.h));
            return it == pipelines.end() ? nullptr : &it->second;
        }

        bool compileShader(const char* source, const char* entry, const char* profile, ComPtr<ID3DBlob>& blob, std::string* error)
        {
            UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(_DEBUG)
            flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
            flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

            ComPtr<ID3DBlob> errors;
            const HRESULT hr = D3DCompile(
                source,
                std::strlen(source),
                "AceRhi3_BasicColor.hlsl",
                nullptr,
                nullptr,
                entry,
                profile,
                flags,
                0,
                &blob,
                &errors);

            if (FAILED(hr))
            {
                if (error)
                {
                    if (errors)
                    {
                        *error = std::string(static_cast<const char*>(errors->GetBufferPointer()), errors->GetBufferSize());
                    }
                    else
                    {
                        *error = Hr("D3DCompile", hr);
                    }
                }
                return false;
            }

            ++gpuStats.compiledShaders;
            return true;
        }

        bool ensurePipeline(Pipeline pipeline, std::string* error)
        {
            if (!pipeline.valid())
            {
                if (error) { *error = "Cannot ensure native DX12 pipeline for invalid handle."; }
                return false;
            }

            const U64 key = Key(pipeline.h);
            if (pipelines.find(key) != pipelines.end())
            {
                return true;
            }

            const auto* pipelineDesc = registry.desc(pipeline);
            if (!pipelineDesc)
            {
                if (error) { *error = "Cannot ensure native DX12 pipeline: missing RHI desc."; }
                return false;
            }

            ComPtr<ID3DBlob> vs;
            ComPtr<ID3DBlob> ps;
            if (!compileShader(kAceRhi3BasicColorHlsl, "VSMain", "vs_5_0", vs, error))
            {
                return false;
            }
            if (!compileShader(kAceRhi3BasicColorHlsl, "PSMain", "ps_5_0", ps, error))
            {
                return false;
            }

            D3D12_ROOT_PARAMETER rootParameters[1]{};
            rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
            rootParameters[0].Constants.ShaderRegister = 0;
            rootParameters[0].Constants.RegisterSpace = 0;
            rootParameters[0].Constants.Num32BitValues = 16;
            rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

            D3D12_ROOT_SIGNATURE_DESC rsDesc{};
            rsDesc.NumParameters = 1;
            rsDesc.pParameters = rootParameters;
            rsDesc.NumStaticSamplers = 0;
            rsDesc.pStaticSamplers = nullptr;
            rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

            ComPtr<ID3DBlob> signature;
            ComPtr<ID3DBlob> signatureErrors;
            const HRESULT serializeHr = D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &signatureErrors);
            if (FAILED(serializeHr))
            {
                if (error)
                {
                    if (signatureErrors)
                    {
                        *error = std::string(static_cast<const char*>(signatureErrors->GetBufferPointer()), signatureErrors->GetBufferSize());
                    }
                    else
                    {
                        *error = Hr("D3D12SerializeRootSignature", serializeHr);
                    }
                }
                return false;
            }

            ComPtr<ID3D12RootSignature> rootSignature;
            const HRESULT rootHr = device->CreateRootSignature(
                0,
                signature->GetBufferPointer(),
                signature->GetBufferSize(),
                IID_PPV_ARGS(&rootSignature));
            if (FAILED(rootHr))
            {
                if (error) { *error = Hr("CreateRootSignature", rootHr); }
                return false;
            }

            D3D12_INPUT_ELEMENT_DESC inputLayout[2]{};
            inputLayout[0].SemanticName = "POSITION";
            inputLayout[0].SemanticIndex = 0;
            inputLayout[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
            inputLayout[0].InputSlot = 0;
            inputLayout[0].AlignedByteOffset = 0;
            inputLayout[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
            inputLayout[0].InstanceDataStepRate = 0;

            inputLayout[1].SemanticName = "COLOR";
            inputLayout[1].SemanticIndex = 0;
            inputLayout[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
            inputLayout[1].InputSlot = 0;
            inputLayout[1].AlignedByteOffset = 12;
            inputLayout[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
            inputLayout[1].InstanceDataStepRate = 0;

            D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};
            pso.pRootSignature = rootSignature.Get();
            pso.VS = { vs->GetBufferPointer(), vs->GetBufferSize() };
            pso.PS = { ps->GetBufferPointer(), ps->GetBufferSize() };
            pso.BlendState.AlphaToCoverageEnable = FALSE;
            pso.BlendState.IndependentBlendEnable = FALSE;
            for (auto& target : pso.BlendState.RenderTarget)
            {
                target.BlendEnable = FALSE;
                target.LogicOpEnable = FALSE;
                target.SrcBlend = D3D12_BLEND_ONE;
                target.DestBlend = D3D12_BLEND_ZERO;
                target.BlendOp = D3D12_BLEND_OP_ADD;
                target.SrcBlendAlpha = D3D12_BLEND_ONE;
                target.DestBlendAlpha = D3D12_BLEND_ZERO;
                target.BlendOpAlpha = D3D12_BLEND_OP_ADD;
                target.LogicOp = D3D12_LOGIC_OP_NOOP;
                target.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
            }

            pso.SampleMask = UINT_MAX;
            pso.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
            pso.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
            pso.RasterizerState.FrontCounterClockwise = FALSE;
            pso.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
            pso.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
            pso.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
            pso.RasterizerState.DepthClipEnable = TRUE;
            pso.RasterizerState.MultisampleEnable = FALSE;
            pso.RasterizerState.AntialiasedLineEnable = FALSE;
            pso.RasterizerState.ForcedSampleCount = 0;
            pso.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

            pso.DepthStencilState.DepthEnable = pipelineDesc->depthFormat != Format::Unknown;
            pso.DepthStencilState.DepthWriteMask = pso.DepthStencilState.DepthEnable ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
            pso.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
            pso.DepthStencilState.StencilEnable = FALSE;
            pso.DepthStencilState.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
            pso.DepthStencilState.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
            pso.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
            pso.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
            pso.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
            pso.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
            pso.DepthStencilState.BackFace = pso.DepthStencilState.FrontFace;

            pso.InputLayout = { inputLayout, 2 };
            pso.PrimitiveTopologyType = ToTopologyType(pipelineDesc->topology);
            pso.NumRenderTargets = static_cast<UINT>(std::min<std::size_t>(pipelineDesc->colorFormats.size(), 8));
            for (UINT i = 0; i < pso.NumRenderTargets; ++i)
            {
                pso.RTVFormats[i] = ToDxgi(pipelineDesc->colorFormats[i]);
            }
            pso.DSVFormat = ToDxgi(pipelineDesc->depthFormat);
            pso.SampleDesc.Count = 1;
            pso.SampleDesc.Quality = 0;
            pso.NodeMask = 0;
            pso.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

            ComPtr<ID3D12PipelineState> state;
            const HRESULT psoHr = device->CreateGraphicsPipelineState(&pso, IID_PPV_ARGS(&state));
            if (FAILED(psoHr))
            {
                if (error) { *error = Hr("CreateGraphicsPipelineState", psoHr); }
                return false;
            }

            Dx12NativePipeline nativePipeline{};
            nativePipeline.handle = pipeline;
            nativePipeline.desc = *pipelineDesc;
            nativePipeline.rootSignature = rootSignature;
            nativePipeline.state = state;
            nativePipeline.topology = ToDxTopology(pipelineDesc->topology);
            pipelines.emplace(key, std::move(nativePipeline));
            ++gpuStats.nativePipelines;
            return true;
        }

        void transition(Dx12NativeBuffer& native, D3D12_RESOURCE_STATES after)
        {
            if (native.desc.memory == Memory::Upload)
            {
                // Upload heap resources are permanently GENERIC_READ. They can feed
                // vertex/constant input directly without a transition barrier.
                native.state = D3D12_RESOURCE_STATE_GENERIC_READ;
                (void)after;
                return;
            }
            if (native.desc.memory == Memory::Readback)
            {
                (void)after;
                return;
            }
            if (native.state == after)
            {
                return;
            }
            auto barrier = Transition(native.resource.Get(), native.state, after);
            commandList->ResourceBarrier(1, &barrier);
            native.state = after;
        }

        void transition(Dx12NativeTexture& native, D3D12_RESOURCE_STATES after)
        {
            if (native.state == after)
            {
                return;
            }
            auto barrier = Transition(native.resource.Get(), native.state, after);
            commandList->ResourceBarrier(1, &barrier);
            native.state = after;
        }

        void resetComposition()
        {
            composition.swapChain.Reset();
            composition.dcompVisual.Reset();
            composition.dcompTarget.Reset();
            composition.dcompDevice.Reset();
            composition.backBufferStates.clear();
            composition.hwnd = nullptr;
            composition.extent = {};
            composition.left = 0.0f;
            composition.top = 0.0f;
            composition.ready = false;
        }

        bool ensureComposition(HWND hwnd, Extent2D extent, float left, float top, std::string* error)
        {
            if (!hwnd)
            {
                if (error) { *error = "DX12 composition requires a valid HWND."; }
                return false;
            }

            extent.width = std::max<U32>(1, extent.width);
            extent.height = std::max<U32>(1, extent.height);

            const bool sameTarget =
                composition.ready &&
                composition.hwnd == hwnd &&
                composition.extent.width == extent.width &&
                composition.extent.height == extent.height;

            if (!composition.ready || composition.hwnd != hwnd)
            {
                resetComposition();

                const HRESULT deviceHr = DCompositionCreateDevice(nullptr, IID_PPV_ARGS(&composition.dcompDevice));
                if (FAILED(deviceHr))
                {
                    if (error) { *error = Hr("DCompositionCreateDevice", deviceHr); }
                    return false;
                }

                const HRESULT targetHr = composition.dcompDevice->CreateTargetForHwnd(hwnd, TRUE, &composition.dcompTarget);
                if (FAILED(targetHr))
                {
                    if (error) { *error = Hr("CreateTargetForHwnd", targetHr); }
                    resetComposition();
                    return false;
                }

                const HRESULT visualHr = composition.dcompDevice->CreateVisual(&composition.dcompVisual);
                if (FAILED(visualHr))
                {
                    if (error) { *error = Hr("CreateVisual", visualHr); }
                    resetComposition();
                    return false;
                }

                const HRESULT rootHr = composition.dcompTarget->SetRoot(composition.dcompVisual.Get());
                if (FAILED(rootHr))
                {
                    if (error) { *error = Hr("IDCompositionTarget::SetRoot", rootHr); }
                    resetComposition();
                    return false;
                }

                composition.hwnd = hwnd;
            }

            if (!sameTarget || !composition.swapChain)
            {
                waitIdle();

                DXGI_SWAP_CHAIN_DESC1 swapDesc{};
                swapDesc.Width = extent.width;
                swapDesc.Height = extent.height;
                swapDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
                swapDesc.Stereo = FALSE;
                swapDesc.SampleDesc.Count = 1;
                swapDesc.SampleDesc.Quality = 0;
                swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
                swapDesc.BufferCount = 2;
                swapDesc.Scaling = DXGI_SCALING_STRETCH;
                swapDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
                swapDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
                swapDesc.Flags = 0;

                ComPtr<IDXGISwapChain1> swapChain1;
                const HRESULT swapHr = factory->CreateSwapChainForComposition(
                    graphicsQueue.Get(),
                    &swapDesc,
                    nullptr,
                    &swapChain1);

                if (FAILED(swapHr))
                {
                    if (error) { *error = Hr("CreateSwapChainForComposition", swapHr); }
                    return false;
                }

                const HRESULT qHr = swapChain1.As(&composition.swapChain);
                if (FAILED(qHr))
                {
                    if (error) { *error = Hr("Query IDXGISwapChain3", qHr); }
                    return false;
                }

                const HRESULT contentHr = composition.dcompVisual->SetContent(composition.swapChain.Get());
                if (FAILED(contentHr))
                {
                    if (error) { *error = Hr("IDCompositionVisual::SetContent", contentHr); }
                    return false;
                }

                composition.backBufferStates.assign(2, D3D12_RESOURCE_STATE_PRESENT);
                composition.extent = extent;
                ++gpuStats.compositionResizes;
            }

            if (composition.left != left || composition.top != top)
            {
                composition.dcompVisual->SetOffsetX(left);
                composition.dcompVisual->SetOffsetY(top);
                composition.left = left;
                composition.top = top;
            }

            if (composition.dcompVisual)
            {
                D2D_RECT_F clipRect{};
                clipRect.left = 0.0f;
                clipRect.top = 0.0f;
                clipRect.right = static_cast<float>(extent.width);
                clipRect.bottom = static_cast<float>(extent.height);
                composition.dcompVisual->SetClip(clipRect);
            }

            const HRESULT commitHr = composition.dcompDevice->Commit();
            if (FAILED(commitHr))
            {
                if (error) { *error = Hr("IDCompositionDevice::Commit", commitHr); }
                return false;
            }

            composition.ready = true;
            return true;
        }

        bool presentComposition(Texture source, HWND hwnd, float left, float top, Extent2D extent, std::string* error)
        {
            if (!ensureTexture(source, error))
            {
                return false;
            }

            auto* src = native(source);
            if (!src || !src->resource)
            {
                if (error) { *error = "DX12 composition source texture is missing."; }
                return false;
            }

            if (src->desc.format != Format::BGRA8)
            {
                if (error) { *error = "DX12 composition currently requires BGRA8 SceneColor."; }
                return false;
            }

            if (!ensureComposition(hwnd, extent, left, top, error))
            {
                return false;
            }

            const UINT backBufferIndex = composition.swapChain->GetCurrentBackBufferIndex();
            ComPtr<ID3D12Resource> backBuffer;
            const HRESULT bufferHr = composition.swapChain->GetBuffer(backBufferIndex, IID_PPV_ARGS(&backBuffer));
            if (FAILED(bufferHr))
            {
                if (error) { *error = Hr("IDXGISwapChain::GetBuffer", bufferHr); }
                return false;
            }

            if (!resetCommandList(error))
            {
                return false;
            }

            transition(*src, D3D12_RESOURCE_STATE_COPY_SOURCE);

            auto before = composition.backBufferStates.size() > backBufferIndex ?
                composition.backBufferStates[backBufferIndex] :
                D3D12_RESOURCE_STATE_PRESENT;

            if (before != D3D12_RESOURCE_STATE_COPY_DEST)
            {
                auto barrier = Transition(backBuffer.Get(), before, D3D12_RESOURCE_STATE_COPY_DEST);
                commandList->ResourceBarrier(1, &barrier);
                before = D3D12_RESOURCE_STATE_COPY_DEST;
            }

            D3D12_TEXTURE_COPY_LOCATION dstLoc{};
            dstLoc.pResource = backBuffer.Get();
            dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            dstLoc.SubresourceIndex = 0;

            D3D12_TEXTURE_COPY_LOCATION srcLoc{};
            srcLoc.pResource = src->resource.Get();
            srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            srcLoc.SubresourceIndex = 0;

            commandList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);

            auto presentBarrier = Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);
            commandList->ResourceBarrier(1, &presentBarrier);
            if (composition.backBufferStates.size() > backBufferIndex)
            {
                composition.backBufferStates[backBufferIndex] = D3D12_RESOURCE_STATE_PRESENT;
            }

            if (!executeCommandList(error))
            {
                return false;
            }

            const HRESULT presentHr = composition.swapChain->Present(0, 0);
            if (FAILED(presentHr))
            {
                if (error) { *error = Hr("composition swapchain Present", presentHr); }
                return false;
            }

            const HRESULT commitHr = composition.dcompDevice->Commit();
            if (FAILED(commitHr))
            {
                if (error) { *error = Hr("composition Commit", commitHr); }
                return false;
            }

            ++gpuStats.compositionFrames;
            ++gpuStats.zeroCopyFrames;
            ++gpuStats.gpuCompositedFrames;
            return true;
        }

    };

    Dx12Device::Dx12Device() : impl_(std::make_unique<Impl>()) {}
    Dx12Device::~Dx12Device() { shutdown(); }

    bool Dx12Device::initialize(DeviceDesc d, std::string* e)
    {
        if (impl_->isInitialized)
        {
            if (e) { *e = "DX12 RHI device already initialized."; }
            return false;
        }

        impl_->desc = std::move(d);
        impl_->registry.reset();
        impl_->deviceStats = {};
        impl_->gpuStats = {};

        if (!impl_->initializeFactory(e)) { return false; }
        if (!impl_->chooseAdapter(e)) { return false; }
        if (!impl_->initializeDevice(e)) { return false; }

        impl_->isInitialized = true;
        impl_->insideFrame = false;
        impl_->frameIndex = 0;
        return true;
    }

    void Dx12Device::shutdown()
    {
        if (!impl_)
        {
            return;
        }

        impl_->waitIdle();
        impl_->resetComposition();
        impl_->readbackCache.reset();

        for (auto& kv : impl_->buffers)
        {
            if (kv.second.resource && kv.second.mapped)
            {
                kv.second.resource->Unmap(0, nullptr);
            }
        }
        impl_->buffers.clear();
        impl_->textures.clear();
        impl_->pipelines.clear();
        impl_->uploadArena.shutdown();

        if (impl_->fenceEvent)
        {
            CloseHandle(impl_->fenceEvent);
            impl_->fenceEvent = nullptr;
        }

        impl_->commandList.Reset();
        impl_->allocator.Reset();
        impl_->graphicsQueue.Reset();
        impl_->fence.Reset();
        impl_->rtvHeap.heap.Reset();
        impl_->dsvHeap.heap.Reset();
        impl_->srvHeap.heap.Reset();
        impl_->device.Reset();
        impl_->adapter.Reset();
        impl_->factory.Reset();
        impl_->registry.reset();
        impl_->isInitialized = false;
        impl_->insideFrame = false;
    }

    bool Dx12Device::beginFrame(U64 frame, float, std::string* e)
    {
        if (!impl_->isInitialized)
        {
            if (e) { *e = "DX12 beginFrame before initialize."; }
            return false;
        }
        if (impl_->insideFrame)
        {
            if (e) { *e = "DX12 beginFrame while frame already active."; }
            return false;
        }
        impl_->insideFrame = true;
        impl_->frameIndex = frame;
        impl_->uploadArena.reset();
        return true;
    }

    bool Dx12Device::submit(SubmitInfo info, std::string* e)
    {
        if (!impl_->isInitialized || !impl_->insideFrame)
        {
            if (e) { *e = "DX12 submit outside frame."; }
            return false;
        }

        if (!info.list)
        {
            if (e) { *e = "DX12 submit missing command list."; }
            return false;
        }

        if (!info.list->validate(impl_->registry, e))
        {
            ++impl_->deviceStats.validationErrors;
            return false;
        }

        if (!impl_->resetCommandList(e))
        {
            return false;
        }

        for (const auto& command : info.list->commands())
        {
            if (auto* marker = std::get_if<CmdMarker>(&command))
            {
                (void)marker;
                continue;
            }

            if (auto* barrier = std::get_if<CmdBarrier>(&command))
            {
                for (const auto& b : barrier->buffers)
                {
                    if (!impl_->ensureBuffer(b.buffer, e)) { return false; }
                    impl_->transition(*impl_->native(b.buffer), ToState(b.after));
                }
                for (const auto& t : barrier->textures)
                {
                    if (!impl_->ensureTexture(t.texture, e)) { return false; }
                    impl_->transition(*impl_->native(t.texture), ToState(t.after));
                }
                continue;
            }

            if (auto* begin = std::get_if<CmdBeginPass>(&command))
            {
                std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvs;
                rtvs.reserve(begin->desc.colors.size());
                for (const auto& color : begin->desc.colors)
                {
                    if (!impl_->ensureTexture(color.texture, e)) { return false; }
                    auto* native = impl_->native(color.texture);
                    if (!native || !native->hasRtv)
                    {
                        if (e) { *e = "DX12 render pass color attachment has no RTV."; }
                        return false;
                    }
                    impl_->transition(*native, D3D12_RESOURCE_STATE_RENDER_TARGET);
                    rtvs.push_back(native->rtv.cpu);

                    if (color.load == LoadOp::Clear)
                    {
                        const float c[4] = {color.clear.color.r, color.clear.color.g, color.clear.color.b, color.clear.color.a};
                        impl_->commandList->ClearRenderTargetView(native->rtv.cpu, c, 0, nullptr);
                    }
                }

                D3D12_CPU_DESCRIPTOR_HANDLE* dsvPtr = nullptr;
                D3D12_CPU_DESCRIPTOR_HANDLE dsv{};
                if (begin->desc.depth)
                {
                    if (!impl_->ensureTexture(begin->desc.depth->texture, e)) { return false; }
                    auto* native = impl_->native(begin->desc.depth->texture);
                    if (!native || !native->hasDsv)
                    {
                        if (e) { *e = "DX12 render pass depth attachment has no DSV."; }
                        return false;
                    }
                    impl_->transition(*native, D3D12_RESOURCE_STATE_DEPTH_WRITE);
                    dsv = native->dsv.cpu;
                    dsvPtr = &dsv;
                    if (begin->desc.depth->load == LoadOp::Clear)
                    {
                        impl_->commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, begin->desc.depth->clearDepth, begin->desc.depth->clearStencil, 0, nullptr);
                    }
                }

                if (!rtvs.empty() || dsvPtr)
                {
                    impl_->commandList->OMSetRenderTargets(static_cast<UINT>(rtvs.size()), rtvs.empty() ? nullptr : rtvs.data(), FALSE, dsvPtr);
                }
                continue;
            }

            if (std::holds_alternative<CmdEndPass>(command))
            {
                continue;
            }

            if (auto* vp = std::get_if<CmdViewport>(&command))
            {
                D3D12_VIEWPORT dxVp{};
                dxVp.TopLeftX = vp->viewport.x;
                dxVp.TopLeftY = vp->viewport.y;
                dxVp.Width = vp->viewport.width;
                dxVp.Height = vp->viewport.height;
                dxVp.MinDepth = vp->viewport.minDepth;
                dxVp.MaxDepth = vp->viewport.maxDepth;
                impl_->commandList->RSSetViewports(1, &dxVp);
                continue;
            }

            if (auto* sc = std::get_if<CmdScissor>(&command))
            {
                D3D12_RECT r{};
                r.left = sc->rect.x;
                r.top = sc->rect.y;
                r.right = sc->rect.x + static_cast<LONG>(sc->rect.width);
                r.bottom = sc->rect.y + static_cast<LONG>(sc->rect.height);
                impl_->commandList->RSSetScissorRects(1, &r);
                continue;
            }

            if (auto* vb = std::get_if<CmdVertexBuffer>(&command))
            {
                if (!impl_->ensureBuffer(vb->buffer, e)) { return false; }
                auto* native = impl_->native(vb->buffer);
                impl_->transition(*native, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
                D3D12_VERTEX_BUFFER_VIEW view{};
                view.BufferLocation = native->resource->GetGPUVirtualAddress() + vb->offset;
                view.SizeInBytes = static_cast<UINT>(native->desc.size - vb->offset);
                view.StrideInBytes = native->desc.stride;
                impl_->commandList->IASetVertexBuffers(vb->slot, 1, &view);
                continue;
            }

            if (auto* ib = std::get_if<CmdIndexBuffer>(&command))
            {
                if (!impl_->ensureBuffer(ib->buffer, e)) { return false; }
                auto* native = impl_->native(ib->buffer);
                impl_->transition(*native, D3D12_RESOURCE_STATE_INDEX_BUFFER);
                D3D12_INDEX_BUFFER_VIEW view{};
                view.BufferLocation = native->resource->GetGPUVirtualAddress() + ib->offset;
                view.SizeInBytes = static_cast<UINT>(native->desc.size - ib->offset);
                view.Format = ib->format == IndexFormat::UInt16 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
                impl_->commandList->IASetIndexBuffer(&view);
                continue;
            }

            if (auto* copy = std::get_if<CmdCopyBuffer>(&command))
            {
                if (!impl_->ensureBuffer(copy->src, e) || !impl_->ensureBuffer(copy->dst, e)) { return false; }
                auto* src = impl_->native(copy->src);
                auto* dst = impl_->native(copy->dst);
                impl_->transition(*src, D3D12_RESOURCE_STATE_COPY_SOURCE);
                impl_->transition(*dst, D3D12_RESOURCE_STATE_COPY_DEST);
                impl_->commandList->CopyBufferRegion(dst->resource.Get(), copy->dstOffset, src->resource.Get(), copy->srcOffset, copy->size);
                continue;
            }

            if (auto* pipeline = std::get_if<CmdPipeline>(&command))
            {
                if (!impl_->ensurePipeline(pipeline->pipeline, e)) { return false; }
                auto* native = impl_->native(pipeline->pipeline);
                if (!native || !native->state || !native->rootSignature)
                {
                    if (e) { *e = "DX12 pipeline command resolved to missing native PSO."; }
                    return false;
                }
                impl_->commandList->SetGraphicsRootSignature(native->rootSignature.Get());
                impl_->commandList->SetPipelineState(native->state.Get());
                impl_->commandList->IASetPrimitiveTopology(native->topology);
                const auto identity = IdentityWvp();
                impl_->commandList->SetGraphicsRoot32BitConstants(0, 16, identity.data(), 0);
                ++impl_->gpuStats.wvpConstantsUploaded;
                continue;
            }

            if (auto* wvp = std::get_if<CmdWvpConstants>(&command))
            {
                impl_->commandList->SetGraphicsRoot32BitConstants(0, 16, wvp->value.data(), 0);
                ++impl_->gpuStats.wvpConstantsUploaded;
                continue;
            }

            if (auto* draw = std::get_if<CmdDraw>(&command))
            {
                impl_->commandList->DrawInstanced(draw->vertices, draw->instances, draw->firstVertex, draw->firstInstance);
                ++impl_->gpuStats.drawCallsExecuted;
                continue;
            }

            if (auto* drawIndexed = std::get_if<CmdDrawIndexed>(&command))
            {
                impl_->commandList->DrawIndexedInstanced(
                    drawIndexed->indices,
                    drawIndexed->instances,
                    drawIndexed->firstIndex,
                    drawIndexed->vertexOffset,
                    drawIndexed->firstInstance);
                ++impl_->gpuStats.drawCallsExecuted;
                continue;
            }

            if (std::holds_alternative<CmdDispatch>(command))
            {
                // Compute root signatures/pipelines are a later RHI milestone.
                continue;
            }
        }

        if (!impl_->executeCommandList(e))
        {
            return false;
        }

        ++impl_->deviceStats.submissions;
        ++impl_->deviceStats.commandLists;
        return true;
    }

    bool Dx12Device::endFrame(std::string* e)
    {
        if (!impl_->insideFrame)
        {
            if (e) { *e = "DX12 endFrame without active frame."; }
            return false;
        }
        impl_->insideFrame = false;
        return true;
    }

    void Dx12Device::waitIdle()
    {
        impl_->waitIdle();
    }

    Registry& Dx12Device::resources()
    {
        return impl_->registry;
    }

    const Registry& Dx12Device::resources() const
    {
        return impl_->registry;
    }

    Stats Dx12Device::stats() const
    {
        auto s = impl_->registry.stats();
        s.submissions += impl_->deviceStats.submissions;
        s.commandLists += impl_->deviceStats.commandLists;
        s.graphPasses += impl_->deviceStats.graphPasses;
        s.validationErrors += impl_->deviceStats.validationErrors;
        return s;
    }

    bool Dx12Device::initialized() const
    {
        return impl_->isInitialized;
    }

    Backend Dx12Device::backend() const
    {
        return Backend::Dx12;
    }

    bool Dx12Device::ensureNative(Buffer buffer, std::string* e)
    {
        return impl_->ensureBuffer(buffer, e);
    }

    bool Dx12Device::ensureNative(Texture texture, std::string* e)
    {
        return impl_->ensureTexture(texture, e);
    }

    bool Dx12Device::upload(Buffer dst, const void* data, U64 size, std::string* e)
    {
        if (!impl_->isInitialized)
        {
            if (e) { *e = "DX12 upload before initialize."; }
            return false;
        }
        if (!data || size == 0)
        {
            if (e) { *e = "DX12 upload requires non-empty data."; }
            return false;
        }

        if (!impl_->ensureBuffer(dst, e))
        {
            return false;
        }

        auto* nativeDst = impl_->native(dst);
        if (!nativeDst || !nativeDst->resource)
        {
            if (e) { *e = "DX12 upload destination resolved to missing native buffer."; }
            return false;
        }

        if (nativeDst->mapped && nativeDst->desc.memory == Memory::Upload)
        {
            // ACE-PERF1: Slate-style dynamic geometry stays in persistently mapped
            // upload memory. For the tiny Aquarium viewport mesh, issuing a copy
            // command list and waiting on a fence every frame is architectural
            // self-sabotage wearing a hard hat. UE's Slate path keeps dynamic UI
            // buffers CPU-visible and updates them directly; ACE mirrors that for
            // transient viewport vertices.
            const U64 capacity = nativeDst->desc.size;
            if (size > capacity)
            {
                if (e) { *e = "DX12 mapped upload exceeds destination buffer capacity."; }
                return false;
            }
            std::memcpy(nativeDst->mapped, data, static_cast<std::size_t>(size));
            impl_->gpuStats.mappedUploadBytes += size;
            ++impl_->gpuStats.mappedUploadUpdates;
            return true;
        }

        U64 uploadOffset = 0;
        void* uploadCpu = nullptr;
        if (!impl_->uploadArena.allocate(size, 256, &uploadOffset, &uploadCpu, e))
        {
            return false;
        }

        std::memcpy(uploadCpu, data, static_cast<std::size_t>(size));
        impl_->gpuStats.uploadBytesAllocated += size;
        ++impl_->gpuStats.uploadAllocations;

        if (!impl_->resetCommandList(e))
        {
            return false;
        }

        impl_->transition(*nativeDst, D3D12_RESOURCE_STATE_COPY_DEST);
        impl_->commandList->CopyBufferRegion(nativeDst->resource.Get(), 0, impl_->uploadArena.resource.Get(), uploadOffset, size);

        return impl_->executeCommandList(e);
    }

    bool Dx12Device::clear(Texture target, Color color, std::string* e)
    {
        if (!impl_->isInitialized)
        {
            if (e) { *e = "DX12 clear before initialize."; }
            return false;
        }

        if (!impl_->ensureTexture(target, e))
        {
            return false;
        }

        auto* native = impl_->native(target);
        if (!native || !native->hasRtv)
        {
            if (e) { *e = "DX12 clear requires render-target texture."; }
            return false;
        }

        if (!impl_->resetCommandList(e))
        {
            return false;
        }

        impl_->transition(*native, D3D12_RESOURCE_STATE_RENDER_TARGET);
        const float c[4] = {color.r, color.g, color.b, color.a};
        impl_->commandList->ClearRenderTargetView(native->rtv.cpu, c, 0, nullptr);
        return impl_->executeCommandList(e);
    }

    bool Dx12Device::createOffscreenSceneTargets(Extent2D extent, Texture* color, Texture* depth, std::string* e)
    {
        if (!impl_->isInitialized)
        {
            if (e) { *e = "DX12 offscreen target creation before initialize."; }
            return false;
        }

        TextureDesc colorDesc{};
        colorDesc.name = Name{"AceRhi4_OffscreenSceneColor"};
        colorDesc.extent = {std::max<U32>(1, extent.width), std::max<U32>(1, extent.height), 1};
        colorDesc.format = Format::BGRA8;
        colorDesc.usage = Usage::RenderTarget | Usage::ShaderResource;
        colorDesc.memory = Memory::GpuOnly;
        colorDesc.clear.color = {0.005f, 0.010f, 0.020f, 1.0f};

        TextureDesc depthDesc{};
        depthDesc.name = Name{"AceRhi4_OffscreenSceneDepth"};
        depthDesc.extent = {std::max<U32>(1, extent.width), std::max<U32>(1, extent.height), 1};
        depthDesc.format = Format::D32F;
        depthDesc.usage = Usage::DepthStencil | Usage::ShaderResource;
        depthDesc.memory = Memory::GpuOnly;
        depthDesc.clear.depth = 1.0f;

        auto colorTexture = impl_->registry.create(colorDesc, e);
        if (!colorTexture.valid())
        {
            return false;
        }

        auto depthTexture = impl_->registry.create(depthDesc, e);
        if (!depthTexture.valid())
        {
            return false;
        }

        if (!impl_->ensureTexture(colorTexture, e) || !impl_->ensureTexture(depthTexture, e))
        {
            return false;
        }

        if (color) { *color = colorTexture; }
        if (depth) { *depth = depthTexture; }
        ++impl_->gpuStats.offscreenSceneTargets;
        return true;
    }

    bool Dx12Device::readbackBgra8(Texture source, std::vector<U32>* pixels, Extent2D* extent, std::string* e)
    {
        if (!impl_->isInitialized)
        {
            if (e) { *e = "DX12 readback before initialize."; }
            return false;
        }

        if (!pixels)
        {
            if (e) { *e = "DX12 readback requires output pixel vector."; }
            return false;
        }

        if (!impl_->ensureTexture(source, e))
        {
            return false;
        }

        auto* native = impl_->native(source);
        if (!native || !native->resource)
        {
            if (e) { *e = "DX12 readback missing native source texture."; }
            return false;
        }

        if (native->desc.format != Format::BGRA8 && native->desc.format != Format::RGBA8)
        {
            if (e) { *e = "DX12 readback currently supports BGRA8/RGBA8 only."; }
            return false;
        }

        const auto resourceDesc = native->resource->GetDesc();
        if (!impl_->readbackCache.matches(resourceDesc))
        {
            impl_->readbackCache.reset();
            impl_->device->GetCopyableFootprints(
                &resourceDesc,
                0,
                1,
                0,
                &impl_->readbackCache.footprint,
                &impl_->readbackCache.rowCount,
                &impl_->readbackCache.rowSizeBytes,
                &impl_->readbackCache.totalBytes);

            auto props = HeapProps(D3D12_HEAP_TYPE_READBACK);
            auto bufferDesc = BufferResourceDesc(impl_->readbackCache.totalBytes);
            const HRESULT hr = impl_->device->CreateCommittedResource(
                &props,
                D3D12_HEAP_FLAG_NONE,
                &bufferDesc,
                D3D12_RESOURCE_STATE_COPY_DEST,
                nullptr,
                IID_PPV_ARGS(&impl_->readbackCache.resource));

            if (FAILED(hr))
            {
                if (e) { *e = Hr("CreateCommittedResource readback cache", hr); }
                impl_->readbackCache.reset();
                return false;
            }

            impl_->readbackCache.extent = {static_cast<U32>(resourceDesc.Width), static_cast<U32>(resourceDesc.Height)};
            impl_->readbackCache.format = resourceDesc.Format;
            ++impl_->gpuStats.readbackBufferResizes;
        }
        else
        {
            ++impl_->gpuStats.readbackBufferReuses;
        }

        if (!impl_->resetCommandList(e))
        {
            return false;
        }

        impl_->transition(*native, D3D12_RESOURCE_STATE_COPY_SOURCE);

        D3D12_TEXTURE_COPY_LOCATION srcLoc{};
        srcLoc.pResource = native->resource.Get();
        srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        srcLoc.SubresourceIndex = 0;

        D3D12_TEXTURE_COPY_LOCATION dstLoc{};
        dstLoc.pResource = impl_->readbackCache.resource.Get();
        dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        dstLoc.PlacedFootprint = impl_->readbackCache.footprint;

        impl_->commandList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);

        if (!impl_->executeCommandList(e))
        {
            return false;
        }

        void* mapped = nullptr;
        D3D12_RANGE readRange{0, static_cast<SIZE_T>(impl_->readbackCache.totalBytes)};
        const HRESULT mapHr = impl_->readbackCache.resource->Map(0, &readRange, &mapped);
        if (FAILED(mapHr))
        {
            if (e) { *e = Hr("Map readback", mapHr); }
            return false;
        }

        const U32 width = static_cast<U32>(native->desc.extent.width);
        const U32 height = static_cast<U32>(native->desc.extent.height);
        pixels->assign(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), 0U);

        const auto* srcBytes = static_cast<const U8*>(mapped) + impl_->readbackCache.footprint.Offset;
        const U32 copyBytesPerRow = static_cast<U32>(std::min<UINT64>(impl_->readbackCache.rowSizeBytes, static_cast<UINT64>(width) * 4ULL));
        for (U32 y = 0; y < height; ++y)
        {
            const auto* srcRow = srcBytes + static_cast<std::size_t>(y) * impl_->readbackCache.footprint.Footprint.RowPitch;
            auto* dstRow = reinterpret_cast<U8*>(pixels->data() + static_cast<std::size_t>(y) * width);
            std::memcpy(dstRow, srcRow, copyBytesPerRow);
        }

        D3D12_RANGE noWrite{0, 0};
        impl_->readbackCache.resource->Unmap(0, &noWrite);

        if (extent)
        {
            *extent = {width, height};
        }

        ++impl_->gpuStats.readbackFrames;
        impl_->gpuStats.readbackBytes += static_cast<U64>(width) * static_cast<U64>(height) * 4ULL;
        return true;
    }


    bool Dx12Device::submitAndReadbackBgra8(SubmitInfo info, Texture source, std::vector<U32>* pixels, Extent2D* extent, std::string* e)
    {
        if (!impl_->isInitialized || !impl_->insideFrame)
        {
            if (e) { *e = "DX12 combined submit/readback outside frame."; }
            return false;
        }

        if (!info.list)
        {
            if (e) { *e = "DX12 combined submit/readback missing command list."; }
            return false;
        }

        if (!pixels)
        {
            if (e) { *e = "DX12 combined submit/readback requires output pixel vector."; }
            return false;
        }

        if (!info.list->validate(impl_->registry, e))
        {
            ++impl_->deviceStats.validationErrors;
            return false;
        }

        if (!impl_->ensureTexture(source, e))
        {
            return false;
        }

        auto* native = impl_->native(source);
        if (!native || !native->resource)
        {
            if (e) { *e = "DX12 combined submit/readback missing native source texture."; }
            return false;
        }

        if (native->desc.format != Format::BGRA8 && native->desc.format != Format::RGBA8)
        {
            if (e) { *e = "DX12 combined submit/readback currently supports BGRA8/RGBA8 only."; }
            return false;
        }

        const auto resourceDesc = native->resource->GetDesc();
        if (!impl_->readbackCache.matches(resourceDesc))
        {
            impl_->readbackCache.reset();
            impl_->device->GetCopyableFootprints(
                &resourceDesc,
                0,
                1,
                0,
                &impl_->readbackCache.footprint,
                &impl_->readbackCache.rowCount,
                &impl_->readbackCache.rowSizeBytes,
                &impl_->readbackCache.totalBytes);

            auto props = HeapProps(D3D12_HEAP_TYPE_READBACK);
            auto bufferDesc = BufferResourceDesc(impl_->readbackCache.totalBytes);
            const HRESULT hr = impl_->device->CreateCommittedResource(
                &props,
                D3D12_HEAP_FLAG_NONE,
                &bufferDesc,
                D3D12_RESOURCE_STATE_COPY_DEST,
                nullptr,
                IID_PPV_ARGS(&impl_->readbackCache.resource));

            if (FAILED(hr))
            {
                if (e) { *e = Hr("CreateCommittedResource combined readback cache", hr); }
                impl_->readbackCache.reset();
                return false;
            }

            impl_->readbackCache.extent = {static_cast<U32>(resourceDesc.Width), static_cast<U32>(resourceDesc.Height)};
            impl_->readbackCache.format = resourceDesc.Format;
            ++impl_->gpuStats.readbackBufferResizes;
        }
        else
        {
            ++impl_->gpuStats.readbackBufferReuses;
        }

        if (!impl_->resetCommandList(e))
        {
            return false;
        }

        // ACE-PERF1R1: record scene draw + readback copy into one command list.
        // The previous DX12_READBACK hot path waited for the render submit, then
        // submitted and waited again for the readback copy. UE-style viewport
        // layering only pays its synchronization bill once per composed viewport frame.
        for (const auto& command : info.list->commands())
        {
            if (auto* marker = std::get_if<CmdMarker>(&command))
            {
                (void)marker;
                continue;
            }

            if (auto* barrier = std::get_if<CmdBarrier>(&command))
            {
                for (const auto& b : barrier->buffers)
                {
                    if (!impl_->ensureBuffer(b.buffer, e)) { return false; }
                    impl_->transition(*impl_->native(b.buffer), ToState(b.after));
                }
                for (const auto& t : barrier->textures)
                {
                    if (!impl_->ensureTexture(t.texture, e)) { return false; }
                    impl_->transition(*impl_->native(t.texture), ToState(t.after));
                }
                continue;
            }

            if (auto* begin = std::get_if<CmdBeginPass>(&command))
            {
                std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvs;
                rtvs.reserve(begin->desc.colors.size());
                for (const auto& color : begin->desc.colors)
                {
                    if (!impl_->ensureTexture(color.texture, e)) { return false; }
                    auto* native = impl_->native(color.texture);
                    if (!native || !native->hasRtv)
                    {
                        if (e) { *e = "DX12 render pass color attachment has no RTV."; }
                        return false;
                    }
                    impl_->transition(*native, D3D12_RESOURCE_STATE_RENDER_TARGET);
                    rtvs.push_back(native->rtv.cpu);

                    if (color.load == LoadOp::Clear)
                    {
                        const float c[4] = {color.clear.color.r, color.clear.color.g, color.clear.color.b, color.clear.color.a};
                        impl_->commandList->ClearRenderTargetView(native->rtv.cpu, c, 0, nullptr);
                    }
                }

                D3D12_CPU_DESCRIPTOR_HANDLE* dsvPtr = nullptr;
                D3D12_CPU_DESCRIPTOR_HANDLE dsv{};
                if (begin->desc.depth)
                {
                    if (!impl_->ensureTexture(begin->desc.depth->texture, e)) { return false; }
                    auto* native = impl_->native(begin->desc.depth->texture);
                    if (!native || !native->hasDsv)
                    {
                        if (e) { *e = "DX12 render pass depth attachment has no DSV."; }
                        return false;
                    }
                    impl_->transition(*native, D3D12_RESOURCE_STATE_DEPTH_WRITE);
                    dsv = native->dsv.cpu;
                    dsvPtr = &dsv;
                    if (begin->desc.depth->load == LoadOp::Clear)
                    {
                        impl_->commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, begin->desc.depth->clearDepth, begin->desc.depth->clearStencil, 0, nullptr);
                    }
                }

                if (!rtvs.empty() || dsvPtr)
                {
                    impl_->commandList->OMSetRenderTargets(static_cast<UINT>(rtvs.size()), rtvs.empty() ? nullptr : rtvs.data(), FALSE, dsvPtr);
                }
                continue;
            }

            if (std::holds_alternative<CmdEndPass>(command))
            {
                continue;
            }

            if (auto* vp = std::get_if<CmdViewport>(&command))
            {
                D3D12_VIEWPORT dxVp{};
                dxVp.TopLeftX = vp->viewport.x;
                dxVp.TopLeftY = vp->viewport.y;
                dxVp.Width = vp->viewport.width;
                dxVp.Height = vp->viewport.height;
                dxVp.MinDepth = vp->viewport.minDepth;
                dxVp.MaxDepth = vp->viewport.maxDepth;
                impl_->commandList->RSSetViewports(1, &dxVp);
                continue;
            }

            if (auto* sc = std::get_if<CmdScissor>(&command))
            {
                D3D12_RECT r{};
                r.left = sc->rect.x;
                r.top = sc->rect.y;
                r.right = sc->rect.x + static_cast<LONG>(sc->rect.width);
                r.bottom = sc->rect.y + static_cast<LONG>(sc->rect.height);
                impl_->commandList->RSSetScissorRects(1, &r);
                continue;
            }

            if (auto* vb = std::get_if<CmdVertexBuffer>(&command))
            {
                if (!impl_->ensureBuffer(vb->buffer, e)) { return false; }
                auto* native = impl_->native(vb->buffer);
                impl_->transition(*native, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
                D3D12_VERTEX_BUFFER_VIEW view{};
                view.BufferLocation = native->resource->GetGPUVirtualAddress() + vb->offset;
                view.SizeInBytes = static_cast<UINT>(native->desc.size - vb->offset);
                view.StrideInBytes = native->desc.stride;
                impl_->commandList->IASetVertexBuffers(vb->slot, 1, &view);
                continue;
            }

            if (auto* ib = std::get_if<CmdIndexBuffer>(&command))
            {
                if (!impl_->ensureBuffer(ib->buffer, e)) { return false; }
                auto* native = impl_->native(ib->buffer);
                impl_->transition(*native, D3D12_RESOURCE_STATE_INDEX_BUFFER);
                D3D12_INDEX_BUFFER_VIEW view{};
                view.BufferLocation = native->resource->GetGPUVirtualAddress() + ib->offset;
                view.SizeInBytes = static_cast<UINT>(native->desc.size - ib->offset);
                view.Format = ib->format == IndexFormat::UInt16 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
                impl_->commandList->IASetIndexBuffer(&view);
                continue;
            }

            if (auto* copy = std::get_if<CmdCopyBuffer>(&command))
            {
                if (!impl_->ensureBuffer(copy->src, e) || !impl_->ensureBuffer(copy->dst, e)) { return false; }
                auto* src = impl_->native(copy->src);
                auto* dst = impl_->native(copy->dst);
                impl_->transition(*src, D3D12_RESOURCE_STATE_COPY_SOURCE);
                impl_->transition(*dst, D3D12_RESOURCE_STATE_COPY_DEST);
                impl_->commandList->CopyBufferRegion(dst->resource.Get(), copy->dstOffset, src->resource.Get(), copy->srcOffset, copy->size);
                continue;
            }

            if (auto* pipeline = std::get_if<CmdPipeline>(&command))
            {
                if (!impl_->ensurePipeline(pipeline->pipeline, e)) { return false; }
                auto* native = impl_->native(pipeline->pipeline);
                if (!native || !native->state || !native->rootSignature)
                {
                    if (e) { *e = "DX12 pipeline command resolved to missing native PSO."; }
                    return false;
                }
                impl_->commandList->SetGraphicsRootSignature(native->rootSignature.Get());
                impl_->commandList->SetPipelineState(native->state.Get());
                impl_->commandList->IASetPrimitiveTopology(native->topology);
                const auto identity = IdentityWvp();
                impl_->commandList->SetGraphicsRoot32BitConstants(0, 16, identity.data(), 0);
                ++impl_->gpuStats.wvpConstantsUploaded;
                continue;
            }

            if (auto* wvp = std::get_if<CmdWvpConstants>(&command))
            {
                impl_->commandList->SetGraphicsRoot32BitConstants(0, 16, wvp->value.data(), 0);
                ++impl_->gpuStats.wvpConstantsUploaded;
                continue;
            }

            if (auto* draw = std::get_if<CmdDraw>(&command))
            {
                impl_->commandList->DrawInstanced(draw->vertices, draw->instances, draw->firstVertex, draw->firstInstance);
                ++impl_->gpuStats.drawCallsExecuted;
                continue;
            }

            if (auto* drawIndexed = std::get_if<CmdDrawIndexed>(&command))
            {
                impl_->commandList->DrawIndexedInstanced(
                    drawIndexed->indices,
                    drawIndexed->instances,
                    drawIndexed->firstIndex,
                    drawIndexed->vertexOffset,
                    drawIndexed->firstInstance);
                ++impl_->gpuStats.drawCallsExecuted;
                continue;
            }

            if (std::holds_alternative<CmdDispatch>(command))
            {
                // Compute root signatures/pipelines are a later RHI milestone.
                continue;
            }
        }

        impl_->transition(*native, D3D12_RESOURCE_STATE_COPY_SOURCE);

        D3D12_TEXTURE_COPY_LOCATION srcLoc{};
        srcLoc.pResource = native->resource.Get();
        srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        srcLoc.SubresourceIndex = 0;

        D3D12_TEXTURE_COPY_LOCATION dstLoc{};
        dstLoc.pResource = impl_->readbackCache.resource.Get();
        dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        dstLoc.PlacedFootprint = impl_->readbackCache.footprint;

        impl_->commandList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);

        if (!impl_->executeCommandList(e))
        {
            return false;
        }

        ++impl_->deviceStats.submissions;
        ++impl_->deviceStats.commandLists;

        void* mapped = nullptr;
        D3D12_RANGE readRange{0, static_cast<SIZE_T>(impl_->readbackCache.totalBytes)};
        const HRESULT mapHr = impl_->readbackCache.resource->Map(0, &readRange, &mapped);
        if (FAILED(mapHr))
        {
            if (e) { *e = Hr("Map combined readback", mapHr); }
            return false;
        }

        const U32 width = static_cast<U32>(native->desc.extent.width);
        const U32 height = static_cast<U32>(native->desc.extent.height);
        pixels->assign(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), 0U);

        const auto* srcBytes = static_cast<const U8*>(mapped) + impl_->readbackCache.footprint.Offset;
        const U32 copyBytesPerRow = static_cast<U32>(std::min<UINT64>(impl_->readbackCache.rowSizeBytes, static_cast<UINT64>(width) * 4ULL));
        for (U32 y = 0; y < height; ++y)
        {
            const auto* srcRow = srcBytes + static_cast<std::size_t>(y) * impl_->readbackCache.footprint.Footprint.RowPitch;
            auto* dstRow = reinterpret_cast<U8*>(pixels->data() + static_cast<std::size_t>(y) * width);
            std::memcpy(dstRow, srcRow, copyBytesPerRow);
        }

        D3D12_RANGE noWrite{0, 0};
        impl_->readbackCache.resource->Unmap(0, &noWrite);

        if (extent)
        {
            *extent = {width, height};
        }

        const U64 bytes = static_cast<U64>(width) * static_cast<U64>(height) * 4ULL;
        ++impl_->gpuStats.readbackFrames;
        impl_->gpuStats.readbackBytes += bytes;
        ++impl_->gpuStats.combinedRenderReadbackFrames;
        impl_->gpuStats.combinedRenderReadbackBytes += bytes;
        return true;
    }


    bool Dx12Device::submitAndPresentBgra8ToComposition(SubmitInfo info, Texture source, void* hwnd, float left, float top, Extent2D extent, std::string* e)
    {
        if (!impl_->isInitialized || !impl_->insideFrame)
        {
            if (e) { *e = "DX12 combined submit/present outside frame."; }
            return false;
        }

        if (!info.list)
        {
            if (e) { *e = "DX12 combined submit/present missing command list."; }
            return false;
        }

        if (!info.list->validate(impl_->registry, e))
        {
            ++impl_->deviceStats.validationErrors;
            return false;
        }

        if (!impl_->ensureTexture(source, e))
        {
            return false;
        }

        auto* src = impl_->native(source);
        if (!src || !src->resource)
        {
            if (e) { *e = "DX12 combined submit/present missing native source texture."; }
            return false;
        }

        if (src->desc.format != Format::BGRA8)
        {
            if (e) { *e = "DX12 combined submit/present currently requires BGRA8 SceneColor."; }
            return false;
        }

        if (!impl_->ensureComposition(static_cast<HWND>(hwnd), extent, left, top, e))
        {
            return false;
        }

        const UINT backBufferIndex = impl_->composition.swapChain->GetCurrentBackBufferIndex();
        ComPtr<ID3D12Resource> backBuffer;
        const HRESULT bufferHr = impl_->composition.swapChain->GetBuffer(backBufferIndex, IID_PPV_ARGS(&backBuffer));
        if (FAILED(bufferHr))
        {
            if (e) { *e = Hr("IDXGISwapChain::GetBuffer", bufferHr); }
            return false;
        }

        if (!impl_->resetCommandList(e))
        {
            return false;
        }

        // ACE-PERF2: record scene render, viewport-local UI overlay, and the
        // DirectComposition swapchain copy into one GPU command list. The old
        // path submitted the scene, waited, then submitted a second copy/present
        // command list. UE/Slate-style viewport layers should be one composed
        // layer transaction, not two synchronized errands to the GPU DMV.
        for (const auto& command : info.list->commands())
        {
            if (auto* marker = std::get_if<CmdMarker>(&command))
            {
                (void)marker;
                continue;
            }

            if (auto* barrier = std::get_if<CmdBarrier>(&command))
            {
                for (const auto& b : barrier->buffers)
                {
                    if (!impl_->ensureBuffer(b.buffer, e)) { return false; }
                    impl_->transition(*impl_->native(b.buffer), ToState(b.after));
                }
                for (const auto& t : barrier->textures)
                {
                    if (!impl_->ensureTexture(t.texture, e)) { return false; }
                    impl_->transition(*impl_->native(t.texture), ToState(t.after));
                }
                continue;
            }

            if (auto* begin = std::get_if<CmdBeginPass>(&command))
            {
                std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvs;
                rtvs.reserve(begin->desc.colors.size());
                for (const auto& color : begin->desc.colors)
                {
                    if (!impl_->ensureTexture(color.texture, e)) { return false; }
                    auto* native = impl_->native(color.texture);
                    if (!native || !native->hasRtv)
                    {
                        if (e) { *e = "DX12 render pass color attachment has no RTV."; }
                        return false;
                    }
                    impl_->transition(*native, D3D12_RESOURCE_STATE_RENDER_TARGET);
                    rtvs.push_back(native->rtv.cpu);

                    if (color.load == LoadOp::Clear)
                    {
                        const float c[4] = {color.clear.color.r, color.clear.color.g, color.clear.color.b, color.clear.color.a};
                        impl_->commandList->ClearRenderTargetView(native->rtv.cpu, c, 0, nullptr);
                    }
                }

                D3D12_CPU_DESCRIPTOR_HANDLE* dsvPtr = nullptr;
                D3D12_CPU_DESCRIPTOR_HANDLE dsv{};
                if (begin->desc.depth)
                {
                    if (!impl_->ensureTexture(begin->desc.depth->texture, e)) { return false; }
                    auto* native = impl_->native(begin->desc.depth->texture);
                    if (!native || !native->hasDsv)
                    {
                        if (e) { *e = "DX12 render pass depth attachment has no DSV."; }
                        return false;
                    }
                    impl_->transition(*native, D3D12_RESOURCE_STATE_DEPTH_WRITE);
                    dsv = native->dsv.cpu;
                    dsvPtr = &dsv;
                    if (begin->desc.depth->load == LoadOp::Clear)
                    {
                        impl_->commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, begin->desc.depth->clearDepth, begin->desc.depth->clearStencil, 0, nullptr);
                    }
                }

                if (!rtvs.empty() || dsvPtr)
                {
                    impl_->commandList->OMSetRenderTargets(static_cast<UINT>(rtvs.size()), rtvs.empty() ? nullptr : rtvs.data(), FALSE, dsvPtr);
                }
                continue;
            }

            if (std::holds_alternative<CmdEndPass>(command))
            {
                continue;
            }

            if (auto* vp = std::get_if<CmdViewport>(&command))
            {
                D3D12_VIEWPORT dxVp{};
                dxVp.TopLeftX = vp->viewport.x;
                dxVp.TopLeftY = vp->viewport.y;
                dxVp.Width = vp->viewport.width;
                dxVp.Height = vp->viewport.height;
                dxVp.MinDepth = vp->viewport.minDepth;
                dxVp.MaxDepth = vp->viewport.maxDepth;
                impl_->commandList->RSSetViewports(1, &dxVp);
                continue;
            }

            if (auto* sc = std::get_if<CmdScissor>(&command))
            {
                D3D12_RECT r{};
                r.left = sc->rect.x;
                r.top = sc->rect.y;
                r.right = sc->rect.x + static_cast<LONG>(sc->rect.width);
                r.bottom = sc->rect.y + static_cast<LONG>(sc->rect.height);
                impl_->commandList->RSSetScissorRects(1, &r);
                continue;
            }

            if (auto* vb = std::get_if<CmdVertexBuffer>(&command))
            {
                if (!impl_->ensureBuffer(vb->buffer, e)) { return false; }
                auto* native = impl_->native(vb->buffer);
                impl_->transition(*native, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
                D3D12_VERTEX_BUFFER_VIEW view{};
                view.BufferLocation = native->resource->GetGPUVirtualAddress() + vb->offset;
                view.SizeInBytes = static_cast<UINT>(native->desc.size - vb->offset);
                view.StrideInBytes = native->desc.stride;
                impl_->commandList->IASetVertexBuffers(vb->slot, 1, &view);
                continue;
            }

            if (auto* ib = std::get_if<CmdIndexBuffer>(&command))
            {
                if (!impl_->ensureBuffer(ib->buffer, e)) { return false; }
                auto* native = impl_->native(ib->buffer);
                impl_->transition(*native, D3D12_RESOURCE_STATE_INDEX_BUFFER);
                D3D12_INDEX_BUFFER_VIEW view{};
                view.BufferLocation = native->resource->GetGPUVirtualAddress() + ib->offset;
                view.SizeInBytes = static_cast<UINT>(native->desc.size - ib->offset);
                view.Format = ib->format == IndexFormat::UInt16 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
                impl_->commandList->IASetIndexBuffer(&view);
                continue;
            }

            if (auto* copy = std::get_if<CmdCopyBuffer>(&command))
            {
                if (!impl_->ensureBuffer(copy->src, e) || !impl_->ensureBuffer(copy->dst, e)) { return false; }
                auto* src = impl_->native(copy->src);
                auto* dst = impl_->native(copy->dst);
                impl_->transition(*src, D3D12_RESOURCE_STATE_COPY_SOURCE);
                impl_->transition(*dst, D3D12_RESOURCE_STATE_COPY_DEST);
                impl_->commandList->CopyBufferRegion(dst->resource.Get(), copy->dstOffset, src->resource.Get(), copy->srcOffset, copy->size);
                continue;
            }

            if (auto* pipeline = std::get_if<CmdPipeline>(&command))
            {
                if (!impl_->ensurePipeline(pipeline->pipeline, e)) { return false; }
                auto* native = impl_->native(pipeline->pipeline);
                if (!native || !native->state || !native->rootSignature)
                {
                    if (e) { *e = "DX12 pipeline command resolved to missing native PSO."; }
                    return false;
                }
                impl_->commandList->SetGraphicsRootSignature(native->rootSignature.Get());
                impl_->commandList->SetPipelineState(native->state.Get());
                impl_->commandList->IASetPrimitiveTopology(native->topology);
                const auto identity = IdentityWvp();
                impl_->commandList->SetGraphicsRoot32BitConstants(0, 16, identity.data(), 0);
                ++impl_->gpuStats.wvpConstantsUploaded;
                continue;
            }

            if (auto* wvp = std::get_if<CmdWvpConstants>(&command))
            {
                impl_->commandList->SetGraphicsRoot32BitConstants(0, 16, wvp->value.data(), 0);
                ++impl_->gpuStats.wvpConstantsUploaded;
                continue;
            }

            if (auto* draw = std::get_if<CmdDraw>(&command))
            {
                impl_->commandList->DrawInstanced(draw->vertices, draw->instances, draw->firstVertex, draw->firstInstance);
                ++impl_->gpuStats.drawCallsExecuted;
                continue;
            }

            if (auto* drawIndexed = std::get_if<CmdDrawIndexed>(&command))
            {
                impl_->commandList->DrawIndexedInstanced(
                    drawIndexed->indices,
                    drawIndexed->instances,
                    drawIndexed->firstIndex,
                    drawIndexed->vertexOffset,
                    drawIndexed->firstInstance);
                ++impl_->gpuStats.drawCallsExecuted;
                continue;
            }

            if (std::holds_alternative<CmdDispatch>(command))
            {
                // Compute root signatures/pipelines are a later RHI milestone.
                continue;
            }
        }

        impl_->transition(*src, D3D12_RESOURCE_STATE_COPY_SOURCE);

        auto before = impl_->composition.backBufferStates.size() > backBufferIndex ?
            impl_->composition.backBufferStates[backBufferIndex] :
            D3D12_RESOURCE_STATE_PRESENT;

        if (before != D3D12_RESOURCE_STATE_COPY_DEST)
        {
            auto barrier = Transition(backBuffer.Get(), before, D3D12_RESOURCE_STATE_COPY_DEST);
            impl_->commandList->ResourceBarrier(1, &barrier);
            before = D3D12_RESOURCE_STATE_COPY_DEST;
        }

        D3D12_TEXTURE_COPY_LOCATION dstLoc{};
        dstLoc.pResource = backBuffer.Get();
        dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dstLoc.SubresourceIndex = 0;

        D3D12_TEXTURE_COPY_LOCATION srcLoc{};
        srcLoc.pResource = src->resource.Get();
        srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        srcLoc.SubresourceIndex = 0;

        impl_->commandList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);

        auto presentBarrier = Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);
        impl_->commandList->ResourceBarrier(1, &presentBarrier);
        if (impl_->composition.backBufferStates.size() > backBufferIndex)
        {
            impl_->composition.backBufferStates[backBufferIndex] = D3D12_RESOURCE_STATE_PRESENT;
        }

        if (!impl_->executeCommandList(e))
        {
            return false;
        }

        const HRESULT presentHr = impl_->composition.swapChain->Present(0, 0);
        if (FAILED(presentHr))
        {
            if (e) { *e = Hr("composition swapchain Present", presentHr); }
            return false;
        }

        const HRESULT commitHr = impl_->composition.dcompDevice->Commit();
        if (FAILED(commitHr))
        {
            if (e) { *e = Hr("composition Commit", commitHr); }
            return false;
        }

        ++impl_->deviceStats.submissions;
        ++impl_->deviceStats.commandLists;
        ++impl_->gpuStats.compositionFrames;
        ++impl_->gpuStats.zeroCopyFrames;
        ++impl_->gpuStats.gpuCompositedFrames;
        ++impl_->gpuStats.combinedGpuCompositionFrames;
        return true;
    }

    bool Dx12Device::presentBgra8ToComposition(Texture source, void* hwnd, float left, float top, Extent2D extent, std::string* e)
    {
        if (!impl_->isInitialized)
        {
            if (e) { *e = "DX12 composition before initialize."; }
            return false;
        }

        return impl_->presentComposition(source, static_cast<HWND>(hwnd), left, top, extent, e);
    }

    void Dx12Device::resetCompositionHost()
    {
        if (!impl_)
        {
            return;
        }

        if (impl_->isInitialized && impl_->composition.ready)
        {
            impl_->waitIdle();
        }
        impl_->resetComposition();
    }

    void Dx12Device::noteGpuViewportComposition(U64 overlayVertices)
    {
        if (!impl_)
        {
            return;
        }
        if (overlayVertices > 0)
        {
            ++impl_->gpuStats.gpuOverlayBakedFrames;
            impl_->gpuStats.gpuOverlayVertices += overlayVertices;
        }
    }

    void Dx12Device::noteD2DTextureBridgeFrame()
    {
        if (impl_)
        {
            ++impl_->gpuStats.d2dTextureBridgeFrames;
        }
    }

    void* Dx12Device::nativeD3D12Device() const
    {
        return impl_ && impl_->device ? impl_->device.Get() : nullptr;
    }

    void* Dx12Device::nativeD3D12GraphicsQueue() const
    {
        return impl_ && impl_->graphicsQueue ? impl_->graphicsQueue.Get() : nullptr;
    }

    void* Dx12Device::nativeD3D12TextureResource(Texture texture)
    {
        if (!impl_ || !texture.valid())
        {
            return nullptr;
        }
        if (!impl_->ensureTexture(texture, nullptr))
        {
            return nullptr;
        }
        auto* native = impl_->native(texture);
        return native && native->resource ? native->resource.Get() : nullptr;
    }

    bool Dx12Device::gpuSmokeTest(std::string* e)
    {
        BufferDesc vertex{};
        vertex.name = Name{"dx12_smoke_vertex_default"};
        vertex.size = 256;
        vertex.stride = 28;
        vertex.usage = Usage::Vertex | Usage::CopyDst;
        vertex.memory = Memory::GpuOnly;
        auto vb = impl_->registry.create(vertex, e);
        if (!vb.valid()) { return false; }

        std::array<float, 21> triangle = {
             0.0f,  0.6f, 0.0f, 1.0f, 0.15f, 0.10f, 1.0f,
             0.6f, -0.6f, 0.0f, 0.10f, 0.85f, 1.0f, 1.0f,
            -0.6f, -0.6f, 0.0f, 1.0f, 0.90f, 0.20f, 1.0f
        };
        if (!upload(vb, triangle.data(), sizeof(triangle), e))
        {
            return false;
        }

        TextureDesc color{};
        color.name = Name{"dx12_smoke_color"};
        color.extent = {256, 256, 1};
        color.format = Format::BGRA8;
        color.usage = Usage::RenderTarget | Usage::ShaderResource;
        color.memory = Memory::GpuOnly;
        color.clear.color = {0.02f, 0.08f, 0.15f, 1.0f};
        auto target = impl_->registry.create(color, e);
        if (!target.valid()) { return false; }

        if (!clear(target, {0.10f, 0.18f, 0.30f, 1.0f}, e))
        {
            return false;
        }

        ShaderDesc vs{};
        vs.name = Name{"dx12_smoke_vs"};
        vs.stage = ShaderStage::Vertex;
        auto vsh = impl_->registry.create(vs, e);
        if (!vsh.valid()) { return false; }

        GraphicsPipelineDesc pipelineDesc{};
        pipelineDesc.name = Name{"dx12_smoke_basic_color_pso"};
        pipelineDesc.vs = vsh;
        pipelineDesc.topology = Topology::TriangleList;
        pipelineDesc.colorFormats.push_back(Format::BGRA8);
        auto pipeline = impl_->registry.create(pipelineDesc, e);
        if (!pipeline.valid()) { return false; }

        CommandList list;
        RenderPassDesc pass{};
        pass.name = Name{"dx12_smoke_draw_pass"};
        pass.extent = {256, 256};
        pass.colors.push_back({target, LoadOp::Clear, StoreOp::Store, {{0.02f, 0.04f, 0.08f, 1.0f}, 1.0f, 0}});
        list.begin(pass);
        list.viewport({0.0f, 0.0f, 256.0f, 256.0f, 0.0f, 1.0f});
        list.scissor({0, 0, 256, 256});
        list.pipeline(pipeline);
        list.vertexBuffer(0, vb, 0);
        list.draw(3);
        list.end();

        if (!beginFrame(impl_->frameIndex + 1, 1.0f / 60.0f, e))
        {
            return false;
        }
        if (!submit({Queue::Graphics, &list}, e))
        {
            endFrame(nullptr);
            return false;
        }
        if (!endFrame(e))
        {
            return false;
        }

        return true;
    }

    Dx12GpuAllocationStats Dx12Device::gpuStats() const
    {
        return impl_->gpuStats;
    }

    bool Dx12RuntimeAvailable()
    {
        ComPtr<IDXGIFactory6> factory;
        if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory))))
        {
            return false;
        }

        for (UINT i = 0; ; ++i)
        {
            ComPtr<IDXGIAdapter1> adapter;
            if (factory->EnumAdapters1(i, &adapter) == DXGI_ERROR_NOT_FOUND)
            {
                break;
            }

            DXGI_ADAPTER_DESC1 desc{};
            adapter->GetDesc1(&desc);
            if ((desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0)
            {
                continue;
            }

            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)))
            {
                return true;
            }
        }

        return false;
    }

    std::unique_ptr<Dx12Device> CreateDx12DeviceConcrete()
    {
        return std::make_unique<Dx12Device>();
    }

    std::unique_ptr<IDevice> CreateDx12Device()
    {
        return std::make_unique<Dx12Device>();
    }
}

#else

namespace am::renderer::rhi
{
    struct Dx12Device::Impl {};
    Dx12Device::Dx12Device() : impl_(std::make_unique<Impl>()) {};
    Dx12Device::~Dx12Device() = default;

    bool Dx12Device::initialize(DeviceDesc, std::string* e) { if (e) { *e = "DX12 RHI is only available on Windows."; } return false; }
    void Dx12Device::shutdown() {}
    bool Dx12Device::beginFrame(U64, float, std::string* e) { if (e) { *e = "DX12 RHI is only available on Windows."; } return false; }
    bool Dx12Device::submit(SubmitInfo, std::string* e) { if (e) { *e = "DX12 RHI is only available on Windows."; } return false; }
    bool Dx12Device::endFrame(std::string* e) { if (e) { *e = "DX12 RHI is only available on Windows."; } return false; }
    void Dx12Device::waitIdle() {}
    Registry& Dx12Device::resources() { static Registry r; return r; }
    const Registry& Dx12Device::resources() const { static Registry r; return r; }
    Stats Dx12Device::stats() const { return {}; }
    bool Dx12Device::initialized() const { return false; }
    Backend Dx12Device::backend() const { return Backend::Dx12; }
    bool Dx12Device::ensureNative(Buffer, std::string* e) { if (e) { *e = "DX12 RHI is only available on Windows."; } return false; }
    bool Dx12Device::ensureNative(Texture, std::string* e) { if (e) { *e = "DX12 RHI is only available on Windows."; } return false; }
    bool Dx12Device::upload(Buffer, const void*, U64, std::string* e) { if (e) { *e = "DX12 RHI is only available on Windows."; } return false; }
    bool Dx12Device::clear(Texture, Color, std::string* e) { if (e) { *e = "DX12 RHI is only available on Windows."; } return false; }
    bool Dx12Device::gpuSmokeTest(std::string* e) { if (e) { *e = "DX12 RHI is only available on Windows."; } return false; }
    bool Dx12Device::createOffscreenSceneTargets(Extent2D, Texture*, Texture*, std::string* e) { if (e) { *e = "DX12 RHI is only available on Windows."; } return false; }
    bool Dx12Device::readbackBgra8(Texture, std::vector<U32>*, Extent2D*, std::string* e) { if (e) { *e = "DX12 RHI is only available on Windows."; } return false; }
    bool Dx12Device::submitAndReadbackBgra8(SubmitInfo, Texture, std::vector<U32>*, Extent2D*, std::string* e) { if (e) { *e = "DX12 RHI is only available on Windows."; } return false; }
    bool Dx12Device::submitAndPresentBgra8ToComposition(SubmitInfo, Texture, void*, float, float, Extent2D, std::string* e) { if (e) { *e = "DX12 RHI is only available on Windows."; } return false; }
    bool Dx12Device::presentBgra8ToComposition(Texture, void*, float, float, Extent2D, std::string* e) { if (e) { *e = "DX12 RHI is only available on Windows."; } return false; }
    void Dx12Device::resetCompositionHost() {}
    void Dx12Device::noteGpuViewportComposition(U64) {}
    void Dx12Device::noteD2DTextureBridgeFrame() {}
    void* Dx12Device::nativeD3D12Device() const { return nullptr; }
    void* Dx12Device::nativeD3D12GraphicsQueue() const { return nullptr; }
    void* Dx12Device::nativeD3D12TextureResource(Texture) { return nullptr; }
    Dx12GpuAllocationStats Dx12Device::gpuStats() const { return {}; }

    bool Dx12RuntimeAvailable() { return false; }
    std::unique_ptr<Dx12Device> CreateDx12DeviceConcrete() { return std::make_unique<Dx12Device>(); }
    std::unique_ptr<IDevice> CreateDx12Device() { return std::make_unique<Dx12Device>(); }
}

#endif
