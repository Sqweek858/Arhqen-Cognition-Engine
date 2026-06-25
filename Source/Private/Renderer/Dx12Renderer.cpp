#include "ArhqenCognitionEngine/Renderer/Dx12Renderer.h"

#include "D3dx12Mini.h"

#include <d3dcompiler.h>

#include <cstring>
#include <sstream>

namespace am::renderer
{
    namespace
    {
        constexpr DXGI_FORMAT kBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

        bool failed(HRESULT hr)
        {
            return FAILED(hr);
        }

        D3D12_RESOURCE_DESC makeBufferDesc(std::uint64_t width)
        {
            D3D12_RESOURCE_DESC desc{};
            desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            desc.Alignment = 0;
            desc.Width = width;
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

        D3D12_HEAP_PROPERTIES makeUploadHeapProps()
        {
            D3D12_HEAP_PROPERTIES props{};
            props.Type = D3D12_HEAP_TYPE_UPLOAD;
            props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
            props.CreationNodeMask = 1;
            props.VisibleNodeMask = 1;
            return props;
        }

        const char* uiVertexShaderSource()
        {
            return R"(
struct VSInput
{
    float2 position : POSITION;
    float4 color : COLOR;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

PSInput main(VSInput input)
{
    PSInput output;
    output.position = float4(input.position, 0.0f, 1.0f);
    output.color = input.color;
    return output;
}
)";
        }

        const char* uiPixelShaderSource()
        {
            return R"(
struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

float4 main(PSInput input) : SV_TARGET
{
    return input.color;
}
)";
        }
    }

    Dx12Renderer::~Dx12Renderer()
    {
        shutdown();
    }

    bool Dx12Renderer::initialize(HWND hwnd, int width, int height, ClearColor clearColor, std::string* error)
    {
        clearColor_ = clearColor;
        renderWidth_ = width;
        renderHeight_ = height;

        if (!createDeviceAndQueue(error))
        {
            return false;
        }

        if (!createSwapChain(hwnd, width, height, error))
        {
            return false;
        }

        if (!createRenderTargetViews(error))
        {
            return false;
        }

        if (!createCommandObjects(error))
        {
            return false;
        }

        if (!createFence(error))
        {
            return false;
        }

        if (!createUiPipeline(error))
        {
            return false;
        }

        if (!createUiVertexBuffer(error))
        {
            return false;
        }

        initialized_ = true;
        return true;
    }

    bool Dx12Renderer::renderFrame(const UiDrawList& drawList, std::string* error)
    {
        if (!initialized_)
        {
            if (error)
            {
                *error = "Dx12Renderer::renderFrame called before initialize.";
            }
            return false;
        }

        if (!uploadUiVertices(drawList, error))
        {
            return false;
        }

        auto* allocator = commandAllocators_[frameIndex_].Get();
        HRESULT hr = allocator->Reset();
        if (failed(hr))
        {
            if (error) { *error = hresultToString("ID3D12CommandAllocator::Reset", hr); }
            return false;
        }

        hr = commandList_->Reset(allocator, nullptr);
        if (failed(hr))
        {
            if (error) { *error = hresultToString("ID3D12GraphicsCommandList::Reset", hr); }
            return false;
        }

        auto transitionToRenderTarget = MakeTransitionBarrier(
            renderTargets_[frameIndex_].Get(),
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET
        );

        commandList_->ResourceBarrier(1, &transitionToRenderTarget);

        const auto rtv = currentRenderTargetView();
        const float color[] = {clearColor_.r, clearColor_.g, clearColor_.b, clearColor_.a};

        D3D12_VIEWPORT viewport{};
        viewport.TopLeftX = 0.0f;
        viewport.TopLeftY = 0.0f;
        viewport.Width = static_cast<float>(renderWidth_);
        viewport.Height = static_cast<float>(renderHeight_);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;

        D3D12_RECT scissorRect{};
        scissorRect.left = 0;
        scissorRect.top = 0;
        scissorRect.right = static_cast<LONG>(renderWidth_);
        scissorRect.bottom = static_cast<LONG>(renderHeight_);

        commandList_->RSSetViewports(1, &viewport);
        commandList_->RSSetScissorRects(1, &scissorRect);

        commandList_->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
        commandList_->ClearRenderTargetView(rtv, color, 0, nullptr);

        if (uiVertexCount_ > 0)
        {
            commandList_->SetGraphicsRootSignature(uiRootSignature_.Get());
            commandList_->SetPipelineState(uiPipelineState_.Get());
            commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            commandList_->IASetVertexBuffers(0, 1, &uiVertexBufferView_);
            commandList_->DrawInstanced(uiVertexCount_, 1, 0, 0);
        }

        auto transitionToPresent = MakeTransitionBarrier(
            renderTargets_[frameIndex_].Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT
        );

        commandList_->ResourceBarrier(1, &transitionToPresent);

        hr = commandList_->Close();
        if (failed(hr))
        {
            if (error) { *error = hresultToString("ID3D12GraphicsCommandList::Close", hr); }
            return false;
        }

        ID3D12CommandList* lists[] = { commandList_.Get() };
        commandQueue_->ExecuteCommandLists(1, lists);

        hr = swapChain_->Present(1, 0);
        if (failed(hr))
        {
            if (error) { *error = hresultToString("IDXGISwapChain::Present", hr); }
            return false;
        }

        moveToNextFrame();
        return true;
    }

