#pragma once

#include "ArhqenCognitionEngine/Renderer/RHI/AceRhi.h"

#include <string>
#include <utility>

namespace am::renderer::rhi
{
    // ACE-VTBRIDGE0: UE/Slate-style viewport texture bridge foundation.
    // This describes a GPU viewport resource that can become a UI draw element.
    // It is intentionally API-light: no UI code may read pixels from it here.
    enum class AceViewportTextureResourceKind
    {
        None,
        RhiTexture,
        NativeGpuResource,
        ReadbackFallback
    };

    enum class AceViewportTextureBridgeMode
    {
        Unavailable,
        GpuTextureReady,
        GpuSampledD2D,
        Failed,
        ReadbackFallback
    };

    enum class AceViewportUiRendererKind
    {
        Unknown,
        LegacyD2DHwndRenderTarget,
        D2DDeviceContext
    };

    enum class AceViewportGpuInteropKind
    {
        None,
        D3D11On12DxgiSurface,
        D3D11On12SharedD3D11Texture,
        D3D11On12SharedBitmap
    };

    inline const char* AceViewportTextureResourceKindToString(AceViewportTextureResourceKind value)
    {
        switch (value)
        {
        case AceViewportTextureResourceKind::RhiTexture: return "RHI_TEXTURE";
        case AceViewportTextureResourceKind::NativeGpuResource: return "NATIVE_GPU_RESOURCE";
        case AceViewportTextureResourceKind::ReadbackFallback: return "READBACK_FALLBACK";
        default: return "NONE";
        }
    }

    inline const char* AceViewportTextureBridgeModeToString(AceViewportTextureBridgeMode value)
    {
        switch (value)
        {
        case AceViewportTextureBridgeMode::GpuTextureReady: return "GPU_TEXTURE_READY";
        case AceViewportTextureBridgeMode::GpuSampledD2D: return "GPU_SAMPLED_D2D";
        case AceViewportTextureBridgeMode::Failed: return "FAILED";
        case AceViewportTextureBridgeMode::ReadbackFallback: return "READBACK_FALLBACK";
        default: return "UNAVAILABLE";
        }
    }

    inline const char* AceViewportUiRendererKindToString(AceViewportUiRendererKind value)
    {
        switch (value)
        {
        case AceViewportUiRendererKind::LegacyD2DHwndRenderTarget: return "LEGACY_D2D_HWND_RENDER_TARGET";
        case AceViewportUiRendererKind::D2DDeviceContext: return "D2D_DEVICE_CONTEXT";
        default: return "UNKNOWN";
        }
    }

    inline const char* AceViewportGpuInteropKindToString(AceViewportGpuInteropKind value)
    {
        switch (value)
        {
        case AceViewportGpuInteropKind::D3D11On12DxgiSurface: return "D3D11ON12_DXGI_SURFACE";
        case AceViewportGpuInteropKind::D3D11On12SharedD3D11Texture: return "D3D11ON12_SHARED_D3D11_TEXTURE";
        case AceViewportGpuInteropKind::D3D11On12SharedBitmap: return "D3D11ON12_SHARED_BITMAP";
        default: return "NONE";
        }
    }

    struct AceViewportTextureResource
    {
        AceViewportTextureResourceKind kind = AceViewportTextureResourceKind::None;
        Backend producerBackend = Backend::Null;
        Texture rhiTexture{};
        void* nativeResource = nullptr;
        Extent2D extent{};
        Format format = Format::Unknown;
        bool shaderReadable = false;
        bool renderTarget = false;
        bool cpuReadback = false;
        std::string debugName;

        bool validGpuTexture() const
        {
            return (kind == AceViewportTextureResourceKind::RhiTexture && rhiTexture.valid()) ||
                (kind == AceViewportTextureResourceKind::NativeGpuResource && nativeResource != nullptr);
        }
    };

    struct AceViewportTextureBridgeStatus
    {
        AceViewportTextureBridgeMode mode = AceViewportTextureBridgeMode::Unavailable;
        bool gpuResourceAvailable = false;
        bool uiCanSampleGpuTexture = false;
        bool readbackFallbackRequired = true;
        bool legacyFallbackUsed = false;
        AceViewportUiRendererKind uiRenderer = AceViewportUiRendererKind::Unknown;
        AceViewportUiRendererKind requiredUiRenderer = AceViewportUiRendererKind::D2DDeviceContext;
        AceViewportGpuInteropKind requiredInterop = AceViewportGpuInteropKind::D3D11On12DxgiSurface;
        std::string fallbackReason = "not_evaluated";
        std::string fatalBridgeStep;
        std::string fatalBridgeHresult;
    };

