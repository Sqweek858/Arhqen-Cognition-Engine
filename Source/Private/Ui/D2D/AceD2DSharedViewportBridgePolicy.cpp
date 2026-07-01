#include "ArhqenCognitionEngine/Ui/D2D/AceD2DSharedViewportBridgePolicy.h"
#include "ArhqenCognitionEngine/Ui/Slate/AceSlateFramePrimitives.h"

#include <sstream>

namespace am::ui
{
    AceD2DViewportBridgePolicyDecision AceD2DSharedViewportBridgePolicy::Decide(const AceD2DViewportBridgePolicyInput& input)
    {
        AceD2DViewportBridgePolicyDecision decision{};
        decision.syncMode = AceD2DViewportBridgeSyncMode::D3D11FlushOnly;
        decision.buffering = AceD2DViewportBridgeBuffering::DoubleBufferedLastGood;
        decision.miscFlag = D3D11_RESOURCE_MISC_SHARED;
        decision.bufferCount = 2;
        decision.drawPreviousCompletedSlot = true;
        decision.allowSameFrameReadAfterWrite = false;
        decision.allowKeyedMutex = false;
        decision.requireD2DFlushBeforeRelease = false;
        decision.cacheBitmapPerSlot = true;
        decision.recreateOnlyOnExtentOrResourceEpoch = true;
        decision.selectedPath = "shared_ui_d3d11_texture_double_buffer_last_good";
        std::ostringstream reason;
        reason << "vtbridge5_policy"
            << ";flip=" << (input.flipModelSwapChain ? "true" : "false")
            << ";full_frame=" << (input.fullFrameRedraw ? "true" : "false")
            << ";format=" << static_cast<unsigned>(input.format)
            << ";extent=" << input.width << "x" << input.height
            << ";direct_rejected=" << (input.directSurfaceRejected ? "true" : "false")
            << ";contention=" << input.previousContentionCount;
        if (input.previousContentionCount > 0)
        {
            reason << ";keyed_mutex_available_but_not_default";
        }
        if (input.liveResize)
        {
            reason << ";live_resize_prefers_last_good";
        }
        if (input.localOverlayActive)
        {
            reason << ";overlay_layered_as_slate_element";
        }
        decision.reason = reason.str();
        lastDecision_ = decision;
        ++decisionCount_;
        return decision;
    }

    void AceD2DSharedViewportBridgePolicy::RecordSuccess(const AceD2DViewportBridgePolicyDecision& decision)
    {
        lastDecision_ = decision;
        ++successCount_;
    }

    void AceD2DSharedViewportBridgePolicy::RecordFailure(const AceD2DViewportBridgePolicyDecision& decision, HRESULT hr)
    {
        lastDecision_ = decision;
        lastFailureHr_ = hr;
        ++failureCount_;
    }

    void AceD2DSharedViewportBridgePolicy::Reset()
    {
        decisionCount_ = 0;
        successCount_ = 0;
        failureCount_ = 0;
        lastDecision_ = {};
        lastFailureHr_ = S_OK;
    }

    std::string AceD2DSharedViewportBridgePolicy::Diagnostics() const
    {
        std::ostringstream oss;
        oss << "decisions=" << decisionCount_
            << ";success=" << successCount_
            << ";failure=" << failureCount_
            << ";sync=" << AceD2DViewportBridgeSyncModeName(lastDecision_.syncMode)
            << ";buffering=" << AceD2DViewportBridgeBufferingName(lastDecision_.buffering)
            << ";buffers=" << lastDecision_.bufferCount
            << ";same_frame=" << (lastDecision_.allowSameFrameReadAfterWrite ? "true" : "false")
            << ";keyed_default=" << (lastDecision_.allowKeyedMutex ? "true" : "false")
            << ";d2d_flush_before_release=" << (lastDecision_.requireD2DFlushBeforeRelease ? "true" : "false")
            << ";selected=" << lastDecision_.selectedPath
            << ";last_failure=" << slate::HResultHex(lastFailureHr_)
            << ";reason=" << lastDecision_.reason;
        return oss.str();
    }

    const char* AceD2DViewportBridgeSyncModeName(AceD2DViewportBridgeSyncMode mode)
    {
        switch (mode)
        {
        case AceD2DViewportBridgeSyncMode::None: return "none";
        case AceD2DViewportBridgeSyncMode::D3D11FlushOnly: return "d3d11_flush_only";
        case AceD2DViewportBridgeSyncMode::KeyedMutexExperimental: return "keyed_mutex_experimental";
        case AceD2DViewportBridgeSyncMode::ExternalFenceReserved: return "external_fence_reserved";
        default: return "unknown";
        }
    }

    const char* AceD2DViewportBridgeBufferingName(AceD2DViewportBridgeBuffering mode)
    {
        switch (mode)
        {
        case AceD2DViewportBridgeBuffering::Single: return "single";
        case AceD2DViewportBridgeBuffering::DoubleBufferedLastGood: return "double_buffered_last_good";
        case AceD2DViewportBridgeBuffering::TripleBufferedLatencyTolerant: return "triple_buffered_latency_tolerant";
        default: return "unknown";
        }
    }
}
