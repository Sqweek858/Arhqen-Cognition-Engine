#include "ArhqenCognitionEngine/Ui/D2D/AceD2DFrameDiagnostics.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>

namespace am::ui
{
    void AceD2DFrameTimer::Start()
    {
        started_ = true;
        start_ = std::chrono::steady_clock::now();
    }

    double AceD2DFrameTimer::MarkMs() const
    {
        if (!started_)
        {
            return 0.0;
        }
        const auto now = std::chrono::steady_clock::now();
        return std::chrono::duration<double, std::milli>(now - start_).count();
    }

    AceD2DFrameRollingStats::AceD2DFrameRollingStats(std::size_t capacity)
        : capacity_(std::max<std::size_t>(1, capacity))
    {
    }

    void AceD2DFrameRollingStats::Reset()
    {
        samples_.clear();
        count_ = 0;
        sum_ = 0.0;
        last_ = 0.0;
        min_ = 0.0;
        max_ = 0.0;
    }

    void AceD2DFrameRollingStats::Push(double valueMs)
    {
        if (valueMs < 0.0 || !std::isfinite(valueMs))
        {
            valueMs = 0.0;
        }
        if (samples_.size() == capacity_)
        {
            sum_ -= samples_.front();
            samples_.pop_front();
        }
        samples_.push_back(valueMs);
        sum_ += valueMs;
        last_ = valueMs;
        ++count_;
        if (samples_.size() == 1)
        {
            min_ = valueMs;
            max_ = valueMs;
        }
        else
        {
            min_ = std::min(min_, valueMs);
            max_ = std::max(max_, valueMs);
        }
        if (samples_.size() == capacity_ && (valueMs <= min_ || valueMs >= max_))
        {
            min_ = *std::min_element(samples_.begin(), samples_.end());
            max_ = *std::max_element(samples_.begin(), samples_.end());
        }
    }

    double AceD2DFrameRollingStats::Average() const
    {
        if (samples_.empty())
        {
            return 0.0;
        }
        return sum_ / static_cast<double>(samples_.size());
    }

    std::vector<AceD2DFrameLatencyBucket> AceD2DFrameRollingStats::Buckets() const
    {
        std::vector<AceD2DFrameLatencyBucket> buckets = {
            {0.0, 4.0, 0},
            {4.0, 8.0, 0},
            {8.0, 16.7, 0},
            {16.7, 33.4, 0},
            {33.4, 66.7, 0},
            {66.7, std::numeric_limits<double>::infinity(), 0}
        };
        for (const double sample : samples_)
        {
            for (auto& bucket : buckets)
            {
                if (sample >= bucket.minMs && sample < bucket.maxMs)
                {
                    ++bucket.count;
                    break;
                }
            }
        }
        return buckets;
    }

    void AceD2DFrameDiagnostics::Reset()
    {
        currentFrame_ = 0;
        frames_ = 0;
        fullFrameRedraws_ = 0;
        partialPaintRequestsRejected_ = 0;
        viewportDraws_ = 0;
        viewportDrawFailures_ = 0;
        presentCalls_ = 0;
        presentFailures_ = 0;
        endDrawFailures_ = 0;
        resizeEpoch_ = 0;
        lastGoodViewportEpoch_ = 0;
        paintRect_ = {};
        currentFullFrame_ = false;
        lastEndDrawHr_ = S_OK;
        lastPresentHr_ = S_OK;
        lastPhase_ = AceD2DFramePhase::Idle;
        lastFailure_.clear();
        frameMs_.Reset();
        presentMs_.Reset();
        phases_.clear();
    }

    void AceD2DFrameDiagnostics::BeginFrame(std::uint64_t frameNumber, std::uint64_t resizeEpoch, UiRect paintRect, bool fullFrame)
    {
        currentFrame_ = frameNumber;
        resizeEpoch_ = resizeEpoch;
        paintRect_ = paintRect;
        currentFullFrame_ = fullFrame;
        lastPhase_ = AceD2DFramePhase::BeginPaint;
        RecordPhase(AceD2DFramePhase::BeginPaint, 0.0, fullFrame ? "full_frame" : "partial_request");
    }

    void AceD2DFrameDiagnostics::RecordPhase(AceD2DFramePhase phase, double elapsedMs, const char* label)
    {
        lastPhase_ = phase;
        if (phases_.size() == kMaxPhaseSamples)
        {
            phases_.erase(phases_.begin());
        }
        AceD2DFramePhaseSample sample{};
        sample.phase = phase;
        sample.elapsedMs = std::max(0.0, elapsedMs);
        sample.frameNumber = currentFrame_;
        if (label)
        {
            sample.label = label;
        }
        phases_.push_back(std::move(sample));
    }

    void AceD2DFrameDiagnostics::RecordFullFrameRedraw()
    {
        ++fullFrameRedraws_;
    }

    void AceD2DFrameDiagnostics::RecordPartialPaintRejected()
    {
        ++partialPaintRequestsRejected_;
    }

    void AceD2DFrameDiagnostics::RecordViewportDraw(bool success, std::uint64_t viewportEpoch, const std::string& failure)
    {
        ++viewportDraws_;
        if (success)
        {
            lastGoodViewportEpoch_ = viewportEpoch;
        }
        else
        {
            ++viewportDrawFailures_;
            if (!failure.empty())
            {
                lastFailure_ = failure;
            }
        }
    }

