#pragma once

#include "ArhqenCognitionEngine/Ui/D2D/D2DUiTypes.h"
#include "ArhqenCognitionEngine/Ui/Slate/AceSlateFramePrimitives.h"

#include <chrono>
#include <cstdint>
#include <deque>
#include <string>
#include <vector>

namespace am::ui
{
    enum class AceD2DFramePhase : std::uint8_t
    {
        Idle,
        BeginPaint,
        BuildElementList,
        DrawBackground,
        DrawViewport,
        DrawOverlay,
        EndDraw,
        Present,
        Complete,
        Failed
    };

    struct AceD2DFramePhaseSample
    {
        AceD2DFramePhase phase = AceD2DFramePhase::Idle;
        double elapsedMs = 0.0;
        std::uint64_t frameNumber = 0;
        std::string label;
    };

    struct AceD2DFrameLatencyBucket
    {
        double minMs = 0.0;
        double maxMs = 0.0;
        std::uint64_t count = 0;
    };

    struct AceD2DFrameDiagnosticsSnapshot
    {
        std::uint64_t frames = 0;
        std::uint64_t fullFrameRedraws = 0;
        std::uint64_t partialPaintRequestsRejected = 0;
        std::uint64_t viewportDraws = 0;
        std::uint64_t viewportDrawFailures = 0;
        std::uint64_t presentCalls = 0;
        std::uint64_t presentFailures = 0;
        std::uint64_t endDrawFailures = 0;
        std::uint64_t resizeEpoch = 0;
        std::uint64_t lastGoodViewportEpoch = 0;
        double lastFrameMs = 0.0;
        double avgFrameMs = 0.0;
        double maxFrameMs = 0.0;
        double lastPresentMs = 0.0;
        double avgPresentMs = 0.0;
        double maxPresentMs = 0.0;
        std::string lastFailure;
        std::string lastPhase;
        std::string presentMode;
        std::vector<AceD2DFrameLatencyBucket> frameBuckets;
    };

    class AceD2DFrameTimer
    {
    public:
        void Start();
        double MarkMs() const;
        bool Started() const { return started_; }
    private:
        bool started_ = false;
        std::chrono::steady_clock::time_point start_{};
    };

    class AceD2DFrameRollingStats
    {
    public:
        explicit AceD2DFrameRollingStats(std::size_t capacity = 240);
        void Reset();
        void Push(double valueMs);
        std::uint64_t Count() const { return count_; }
        double Last() const { return last_; }
        double Average() const;
        double Min() const { return min_; }
        double Max() const { return max_; }
        std::vector<AceD2DFrameLatencyBucket> Buckets() const;
    private:
        std::size_t capacity_ = 240;
        std::deque<double> samples_{};
        std::uint64_t count_ = 0;
        double sum_ = 0.0;
        double last_ = 0.0;
        double min_ = 0.0;
        double max_ = 0.0;
    };

    class AceD2DFrameDiagnostics
    {
    public:
        void Reset();
        void BeginFrame(std::uint64_t frameNumber, std::uint64_t resizeEpoch, UiRect paintRect, bool fullFrame);
        void RecordPhase(AceD2DFramePhase phase, double elapsedMs, const char* label = nullptr);
        void RecordFullFrameRedraw();
        void RecordPartialPaintRejected();
        void RecordViewportDraw(bool success, std::uint64_t viewportEpoch, const std::string& failure = {});
        void RecordEndDraw(HRESULT hr);
        void RecordPresent(HRESULT hr, double presentMs);
        void CompleteFrame(double frameMs);
        void RecordFailure(AceD2DFramePhase phase, const std::string& message);
        AceD2DFrameDiagnosticsSnapshot Snapshot() const;
        std::string Diagnostics() const;
        std::wstring WideDiagnostics() const;
        const std::vector<AceD2DFramePhaseSample>& RecentPhases() const { return phases_; }
    private:
        static constexpr std::size_t kMaxPhaseSamples = 96;
        std::uint64_t currentFrame_ = 0;
        std::uint64_t frames_ = 0;
        std::uint64_t fullFrameRedraws_ = 0;
        std::uint64_t partialPaintRequestsRejected_ = 0;
        std::uint64_t viewportDraws_ = 0;
        std::uint64_t viewportDrawFailures_ = 0;
        std::uint64_t presentCalls_ = 0;
        std::uint64_t presentFailures_ = 0;
        std::uint64_t endDrawFailures_ = 0;
        std::uint64_t resizeEpoch_ = 0;
        std::uint64_t lastGoodViewportEpoch_ = 0;
        UiRect paintRect_{};
        bool currentFullFrame_ = false;
        HRESULT lastEndDrawHr_ = S_OK;
        HRESULT lastPresentHr_ = S_OK;
        AceD2DFramePhase lastPhase_ = AceD2DFramePhase::Idle;
        std::string lastFailure_;
        AceD2DFrameRollingStats frameMs_{240};
        AceD2DFrameRollingStats presentMs_{240};
        std::vector<AceD2DFramePhaseSample> phases_{};
    };

    const char* AceD2DFramePhaseName(AceD2DFramePhase phase);
}