    void Dx12Renderer::shutdown()
    {
        if (initialized_)
        {
            waitForGpu();
            initialized_ = false;
        }

        if (fenceEvent_)
        {
            CloseHandle(fenceEvent_);
            fenceEvent_ = nullptr;
        }
    }

    bool Dx12Renderer::initialized() const
    {
        return initialized_;
    }

    bool Dx12Renderer::createDeviceAndQueue(std::string* error)
    {
        UINT factoryFlags = 0;

    #if defined(_DEBUG)
        Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();
            factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }
    #endif

        HRESULT hr = CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&factory_));
        if (failed(hr))
        {
            if (error) { *error = hresultToString("CreateDXGIFactory2", hr); }
            return false;
        }

        Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;

        for (UINT adapterIndex = 0;
             factory_->EnumAdapters1(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND;
             ++adapterIndex)
        {
            DXGI_ADAPTER_DESC1 desc{};
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                continue;
            }

            hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device_));
            if (SUCCEEDED(hr))
            {
                break;
            }
        }

        if (!device_)
        {
            Microsoft::WRL::ComPtr<IDXGIAdapter> warpAdapter;
            hr = factory_->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter));
            if (failed(hr))
            {
                if (error) { *error = hresultToString("IDXGIFactory4::EnumWarpAdapter", hr); }
                return false;
            }

            hr = D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device_));
            if (failed(hr))
            {
                if (error) { *error = hresultToString("D3D12CreateDevice WARP", hr); }
                return false;
            }
        }

        D3D12_COMMAND_QUEUE_DESC queueDesc{};
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

        hr = device_->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue_));
        if (failed(hr))
        {
            if (error) { *error = hresultToString("ID3D12Device::CreateCommandQueue", hr); }
            return false;
        }

        return true;
    }

    bool Dx12Renderer::createSwapChain(HWND hwnd, int width, int height, std::string* error)
    {
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
        swapChainDesc.BufferCount = FrameCount;
        swapChainDesc.Width = static_cast<UINT>(width);
        swapChainDesc.Height = static_cast<UINT>(height);
        swapChainDesc.Format = kBackBufferFormat;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.SampleDesc.Count = 1;

        Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain;

        HRESULT hr = factory_->CreateSwapChainForHwnd(
            commandQueue_.Get(),
            hwnd,
            &swapChainDesc,
            nullptr,
            nullptr,
            &swapChain
        );

        if (failed(hr))
        {
            if (error) { *error = hresultToString("IDXGIFactory4::CreateSwapChainForHwnd", hr); }
            return false;
        }

        factory_->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);

        hr = swapChain.As(&swapChain_);
        if (failed(hr))
        {
            if (error) { *error = hresultToString("IDXGISwapChain1::As IDXGISwapChain3", hr); }
            return false;
        }

        frameIndex_ = swapChain_->GetCurrentBackBufferIndex();
        return true;
    }

    bool Dx12Renderer::createRenderTargetViews(std::string* error)
    {
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
        heapDesc.NumDescriptors = FrameCount;
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        HRESULT hr = device_->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeap_));
        if (failed(hr))
        {
            if (error) { *error = hresultToString("ID3D12Device::CreateDescriptorHeap RTV", hr); }
            return false;
        }

        rtvDescriptorSize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        auto rtvHandle = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
        for (std::uint32_t i = 0; i < FrameCount; ++i)
        {
            hr = swapChain_->GetBuffer(i, IID_PPV_ARGS(&renderTargets_[i]));
            if (failed(hr))
            {
                if (error) { *error = hresultToString("IDXGISwapChain3::GetBuffer", hr); }
                return false;
            }

            device_->CreateRenderTargetView(renderTargets_[i].Get(), nullptr, rtvHandle);
            rtvHandle.ptr += rtvDescriptorSize_;
        }

        return true;
    }

    bool Dx12Renderer::createCommandObjects(std::string* error)
    {
        HRESULT hr = S_OK;

        for (std::uint32_t i = 0; i < FrameCount; ++i)
        {
            hr = device_->CreateCommandAllocator(
                D3D12_COMMAND_LIST_TYPE_DIRECT,
                IID_PPV_ARGS(&commandAllocators_[i])
            );

            if (failed(hr))
            {
                if (error) { *error = hresultToString("ID3D12Device::CreateCommandAllocator", hr); }
                return false;
            }
        }

        hr = device_->CreateCommandList(
            0,
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            commandAllocators_[frameIndex_].Get(),
            nullptr,
            IID_PPV_ARGS(&commandList_)
        );

        if (failed(hr))
        {
            if (error) { *error = hresultToString("ID3D12Device::CreateCommandList", hr); }
            return false;
        }

        hr = commandList_->Close();
        if (failed(hr))
        {
            if (error) { *error = hresultToString("ID3D12GraphicsCommandList::Close initial", hr); }
            return false;
        }

        return true;
    }

    bool Dx12Renderer::createFence(std::string* error)
    {
        HRESULT hr = device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
        if (failed(hr))
        {
            if (error) { *error = hresultToString("ID3D12Device::CreateFence", hr); }
            return false;
        }

        for (auto& value : fenceValues_)
        {
            value = 1;
        }

        fenceEvent_ = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        if (!fenceEvent_)
        {
            if (error)
            {
                *error = "CreateEventW failed while creating DX12 fence event.";
            }
            return false;
        }

        return true;
    }

    bool Dx12Renderer::createUiPipeline(std::string* error)
    {
        Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

        D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
        rootSignatureDesc.NumParameters = 0;
        rootSignatureDesc.pParameters = nullptr;
        rootSignatureDesc.NumStaticSamplers = 0;
        rootSignatureDesc.pStaticSamplers = nullptr;
        rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        HRESULT hr = D3D12SerializeRootSignature(
            &rootSignatureDesc,
            D3D_ROOT_SIGNATURE_VERSION_1,
            &signatureBlob,
            &errorBlob
        );

        if (failed(hr))
        {
            if (error)
            {
                *error = hresultToString("D3D12SerializeRootSignature", hr);
                if (errorBlob)
                {
                    *error += " | ";
                    *error += static_cast<const char*>(errorBlob->GetBufferPointer());
                }
            }
            return false;
        }

        hr = device_->CreateRootSignature(
            0,
            signatureBlob->GetBufferPointer(),
            signatureBlob->GetBufferSize(),
            IID_PPV_ARGS(&uiRootSignature_)
        );

        if (failed(hr))
        {
            if (error) { *error = hresultToString("ID3D12Device::CreateRootSignature", hr); }
            return false;
        }

        Microsoft::WRL::ComPtr<ID3DBlob> vertexShader;
        Microsoft::WRL::ComPtr<ID3DBlob> pixelShader;

        UINT compileFlags = 0;
    #if defined(_DEBUG)
        compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
    #endif

        hr = D3DCompile(
            uiVertexShaderSource(),
            std::strlen(uiVertexShaderSource()),
            "ArhqenCognitionEngineUiVS",
            nullptr,
            nullptr,
            "main",
            "vs_5_0",
            compileFlags,
            0,
            &vertexShader,
            &errorBlob
        );

        if (failed(hr))
        {
            if (error)
            {
                *error = hresultToString("D3DCompile UI vertex shader", hr);
                if (errorBlob)
                {
                    *error += " | ";
                    *error += static_cast<const char*>(errorBlob->GetBufferPointer());
                }
            }
            return false;
        }

        errorBlob.Reset();

        hr = D3DCompile(
            uiPixelShaderSource(),
            std::strlen(uiPixelShaderSource()),
            "ArhqenCognitionEngineUiPS",
            nullptr,
            nullptr,
            "main",
            "ps_5_0",
            compileFlags,
            0,
            &pixelShader,
            &errorBlob
        );

        if (failed(hr))
        {
            if (error)
            {
                *error = hresultToString("D3DCompile UI pixel shader", hr);
                if (errorBlob)
                {
                    *error += " | ";
                    *error += static_cast<const char*>(errorBlob->GetBufferPointer());
                }
            }
            return false;
        }

        D3D12_INPUT_ELEMENT_DESC inputElements[] =
        {
            {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
        };

        D3D12_RASTERIZER_DESC rasterizerDesc{};
        rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
        rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
        rasterizerDesc.FrontCounterClockwise = FALSE;
        rasterizerDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
        rasterizerDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
        rasterizerDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        rasterizerDesc.DepthClipEnable = TRUE;
        rasterizerDesc.MultisampleEnable = FALSE;
        rasterizerDesc.AntialiasedLineEnable = FALSE;
        rasterizerDesc.ForcedSampleCount = 0;
        rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

        D3D12_BLEND_DESC blendDesc{};
        blendDesc.AlphaToCoverageEnable = FALSE;
        blendDesc.IndependentBlendEnable = FALSE;
        auto& rtBlend = blendDesc.RenderTarget[0];
        rtBlend.BlendEnable = TRUE;
        rtBlend.LogicOpEnable = FALSE;
        rtBlend.SrcBlend = D3D12_BLEND_SRC_ALPHA;
        rtBlend.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        rtBlend.BlendOp = D3D12_BLEND_OP_ADD;
        rtBlend.SrcBlendAlpha = D3D12_BLEND_ONE;
        rtBlend.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
        rtBlend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
        rtBlend.LogicOp = D3D12_LOGIC_OP_NOOP;
        rtBlend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
        psoDesc.InputLayout = { inputElements, 2 };
        psoDesc.pRootSignature = uiRootSignature_.Get();
        psoDesc.VS = { vertexShader->GetBufferPointer(), vertexShader->GetBufferSize() };
        psoDesc.PS = { pixelShader->GetBufferPointer(), pixelShader->GetBufferSize() };
        psoDesc.RasterizerState = rasterizerDesc;
        psoDesc.BlendState = blendDesc;
        psoDesc.DepthStencilState.DepthEnable = FALSE;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = kBackBufferFormat;
        psoDesc.SampleDesc.Count = 1;

        hr = device_->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&uiPipelineState_));
        if (failed(hr))
        {
            if (error) { *error = hresultToString("ID3D12Device::CreateGraphicsPipelineState UI", hr); }
            return false;
        }

        return true;
    }

    bool Dx12Renderer::createUiVertexBuffer(std::string* error)
    {
        const auto bufferBytes = static_cast<std::uint64_t>(MaxUiVertices) * sizeof(UiVertex);
        const auto heapProps = makeUploadHeapProps();
        const auto bufferDesc = makeBufferDesc(bufferBytes);

        HRESULT hr = device_->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&uiVertexBuffer_)
        );

        if (failed(hr))
        {
            if (error) { *error = hresultToString("ID3D12Device::CreateCommittedResource UI vertex buffer", hr); }
            return false;
        }

        uiVertexBufferView_.BufferLocation = uiVertexBuffer_->GetGPUVirtualAddress();
        uiVertexBufferView_.SizeInBytes = static_cast<UINT>(bufferBytes);
        uiVertexBufferView_.StrideInBytes = sizeof(UiVertex);

        return true;
    }

    bool Dx12Renderer::uploadUiVertices(const UiDrawList& drawList, std::string* error)
    {
        uiVertexCount_ = drawList.vertexCount();

        if (uiVertexCount_ == 0)
        {
            return true;
        }

        if (uiVertexCount_ > MaxUiVertices)
        {
            if (error)
            {
                *error = "UI draw list exceeds MaxUiVertices.";
            }
            return false;
        }

        const auto bytes = static_cast<std::size_t>(uiVertexCount_) * sizeof(UiVertex);

        void* mapped = nullptr;
        D3D12_RANGE readRange{0, 0};

        HRESULT hr = uiVertexBuffer_->Map(0, &readRange, &mapped);
        if (failed(hr))
        {
            if (error) { *error = hresultToString("ID3D12Resource::Map UI vertex buffer", hr); }
            return false;
        }

        std::memcpy(mapped, drawList.vertices().data(), bytes);
        uiVertexBuffer_->Unmap(0, nullptr);

        return true;
    }

    void Dx12Renderer::waitForGpu()
    {
        if (!commandQueue_ || !fence_)
        {
            return;
        }

        const auto fenceValue = fenceValues_[frameIndex_];
        commandQueue_->Signal(fence_.Get(), fenceValue);
        fence_->SetEventOnCompletion(fenceValue, fenceEvent_);
        WaitForSingleObjectEx(fenceEvent_, INFINITE, FALSE);
        ++fenceValues_[frameIndex_];
    }

    void Dx12Renderer::moveToNextFrame()
    {
        const auto currentFenceValue = fenceValues_[frameIndex_];
        commandQueue_->Signal(fence_.Get(), currentFenceValue);

        frameIndex_ = swapChain_->GetCurrentBackBufferIndex();

        if (fence_->GetCompletedValue() < fenceValues_[frameIndex_])
        {
            fence_->SetEventOnCompletion(fenceValues_[frameIndex_], fenceEvent_);
            WaitForSingleObjectEx(fenceEvent_, INFINITE, FALSE);
        }

        fenceValues_[frameIndex_] = currentFenceValue + 1;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE Dx12Renderer::currentRenderTargetView() const
    {
        auto handle = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
        handle.ptr += static_cast<SIZE_T>(frameIndex_) * rtvDescriptorSize_;
        return handle;
    }

    std::string Dx12Renderer::hresultToString(const char* label, HRESULT hr)
    {
        std::ostringstream msg;
        msg << label << " failed. HRESULT=0x" << std::hex << static_cast<unsigned long>(hr);
        return msg.str();
    }
}
