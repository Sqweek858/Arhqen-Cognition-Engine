#include "ArhqenCognitionEngine/Ui/D2D/AceD2DFrameTransaction.h"

#include <dxgi.h>

#include <sstream>

namespace am::ui
{
    void AceD2DFrameTransaction::Reset()
    {
        input_ = {};
        presentPlan_ = {};
        state_ = AceD2DFrameTransactionState::Idle;
        lastEndDrawHr_ = S_OK;
        lastPresentHr_ = S_OK;
        lastFailure_.clear();
        diagnostics_.Reset();
        presentScheduler_.Reset();
    }

    bool AceD2DFrameTransaction::Begin(const AceD2DFrameTransactionInput& input)
    {
        input_ = input;
        state_ = AceD2DFrameTransactionState::Begun;
        diagnostics_.BeginFrame(input.frameNumber, input.resizeEpoch, input.frameRect, input.fullFrame);
        if (input.fullFrame)
        {
            diagnostics_.RecordFullFrameRedraw();
        }
        else
        {
            diagnostics_.RecordPartialPaintRejected();
        }
        AceD2DPresentInput presentInput{};
        presentInput.hasSwapChain = input.hasSwapChain;
        presentInput.fullFrameRedraw = input.fullFrame;
        presentInput.liveResize = input.liveResize;
        presentInput.viewportActive = input.viewportActive;
        presentInput.deviceLost = false;
        presentInput.frameNumber = input.frameNumber;
        presentInput.resizeEpoch = input.resizeEpoch;
        presentInput.frameRect = input.frameRect;
        presentInput.requestedDirtyRect = input.frameRect;
        presentPlan_ = presentScheduler_.BuildPlan(presentInput);
        return true;
    }

    void AceD2DFrameTransaction::MarkDrawingPhase(AceD2DFramePhase phase, const char* label)
    {
        if (state_ == AceD2DFrameTransactionState::Begun || state_ == AceD2DFrameTransactionState::Drawing)
        {
            state_ = AceD2DFrameTransactionState::Drawing;
            diagnostics_.RecordPhase(phase, 0.0, label);
        }
    }

    AceD2DFrameTransactionResult AceD2DFrameTransaction::EndDraw(HRESULT hr)
    {
        lastEndDrawHr_ = hr;
        diagnostics_.RecordEndDraw(hr);
        if (FAILED(hr))
        {
            state_ = AceD2DFrameTransactionState::Failed;
            lastFailure_ = slate::HResultHex(hr);
        }
        else
        {
            state_ = AceD2DFrameTransactionState::EndDrawSubmitted;
        }
        return BuildResult();
    }

    AceD2DFrameTransactionResult AceD2DFrameTransaction::Present(HRESULT hr, double presentMs)
    {
        lastPresentHr_ = hr;
        presentScheduler_.RecordPresent(presentPlan_, hr, presentMs);
        diagnostics_.RecordPresent(hr, presentMs);
        if (FAILED(hr))
        {
            state_ = AceD2DFrameTransactionState::Failed;
            lastFailure_ = slate::HResultHex(hr);
        }
        else
        {
            state_ = AceD2DFrameTransactionState::Presented;
        }
        return BuildResult();
    }

    AceD2DFrameTransactionResult AceD2DFrameTransaction::Complete(double frameMs)
    {
        diagnostics_.CompleteFrame(frameMs);
        if (state_ != AceD2DFrameTransactionState::Failed)
        {
            state_ = AceD2DFrameTransactionState::Idle;
        }
        return BuildResult();
    }

    void AceD2DFrameTransaction::Fail(AceD2DFramePhase phase, const std::string& reason)
    {
        state_ = AceD2DFrameTransactionState::Failed;
        lastFailure_ = reason;
        diagnostics_.RecordFailure(phase, reason);
    }

    AceD2DFrameTransactionResult AceD2DFrameTransaction::BuildResult() const
    {
        AceD2DFrameTransactionResult result{};
        result.ok = state_ != AceD2DFrameTransactionState::Failed;
        result.shouldPresent = presentPlan_.callPresent && SUCCEEDED(lastEndDrawHr_);
        result.shouldDiscardDevice = lastEndDrawHr_ == D2DERR_RECREATE_TARGET ||
            lastPresentHr_ == DXGI_ERROR_DEVICE_REMOVED ||
            lastPresentHr_ == DXGI_ERROR_DEVICE_RESET;
        result.fullFrame = input_.fullFrame;
        result.endDrawHr = lastEndDrawHr_;
        result.presentHr = lastPresentHr_;
        result.presentPlan = presentPlan_;
        result.diagnostics = Diagnostics();
        return result;
    }

    std::string AceD2DFrameTransaction::Diagnostics() const
    {
        std::ostringstream oss;
        oss << "state=" << AceD2DFrameTransactionStateName(state_)
            << ";frame=" << input_.frameNumber
            << ";resize_epoch=" << input_.resizeEpoch
            << ";full_frame=" << (input_.fullFrame ? "true" : "false")
            << ";viewport=" << (input_.viewportActive ? "true" : "false")
            << ";enddraw=" << slate::HResultHex(lastEndDrawHr_)
            << ";present=" << slate::HResultHex(lastPresentHr_)
            << ";present_plan={" << presentPlan_.reason << "}"
            << ";diagnostics={" << diagnostics_.Diagnostics() << "}"
            << ";scheduler={" << presentScheduler_.Diagnostics() << "}";
        if (!lastFailure_.empty())
        {
            oss << ";failure=" << lastFailure_;
        }
        return oss.str();
    }

    std::wstring AceD2DFrameTransaction::WideDiagnostics() const
    {
        const std::string text = Diagnostics();
        return std::wstring(text.begin(), text.end());
    }

    const char* AceD2DFrameTransactionStateName(AceD2DFrameTransactionState state)
    {
        switch (state)
        {
        case AceD2DFrameTransactionState::Idle: return "idle";
        case AceD2DFrameTransactionState::Begun: return "begun";
        case AceD2DFrameTransactionState::Drawing: return "drawing";
        case AceD2DFrameTransactionState::EndDrawSubmitted: return "enddraw_submitted";
        case AceD2DFrameTransactionState::Presented: return "presented";
        case AceD2DFrameTransactionState::Failed: return "failed";
        default: return "unknown";
        }
    }
}
