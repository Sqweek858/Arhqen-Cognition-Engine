#pragma once

#if !defined(_WIN32)
#error ArhqenCognitionEngine M4 Dx12Renderer is Windows-only.
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "ArhqenCognitionEngine/Renderer/UiDrawList.h"
#include "ArhqenCognitionEngine/Renderer/Scene3DDrawList.h"

#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <array>
#include <cstdint>
#include <string>

namespace am::renderer
{
    struct ClearColor
    {
        float r = 0.02f;
        float g = 0.04f;
        float b = 0.07f;
        float a = 1.0f;
    };

    class Dx12Renderer
    {
    public:
        static constexpr std::uint32_t FrameCount = 2;
        static constexpr std::uint32_t MaxUiVertices = 8192;
        static constexpr std::uint32_t MaxScene3DVertices = 65536;

        Dx12Renderer() = default;
        ~Dx12Renderer();

        Dx12Renderer(const Dx12Renderer&) = delete;
        Dx12Renderer& operator=(const Dx12Renderer&) = delete;

        bool initialize(HWND hwnd, int width, int height, ClearColor clearColor, std::string* error);
        bool renderFrame(const UiDrawList& drawList, std::string* error);
        bool renderFrame3D(const Scene3DDrawList& scene, const Scene3DConstants& constants, const UiDrawList& overlay, std::string* error);
        void shutdown();

        bool initialized() const;

    private:
        bool createDeviceAndQueue(std::string* error);
        bool createSwapChain(HWND hwnd, int width, int height, std::string* error);
        bool createRenderTargetViews(std::string* error);
        bool createCommandObjects(std::string* error);
        bool createFence(std::string* error);
        bool createUiPipeline(std::string* error);
        bool createUiVertexBuffer(std::string* error);
        bool uploadUiVertices(const UiDrawList& drawList, std::string* error);
        bool createScene3DPipeline(std::string* error);
        bool createScene3DResources(std::string* error);
        bool createDepthResources(std::string* error);
        bool uploadScene3DVertices(const Scene3DDrawList& scene, std::string* error);
        bool uploadScene3DConstants(const Scene3DConstants& constants, std::string* error);

        void waitForGpu();
        void moveToNextFrame();
        D3D12_CPU_DESCRIPTOR_HANDLE currentRenderTargetView() const;

        static std::string hresultToString(const char* label, HRESULT hr);

        ClearColor clearColor_{};

        Microsoft::WRL::ComPtr<IDXGIFactory4> factory_;
        Microsoft::WRL::ComPtr<ID3D12Device> device_;
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue_;
        Microsoft::WRL::ComPtr<IDXGISwapChain3> swapChain_;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap_;
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocators_[FrameCount];
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_;
        Microsoft::WRL::ComPtr<ID3D12Resource> renderTargets_[FrameCount];
        Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap_;
        Microsoft::WRL::ComPtr<ID3D12Resource> depthStencil_;

        Microsoft::WRL::ComPtr<ID3D12RootSignature> uiRootSignature_;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> uiPipelineState_;
        Microsoft::WRL::ComPtr<ID3D12Resource> uiVertexBuffer_;
        D3D12_VERTEX_BUFFER_VIEW uiVertexBufferView_{};
        std::uint32_t uiVertexCount_ = 0;

        Microsoft::WRL::ComPtr<ID3D12RootSignature> scene3DRootSignature_;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> scene3DPipelineState_;
        Microsoft::WRL::ComPtr<ID3D12Resource> scene3DVertexBuffer_;
        Microsoft::WRL::ComPtr<ID3D12Resource> scene3DConstantBuffer_;
        D3D12_VERTEX_BUFFER_VIEW scene3DVertexBufferView_{};
        std::uint32_t scene3DVertexCount_ = 0;

        HANDLE fenceEvent_ = nullptr;
        std::uint64_t fenceValues_[FrameCount] = {};
        std::uint32_t frameIndex_ = 0;
        std::uint32_t rtvDescriptorSize_ = 0;
        int renderWidth_ = 0;
        int renderHeight_ = 0;
        bool initialized_ = false;
    };
}