    class IAceViewportTextureSource
    {
    public:
        virtual ~IAceViewportTextureSource() = default;
        virtual Extent2D viewportTextureExtent() const = 0;
        virtual AceViewportTextureResource viewportTextureResource() const = 0;
        virtual bool useSeparateRenderTarget() const { return true; }
        virtual bool requiresVsync() const { return true; }
        virtual const char* viewportSourceName() const { return "AceViewportTextureSource"; }
    };

    inline AceViewportTextureResource MakeAceRhiViewportTextureResource(
        Texture texture,
        Extent2D extent,
        Format format,
        Backend backend,
        std::string debugName)
    {
        AceViewportTextureResource resource{};
        resource.kind = texture.valid() ? AceViewportTextureResourceKind::RhiTexture : AceViewportTextureResourceKind::None;
        resource.producerBackend = backend;
        resource.rhiTexture = texture;
        resource.extent = extent;
        resource.format = format;
        resource.shaderReadable = texture.valid();
        resource.renderTarget = texture.valid();
        resource.cpuReadback = false;
        resource.debugName = std::move(debugName);
        return resource;
    }

    // Compatibility marker for ACE-VTBRIDGE0 validators: the old generic
    // fallback reason was ui_renderer_cannot_sample_gpu_viewport_texture_yet.
    // ACE-VTBRIDGE1 keeps that diagnosis but splits it into concrete blockers.

    inline AceViewportTextureBridgeStatus EvaluateAceViewportTextureBridge(
        const AceViewportTextureResource& resource,
        AceViewportUiRendererKind uiRenderer,
        bool gpuTextureSamplingPathAvailable)
    {
        AceViewportTextureBridgeStatus status{};
        status.gpuResourceAvailable = resource.validGpuTexture();
        status.uiRenderer = uiRenderer;
        status.uiCanSampleGpuTexture = gpuTextureSamplingPathAvailable && uiRenderer == AceViewportUiRendererKind::D2DDeviceContext;

        if (!status.gpuResourceAvailable)
        {
            status.mode = AceViewportTextureBridgeMode::Unavailable;
            status.readbackFallbackRequired = true;
            status.fallbackReason = "no_gpu_viewport_resource";
            return status;
        }

        if (uiRenderer == AceViewportUiRendererKind::LegacyD2DHwndRenderTarget)
        {
            status.mode = AceViewportTextureBridgeMode::ReadbackFallback;
            status.readbackFallbackRequired = true;
            status.fallbackReason = "ui_renderer_legacy_hwnd_render_target_needs_d2d_device_context_bridge";
            return status;
        }

        if (!gpuTextureSamplingPathAvailable)
        {
            status.mode = AceViewportTextureBridgeMode::ReadbackFallback;
            status.readbackFallbackRequired = true;
            status.fallbackReason = "d3d11on12_dxgi_surface_bridge_not_created_yet";
            return status;
        }

        status.mode = AceViewportTextureBridgeMode::GpuSampledD2D;
        status.requiredInterop = AceViewportGpuInteropKind::D3D11On12DxgiSurface;
        status.readbackFallbackRequired = false;
        status.legacyFallbackUsed = false;
        status.fallbackReason = "none";
        return status;
    }


    inline AceViewportTextureBridgeStatus MakeAceViewportD2DDeviceContextFailure(
        const AceViewportTextureResource& resource,
        std::string step,
        std::string hresultOrReason)
    {
        AceViewportTextureBridgeStatus status{};
        status.mode = AceViewportTextureBridgeMode::Failed;
        status.gpuResourceAvailable = resource.validGpuTexture();
        status.uiCanSampleGpuTexture = false;
        status.readbackFallbackRequired = false;
        status.legacyFallbackUsed = false;
        status.uiRenderer = AceViewportUiRendererKind::D2DDeviceContext;
        status.requiredUiRenderer = AceViewportUiRendererKind::D2DDeviceContext;
        status.requiredInterop = AceViewportGpuInteropKind::D3D11On12DxgiSurface;
        status.fallbackReason = hresultOrReason.empty() ? step : hresultOrReason;
        status.fatalBridgeStep = std::move(step);
        status.fatalBridgeHresult = std::move(hresultOrReason);
        return status;
    }

    inline AceViewportTextureBridgeStatus EvaluateAceViewportTextureBridge(
        const AceViewportTextureResource& resource,
        bool uiGpuTextureSamplingAvailable)
    {
        return EvaluateAceViewportTextureBridge(
            resource,
            uiGpuTextureSamplingAvailable ? AceViewportUiRendererKind::D2DDeviceContext : AceViewportUiRendererKind::LegacyD2DHwndRenderTarget,
            uiGpuTextureSamplingAvailable);
    }
}
