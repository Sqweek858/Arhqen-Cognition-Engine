#pragma once

#include "ArhqenCognitionEngine/AquariumRender/AceAquariumRenderPrimitive.h"
#include "ArhqenCognitionEngine/Renderer/RHI/AceDx12Rhi.h"
#include "ArhqenCognitionEngine/Renderer/RHI/AceViewportTextureBridge.h"

#include <array>
#include <memory>
#include <string>
#include <vector>

namespace am::renderer::scene
{
    struct AceAquariumGpuViewportSnapshot
    {
        std::vector<am::renderer::rhi::U32> bgraPixels;
        am::renderer::rhi::Extent2D extent{};
        bool valid = false;
        bool gpuRendered = false;
        bool zeroCopyPresented = false;
        bool gpuComposited = false;
        bool overlayBaked = false;
        bool combinedRenderReadback = false;
        bool d2dTextureBridgeReady = false;
        am::renderer::rhi::AceViewportTextureResource viewportTexture{};
        am::renderer::rhi::AceViewportTextureBridgeStatus viewportBridge{};
        std::string status;
    };


    struct AceAquariumGpuOverlayRect
    {
        float x = 0.0f;
        float y = 0.0f;
        float w = 0.0f;
        float h = 0.0f;
        float r = 1.0f;
        float g = 1.0f;
        float b = 1.0f;
        float a = 1.0f;
    };

    struct AceAquariumGpuOverlayText
    {
        float x = 0.0f;
        float y = 0.0f;
        float scale = 2.0f;
        float r = 0.25f;
        float g = 0.85f;
        float b = 1.0f;
        float a = 1.0f;
        std::string text;
    };

    struct AceAquariumGpuViewportOverlay
    {
        std::vector<AceAquariumGpuOverlayRect> rects;
        std::vector<AceAquariumGpuOverlayText> texts;
        bool enabled = false;
    };

    struct AceAquariumGpuViewportStats
    {
        am::renderer::rhi::U64 framesRendered = 0;
        am::renderer::rhi::U64 verticesUploaded = 0;
        am::renderer::rhi::U64 readbackFrames = 0;
        am::renderer::rhi::U64 zeroCopyFrames = 0;
        am::renderer::rhi::U64 combinedReadbackFrames = 0;
        am::renderer::rhi::U64 gpuCompositedFrames = 0;
        am::renderer::rhi::U64 gpuOverlayBakedFrames = 0;
        am::renderer::rhi::U64 gpuOverlayVertexCount = 0;
        am::renderer::rhi::U64 viewportTextureExports = 0;
        am::renderer::rhi::U64 viewportBridgeReadbackFallbacks = 0;
        am::renderer::rhi::U64 d2dTextureBridgeFrames = 0;
        am::renderer::rhi::U64 geometryUploadFrames = 0;
        am::renderer::rhi::U64 geometryReuseFrames = 0;
        am::renderer::rhi::U64 compositionPresentedFrames = 0;
        am::renderer::rhi::U64 targetResizes = 0;
        am::renderer::rhi::U32 lastPrimitiveCount = 0;
        am::renderer::rhi::U32 lastVertexCount = 0;
        am::renderer::rhi::Extent2D lastExtent{};
    };

    class AceAquariumGpuViewportRenderer
    {
    public:
        AceAquariumGpuViewportRenderer();
        ~AceAquariumGpuViewportRenderer();

        bool initialize(std::string* error = nullptr);
        void shutdown();
        void setWorldToClipMatrix(const std::array<float, 16>& matrix);
        void resetWorldToClipMatrix();

        bool render(
            const std::vector<ace::aquarium_render::AceAqRenderPrimitive>& primitives,
            am::renderer::rhi::U32 requestedWidth,
            am::renderer::rhi::U32 requestedHeight,
            bool debugTruthEnabled,
            AceAquariumGpuViewportSnapshot* snapshot,
            std::string* error = nullptr,
            const AceAquariumGpuViewportOverlay* overlay = nullptr,
            void* compositionHwnd = nullptr,
            float compositionLeft = 0.0f,
            float compositionTop = 0.0f,
            bool preferD2DTextureBridge = false);

        bool render(
            const std::vector<ace::aquarium_render::AceAqRenderPrimitive>& primitives,
            am::renderer::rhi::U32 requestedWidth,
            am::renderer::rhi::U32 requestedHeight,
            bool debugTruthEnabled,
            AceAquariumGpuViewportSnapshot* snapshot,
            std::string* error,
            void* compositionHwnd,
            float compositionLeft,
            float compositionTop);

        bool initialized() const;
        bool setCompositionOverlay(void* content, float left, float top, am::renderer::rhi::Extent2D extent, std::string* error = nullptr);
        void resetCompositionHost();
        AceAquariumGpuViewportStats stats() const { return stats_; }
        am::renderer::rhi::Dx12GpuAllocationStats gpuStats() const;
        am::renderer::rhi::Extent2D viewportTextureExtent() const;
        am::renderer::rhi::AceViewportTextureResource viewportTextureResource() const;
        am::renderer::rhi::AceViewportTextureBridgeStatus viewportBridgeStatus(bool uiGpuTextureSamplingAvailable) const;
        void* nativeD3D12Device() const;
        void* nativeD3D12GraphicsQueue() const;
        std::wstring adapterName() const;

    public:
        struct Vertex
        {
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;
            float r = 1.0f;
            float g = 1.0f;
            float b = 1.0f;
            float a = 1.0f;
        };

    private:
        bool ensureTargets(am::renderer::rhi::Extent2D extent, std::string* error);
        bool ensurePipeline(std::string* error);
        bool ensureOverlayPipeline(std::string* error);
        bool ensureVertexBuffer(std::size_t requiredBytes, std::string* error);
        void buildVertices(
            const std::vector<ace::aquarium_render::AceAqRenderPrimitive>& primitives,
            bool debugTruthEnabled,
            std::vector<Vertex>& outVertices) const;
        void appendOverlayVertices(
            const AceAquariumGpuViewportOverlay& overlay,
            am::renderer::rhi::Extent2D extent,
            std::vector<Vertex>& outVertices) const;

        std::unique_ptr<am::renderer::rhi::Dx12Device> device_;
        am::renderer::rhi::Texture sceneColor_{};
        am::renderer::rhi::Texture sceneDepth_{};
        am::renderer::rhi::Buffer vertexBuffer_{};
        am::renderer::rhi::Shader vertexShader_{};
        am::renderer::rhi::Pipeline pipeline_{};
        am::renderer::rhi::Pipeline overlayPipeline_{};
        am::renderer::rhi::Extent2D targetExtent_{};
        std::size_t vertexBufferCapacityBytes_ = 0;
        std::vector<Vertex> cachedUploadedVertices_{};
        AceAquariumGpuViewportStats stats_{};
        std::array<float, 16> worldToClip_{};
        bool hasWorldToClip_ = false;
    };
}
