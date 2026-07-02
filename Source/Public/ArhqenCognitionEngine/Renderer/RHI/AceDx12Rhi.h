#pragma once

#include "ArhqenCognitionEngine/Renderer/RHI/AceRhi.h"

#include <array>
#include <memory>

namespace am::renderer::rhi
{
    struct Dx12GpuAllocationStats
    {
        U64 nativeBuffers = 0;
        U64 nativeTextures = 0;
        U64 uploadBytesAllocated = 0;
        U64 uploadAllocations = 0;
        U64 descriptorAllocations = 0;
        U64 submittedGpuCommandLists = 0;
        U64 completedFenceValue = 0;
        U64 compiledShaders = 0;
        U64 nativePipelines = 0;
        U64 drawCallsExecuted = 0;
        U64 offscreenSceneTargets = 0;
        U64 readbackFrames = 0;
        U64 readbackBytes = 0;
        U64 wvpConstantsUploaded = 0;
        U64 compositionFrames = 0;
        U64 compositionResizes = 0;
        U64 compositionPresentSkips = 0;
        U64 compositionPacingSkips = 0;
        U64 compositionRenderOnlyFrames = 0;
        U64 compositionRenderIntervalSamples = 0;
        U64 compositionPresentIntervalSamples = 0;
        double compositionRenderIntervalLastMs = 0.0;
        double compositionRenderIntervalAvgMs = 0.0;
        double compositionRenderIntervalMaxMs = 0.0;
        double compositionPresentIntervalLastMs = 0.0;
        double compositionPresentIntervalAvgMs = 0.0;
        double compositionPresentIntervalMaxMs = 0.0;
        U64 zeroCopyFrames = 0;
        U64 mappedUploadBytes = 0;
        U64 mappedUploadUpdates = 0;
        U64 readbackBufferReuses = 0;
        U64 readbackBufferResizes = 0;
        U64 combinedRenderReadbackFrames = 0;
        U64 combinedRenderReadbackBytes = 0;
        U64 gpuCompositedFrames = 0;
        U64 combinedGpuCompositionFrames = 0;
        U64 gpuOverlayBakedFrames = 0;
        U64 gpuOverlayVertices = 0;
        U64 d2dTextureBridgeFrames = 0;
        U64 blockingFenceWaits = 0;
        double blockingFenceWaitMs = 0.0;
    };

    class Dx12Device final : public IDevice
    {
    public:
        Dx12Device();
        ~Dx12Device() override;

        bool initialize(DeviceDesc d, std::string* e) override;
        void shutdown() override;
        bool beginFrame(U64 frame, float dt, std::string* e) override;
        bool submit(SubmitInfo info, std::string* e) override;
        bool endFrame(std::string* e) override;
        void waitIdle() override;
        bool destroy(Buffer h, std::string* e=nullptr) override;
        bool destroy(Texture h, std::string* e=nullptr) override;
        bool destroy(Sampler h, std::string* e=nullptr) override;
        bool destroy(Shader h, std::string* e=nullptr) override;
        bool destroy(Pipeline h, std::string* e=nullptr) override;

        Registry& resources() override;
        const Registry& resources() const override;
        Stats stats() const override;
        bool initialized() const override;
        Backend backend() const override;

        bool ensureNative(Buffer buffer, std::string* e = nullptr);
        bool ensureNative(Texture texture, std::string* e = nullptr);
        bool upload(Buffer dst, const void* data, U64 size, std::string* e = nullptr);
        bool clear(Texture target, Color color, std::string* e = nullptr);
        bool gpuSmokeTest(std::string* e = nullptr);
        bool createOffscreenSceneTargets(Extent2D extent, Texture* color, Texture* depth, std::string* e = nullptr);
        bool readbackBgra8(Texture source, std::vector<U32>* pixels, Extent2D* extent, std::string* e = nullptr);
        bool submitAndReadbackBgra8(SubmitInfo info, Texture source, std::vector<U32>* pixels, Extent2D* extent, std::string* e = nullptr);
        bool submitAndPresentBgra8ToComposition(SubmitInfo info, Texture source, void* hwnd, float left, float top, Extent2D extent, std::string* e = nullptr);
        bool presentBgra8ToComposition(Texture source, void* hwnd, float left, float top, Extent2D extent, std::string* e = nullptr);
        bool setCompositionOverlay(void* content, float left, float top, Extent2D extent, std::string* e = nullptr);
        void resetCompositionHost();
        void noteGpuViewportComposition(U64 overlayVertices = 0);
        void noteD2DTextureBridgeFrame();

        void* nativeD3D12Device() const;
        void* nativeD3D12GraphicsQueue() const;
        void* nativeD3D12TextureResource(Texture texture);
        std::wstring adapterName() const;

        Dx12GpuAllocationStats gpuStats() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };

    bool Dx12RuntimeAvailable();
    std::unique_ptr<Dx12Device> CreateDx12DeviceConcrete();
    std::unique_ptr<IDevice> CreateDx12Device();
}
