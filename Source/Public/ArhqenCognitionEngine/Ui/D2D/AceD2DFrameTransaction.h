#pragma once

#include "ArhqenCognitionEngine/Ui/D2D/AceD2DFrameDiagnostics.h"
#include "ArhqenCognitionEngine/Ui/D2D/AceD2DPresentScheduler.h"
#include "ArhqenCognitionEngine/Ui/Slate/AceSlateFramePrimitives.h"

#include <cstdint>
#include <string>
#include <vector>

namespace am::ui
{
    enum class AceD2DFrameTransactionState : std::uint8_t
    {
        Idle,
        Begun,
        Drawing,
        EndDrawSubmitted,
        Presented,
        Failed
    };

    struct AceD2DFrameTransactionInput
    {
        std::uint64_t frameNumber = 0;
        std::uint64_t resizeEpoch = 0;
        UiRect frameRect{};
        bool hasSwapChain = false;
        bool fullFrame = true;
        bool viewportActive = false;
        bool liveResize = false;
    };

    struct AceD2DFrameTransactionResult
    {
        bool ok = true;
        bool shouldPresent = true;
        bool shouldDiscardDevice = false;
        bool fullFrame = true;
        HRESULT endDrawHr = S_OK;
        HRESULT presentHr = S_OK;
        AceD2DPresentPlan presentPlan{};
        std::string diagnostics;
    };

    class AceD2DFrameTransaction
    {
    public:
        void Reset();
        bool Begin(const AceD2DFrameTransactionInput& input);
        void MarkDrawingPhase(AceD2DFramePhase phase, const char* label = nullptr);
        AceD2DFrameTransactionResult EndDraw(HRESULT hr);
        AceD2DFrameTransactionResult Present(HRESULT hr, double presentMs);
        AceD2DFrameTransactionResult Complete(double frameMs);
        void Fail(AceD2DFramePhase phase, const std::string& reason);
        AceD2DFrameTransactionState State() const { return state_; }
        AceD2DFrameDiagnostics& DiagnosticsRecorder() { return diagnostics_; }
        const AceD2DFrameDiagnostics& DiagnosticsRecorder() const { return diagnostics_; }
        AceD2DPresentScheduler& PresentScheduler() { return presentScheduler_; }
        const AceD2DPresentScheduler& PresentScheduler() const { return presentScheduler_; }
        std::string Diagnostics() const;
        std::wstring WideDiagnostics() const;
    private:
        AceD2DFrameTransactionResult BuildResult() const;
        AceD2DFrameTransactionInput input_{};
        AceD2DPresentPlan presentPlan_{};
        AceD2DFrameTransactionState state_ = AceD2DFrameTransactionState::Idle;
        HRESULT lastEndDrawHr_ = S_OK;
        HRESULT lastPresentHr_ = S_OK;
        std::string lastFailure_;
        AceD2DFrameDiagnostics diagnostics_{};
        AceD2DPresentScheduler presentScheduler_{};
    };

    const char* AceD2DFrameTransactionStateName(AceD2DFrameTransactionState state);
}
