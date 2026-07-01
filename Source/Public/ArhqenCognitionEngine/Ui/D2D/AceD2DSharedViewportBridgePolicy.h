#pragma once

#include "ArhqenCognitionEngine/Renderer/RHI/AceViewportTextureBridge.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DUiTypes.h"

#include <d3d11.h>
#include <dxgi1_2.h>

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace am::ui
{
    enum class AceD2DViewportBridgeSyncMode : std::uint8_t
    {
        None,
        D3D11FlushOnly,
        KeyedMutexExperimental,
        ExternalFenceReserved
    };

    enum class AceD2DViewportBridgeBuffering : std::uint8_t
    {
        Single,
        DoubleBufferedLastGood,
        TripleBufferedLatencyTolerant
    };

    struct AceD2DViewportBridgePolicyInput
    {
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
        bool flipModelSwapChain = true;
        bool fullFrameRedraw = true;
        bool localOverlayActive = false;
        bool liveResize = false;
        bool directSurfaceRejected = true;
        std::uint64_t frameNumber = 0;
        std::uint64_t previousContentionCount = 0;
    };

    struct AceD2DViewportBridgePolicyDecision
    {
        AceD2DViewportBridgeSyncMode syncMode = AceD2DViewportBridgeSyncMode::D3D11FlushOnly;
        AceD2DViewportBridgeBuffering buffering = AceD2DViewportBridgeBuffering::DoubleBufferedLastGood;
        D3D11_RESOURCE_MISC_FLAG miscFlag = D3D11_RESOURCE_MISC_SHARED;
        std::uint32_t bufferCount = 2;
        bool drawPreviousCompletedSlot = true;
        bool allowSameFrameReadAfterWrite = false;
        bool allowKeyedMutex = false;
        bool requireD2DFlushBeforeRelease = false;
        bool cacheBitmapPerSlot = true;
        bool recreateOnlyOnExtentOrResourceEpoch = true;
        std::string selectedPath = "shared_ui_d3d11_texture_double_buffer_flush";
        std::string reason;
    };

    class AceD2DSharedViewportBridgePolicy
    {
    public:
        AceD2DViewportBridgePolicyDecision Decide(const AceD2DViewportBridgePolicyInput& input);
        void RecordSuccess(const AceD2DViewportBridgePolicyDecision& decision);
        void RecordFailure(const AceD2DViewportBridgePolicyDecision& decision, HRESULT hr);
        void Reset();
        std::string Diagnostics() const;
        std::uint64_t DecisionCount() const { return decisionCount_; }
        std::uint64_t SuccessCount() const { return successCount_; }
        std::uint64_t FailureCount() const { return failureCount_; }

    private:
        std::uint64_t decisionCount_ = 0;
        std::uint64_t successCount_ = 0;
        std::uint64_t failureCount_ = 0;
        AceD2DViewportBridgePolicyDecision lastDecision_{};
        HRESULT lastFailureHr_ = S_OK;
    };

    const char* AceD2DViewportBridgeSyncModeName(AceD2DViewportBridgeSyncMode mode);
    const char* AceD2DViewportBridgeBufferingName(AceD2DViewportBridgeBuffering mode);
}