    void AceD2DFrameDiagnostics::RecordEndDraw(HRESULT hr)
    {
        lastEndDrawHr_ = hr;
        RecordPhase(AceD2DFramePhase::EndDraw, 0.0, SUCCEEDED(hr) ? "ok" : "failed");
        if (FAILED(hr))
        {
            ++endDrawFailures_;
            lastFailure_ = slate::HResultHex(hr);
        }
    }

    void AceD2DFrameDiagnostics::RecordPresent(HRESULT hr, double presentMsValue)
    {
        ++presentCalls_;
        lastPresentHr_ = hr;
        presentMs_.Push(presentMsValue);
        RecordPhase(AceD2DFramePhase::Present, presentMsValue, SUCCEEDED(hr) ? "ok" : "failed");
        if (FAILED(hr))
        {
            ++presentFailures_;
            lastFailure_ = slate::HResultHex(hr);
        }
    }

    void AceD2DFrameDiagnostics::CompleteFrame(double frameMsValue)
    {
        ++frames_;
        frameMs_.Push(frameMsValue);
        RecordPhase(AceD2DFramePhase::Complete, frameMsValue, currentFullFrame_ ? "full_frame" : "partial");
    }

    void AceD2DFrameDiagnostics::RecordFailure(AceD2DFramePhase phase, const std::string& message)
    {
        lastPhase_ = phase;
        lastFailure_ = message;
        RecordPhase(phase, 0.0, "failed");
    }

    AceD2DFrameDiagnosticsSnapshot AceD2DFrameDiagnostics::Snapshot() const
    {
        AceD2DFrameDiagnosticsSnapshot snapshot{};
        snapshot.frames = frames_;
        snapshot.fullFrameRedraws = fullFrameRedraws_;
        snapshot.partialPaintRequestsRejected = partialPaintRequestsRejected_;
        snapshot.viewportDraws = viewportDraws_;
        snapshot.viewportDrawFailures = viewportDrawFailures_;
        snapshot.presentCalls = presentCalls_;
        snapshot.presentFailures = presentFailures_;
        snapshot.endDrawFailures = endDrawFailures_;
        snapshot.resizeEpoch = resizeEpoch_;
        snapshot.lastGoodViewportEpoch = lastGoodViewportEpoch_;
        snapshot.lastFrameMs = frameMs_.Last();
        snapshot.avgFrameMs = frameMs_.Average();
        snapshot.maxFrameMs = frameMs_.Max();
        snapshot.lastPresentMs = presentMs_.Last();
        snapshot.avgPresentMs = presentMs_.Average();
        snapshot.maxPresentMs = presentMs_.Max();
        snapshot.lastFailure = lastFailure_;
        snapshot.lastPhase = AceD2DFramePhaseName(lastPhase_);
        snapshot.presentMode = "full_frame_flip";
        snapshot.frameBuckets = frameMs_.Buckets();
        return snapshot;
    }

    std::string AceD2DFrameDiagnostics::Diagnostics() const
    {
        const auto snapshot = Snapshot();
        std::ostringstream oss;
        oss << "frames=" << snapshot.frames
            << ";full_frame=" << snapshot.fullFrameRedraws
            << ";partial_rejected=" << snapshot.partialPaintRequestsRejected
            << ";viewport_draws=" << snapshot.viewportDraws
            << ";viewport_failures=" << snapshot.viewportDrawFailures
            << ";present=" << snapshot.presentCalls
            << ";present_failures=" << snapshot.presentFailures
            << ";enddraw_failures=" << snapshot.endDrawFailures
            << ";resize_epoch=" << snapshot.resizeEpoch
            << ";last_good_viewport_epoch=" << snapshot.lastGoodViewportEpoch
            << ";frame_ms_last=" << std::fixed << std::setprecision(2) << snapshot.lastFrameMs
            << ";frame_ms_avg=" << snapshot.avgFrameMs
            << ";frame_ms_max=" << snapshot.maxFrameMs
            << ";present_ms_last=" << snapshot.lastPresentMs
            << ";present_ms_avg=" << snapshot.avgPresentMs
            << ";present_ms_max=" << snapshot.maxPresentMs
            << ";phase=" << snapshot.lastPhase
            << ";present_mode=" << snapshot.presentMode
            << ";last_failure=" << snapshot.lastFailure;
        return oss.str();
    }

    std::wstring AceD2DFrameDiagnostics::WideDiagnostics() const
    {
        const std::string text = Diagnostics();
        return std::wstring(text.begin(), text.end());
    }

    const char* AceD2DFramePhaseName(AceD2DFramePhase phase)
    {
        switch (phase)
        {
        case AceD2DFramePhase::Idle: return "idle";
        case AceD2DFramePhase::BeginPaint: return "begin_paint";
        case AceD2DFramePhase::BuildElementList: return "build_element_list";
        case AceD2DFramePhase::DrawBackground: return "draw_background";
        case AceD2DFramePhase::DrawViewport: return "draw_viewport";
        case AceD2DFramePhase::DrawOverlay: return "draw_overlay";
        case AceD2DFramePhase::EndDraw: return "end_draw";
        case AceD2DFramePhase::Present: return "present";
        case AceD2DFramePhase::Complete: return "complete";
        case AceD2DFramePhase::Failed: return "failed";
        default: return "unknown";
        }
    }
}
