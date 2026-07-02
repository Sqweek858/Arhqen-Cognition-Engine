#pragma once

#include "ArhqenCognitionEngine/Renderer/RHI/AceDx12Rhi.h"
#include "ArhqenCognitionEngine/Renderer/Scene/AceAquariumGpuViewportRenderer.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace am::ui
{
    enum class AceEngineRenderPath
    {
        Unknown,
        Dx12ZeroCopy,
        Dx12GpuComposited,
        Dx12Readback,
        Dx12CombinedReadback,
        Dx12CachedReadback,
        Dx12D2DTextureBridge,
        FailedD2DDeviceContext,
        // Compatibility marker: old VTBRIDGE2 label was DX12_D2D_TEXTURE_BRIDGE.
        CpuD2DFallback,
        ChildDx12
    };

    inline const wchar_t* AceEngineRenderPathToWide(AceEngineRenderPath path)
    {
        switch (path)
        {
        case AceEngineRenderPath::Dx12ZeroCopy: return L"DX12_ZERO_COPY";
        case AceEngineRenderPath::Dx12GpuComposited: return L"DX12_GPU_COMPOSITED";
        case AceEngineRenderPath::Dx12Readback: return L"DX12_READBACK";
        case AceEngineRenderPath::Dx12CombinedReadback: return L"DX12_COMBINED_READBACK";
        case AceEngineRenderPath::Dx12CachedReadback: return L"DX12_CACHED_READBACK";
        case AceEngineRenderPath::Dx12D2DTextureBridge: return L"DX12_D2D_DEVICE_CONTEXT";
        case AceEngineRenderPath::FailedD2DDeviceContext: return L"FAILED_D2D_DEVICE_CONTEXT";
        case AceEngineRenderPath::CpuD2DFallback: return L"CPU_D2D_FALLBACK";
        case AceEngineRenderPath::ChildDx12: return L"CHILD_DX12";
        default: return L"UNKNOWN";
        }
    }

    inline const char* AceEngineRenderPathToUtf8(AceEngineRenderPath path)
    {
        switch (path)
        {
        case AceEngineRenderPath::Dx12ZeroCopy: return "DX12_ZERO_COPY";
        case AceEngineRenderPath::Dx12GpuComposited: return "DX12_GPU_COMPOSITED";
        case AceEngineRenderPath::Dx12Readback: return "DX12_READBACK";
        case AceEngineRenderPath::Dx12CombinedReadback: return "DX12_COMBINED_READBACK";
        case AceEngineRenderPath::Dx12CachedReadback: return "DX12_CACHED_READBACK";
        case AceEngineRenderPath::Dx12D2DTextureBridge: return "DX12_D2D_DEVICE_CONTEXT";
        case AceEngineRenderPath::FailedD2DDeviceContext: return "FAILED_D2D_DEVICE_CONTEXT";
        case AceEngineRenderPath::CpuD2DFallback: return "CPU_D2D_FALLBACK";
        case AceEngineRenderPath::ChildDx12: return "CHILD_DX12";
        default: return "UNKNOWN";
        }
    }

    struct AceEngineCommandResult
    {
        bool handled = false;
        bool ok = true;
        std::wstring title = L"Tool";
        std::wstring message;
        std::string logTag = "STAT";
        std::string logLine;
        std::wstring warning;
    };

    struct AceEngineStatsSnapshot
    {
        AceEngineRenderPath activeRenderPath = AceEngineRenderPath::Unknown;
        std::wstring backend = L"unknown";
        std::wstring adapterName = L"n/a";
        am::renderer::scene::AceAquariumGpuViewportStats viewportStats{};
        am::renderer::rhi::Dx12GpuAllocationStats rhiStats{};
        am::renderer::rhi::AceViewportTextureResource viewportTexture{};
        am::renderer::rhi::AceViewportTextureBridgeStatus viewportBridge{};
    };

    struct AceEnginePerfSample
    {
        double frameMs = 0.0;
        double uiMs = -1.0;
        double layoutMs = -1.0;
        double aquariumBuildMs = -1.0;
        double rhiRenderMs = -1.0;
        double presentOrCompositeMs = -1.0;
        AceEngineRenderPath fallbackPath = AceEngineRenderPath::Unknown;
    };

    struct AceEnginePerfSummary
    {
        std::size_t count = 0;
        double fpsLast = 0.0;
        double fpsAvg = 0.0;
        double fpsMin = 0.0;
        double fpsMax = 0.0;
        double frameMsLast = 0.0;
        double frameMsAvg = 0.0;
        double frameMsMin = 0.0;
        double frameMsMax = 0.0;
        double uiMsLast = -1.0;
        double layoutMsLast = -1.0;
        double aquariumBuildMsLast = -1.0;
        double rhiRenderMsLast = -1.0;
        double presentOrCompositeMsLast = -1.0;
        AceEngineRenderPath fallbackPath = AceEngineRenderPath::Unknown;
    };

    class AceEnginePerfStats
    {
    public:
        static constexpr std::size_t kMaxSamples = 240;

        void Push(AceEnginePerfSample sample)
        {
            samples_[cursor_] = sample;
            cursor_ = (cursor_ + 1) % samples_.size();
            if (count_ < samples_.size())
            {
                ++count_;
            }
        }

        void SetLastRhiRenderMs(double value) { lastRhiRenderMs_ = value; }
        void SetLastPresentOrCompositeMs(double value) { lastPresentOrCompositeMs_ = value; }
        void SetLastAquariumBuildMs(double value) { lastAquariumBuildMs_ = value; }
        void SetLastFallbackPath(AceEngineRenderPath value) { lastFallbackPath_ = value; }

        double TakeLastRhiRenderMs()
        {
            const double value = lastRhiRenderMs_;
            lastRhiRenderMs_ = -1.0;
            return value;
        }

        double TakeLastPresentOrCompositeMs()
        {
            const double value = lastPresentOrCompositeMs_;
            lastPresentOrCompositeMs_ = -1.0;
            return value;
        }

        double TakeLastAquariumBuildMs()
        {
            const double value = lastAquariumBuildMs_;
            lastAquariumBuildMs_ = -1.0;
            return value;
        }

        AceEngineRenderPath LastFallbackPath() const { return lastFallbackPath_; }

        AceEnginePerfSummary Summary() const
        {
            AceEnginePerfSummary out{};
            out.count = count_;
            if (count_ == 0)
            {
                out.fallbackPath = lastFallbackPath_;
                return out;
            }

            auto sampleAt = [&](std::size_t logicalIndex) -> const AceEnginePerfSample&
            {
                const std::size_t start = (cursor_ + samples_.size() - count_) % samples_.size();
                return samples_[(start + logicalIndex) % samples_.size()];
            };

            const AceEnginePerfSample& last = sampleAt(count_ - 1);
            out.frameMsLast = last.frameMs;
            out.frameMsMin = last.frameMs;
            out.frameMsMax = last.frameMs;
            double totalFrameMs = 0.0;
            out.fpsMin = 1000000.0;
            out.fpsMax = 0.0;

            for (std::size_t i = 0; i < count_; ++i)
            {
                const auto& s = sampleAt(i);
                const double frameMs = s.frameMs > 0.0001 ? s.frameMs : 0.0001;
                const double fps = 1000.0 / frameMs;
                totalFrameMs += s.frameMs;
                out.frameMsMin = std::min(out.frameMsMin, s.frameMs);
                out.frameMsMax = std::max(out.frameMsMax, s.frameMs);
                out.fpsMin = std::min(out.fpsMin, fps);
                out.fpsMax = std::max(out.fpsMax, fps);
            }

            out.frameMsAvg = totalFrameMs / static_cast<double>(count_);
            out.fpsAvg = out.frameMsAvg > 0.0001 ? 1000.0 / out.frameMsAvg : 0.0;
            out.fpsLast = last.frameMs > 0.0001 ? 1000.0 / last.frameMs : 0.0;
            out.uiMsLast = last.uiMs;
            out.layoutMsLast = last.layoutMs;
            out.aquariumBuildMsLast = last.aquariumBuildMs;
            out.rhiRenderMsLast = last.rhiRenderMs;
            out.presentOrCompositeMsLast = last.presentOrCompositeMs;
            out.fallbackPath = last.fallbackPath;
            return out;
        }

    private:
        std::array<AceEnginePerfSample, kMaxSamples> samples_{};
        std::size_t cursor_ = 0;
        std::size_t count_ = 0;
        double lastRhiRenderMs_ = -1.0;
        double lastPresentOrCompositeMs_ = -1.0;
        double lastAquariumBuildMs_ = -1.0;
        AceEngineRenderPath lastFallbackPath_ = AceEngineRenderPath::Unknown;
    };

    inline std::filesystem::path& AceEngineLogPathStorage()
    {
        static std::filesystem::path path = std::filesystem::path("Build") / "Logs" / "ace_engine.log";
        return path;
    }

    inline void AceEngineSetLogPath(std::filesystem::path path)
    {
        AceEngineLogPathStorage() = std::move(path);
    }

    inline std::filesystem::path AceEngineLogPath()
    {
        return AceEngineLogPathStorage();
    }

    inline std::string AceEngineTimestamp()
    {
        const auto now = std::chrono::system_clock::now();
        const auto t = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
#if defined(_WIN32)
        localtime_s(&tm, &t);
#else
        localtime_r(&t, &tm);
#endif
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

    inline std::vector<std::string> AceEngineReadLogTail(std::size_t maxLines, std::wstring* warning = nullptr)
    {
        std::vector<std::string> lines;
        try
        {
            const auto path = AceEngineLogPath();
            if (!std::filesystem::exists(path))
            {
                return lines;
            }

            std::ifstream file(path);
            if (!file)
            {
                if (warning) { *warning = L"Could not open Build/Logs/ace_engine.log for read."; }
                return lines;
            }

            std::string line;
            while (std::getline(file, line))
            {
                lines.push_back(line);
                if (maxLines > 0 && lines.size() > maxLines)
                {
                    lines.erase(lines.begin());
                }
            }
        }
        catch (...)
        {
            if (warning) { *warning = L"Could not read Build/Logs/ace_engine.log."; }
        }
        return lines;
    }

    inline bool AceEngineAppendLog(const std::string& tag, const std::string& line, std::wstring* warning = nullptr)
    {
        try
        {
            const auto path = AceEngineLogPath();
            std::filesystem::create_directories(path.parent_path());
            std::ofstream file(path, std::ios::app);
            if (!file)
            {
                if (warning) { *warning = L"Could not open Build/Logs/ace_engine.log for append."; }
                return false;
            }
            file << '[' << AceEngineTimestamp() << "] [" << tag << "] " << line << '\n';
            return true;
        }
        catch (...)
        {
            if (warning) { *warning = L"Could not write Build/Logs/ace_engine.log."; }
            return false;
        }
    }

    inline bool AceEngineAppendLogLines(const std::string& tag, const std::vector<std::string>& lines, std::wstring* warning = nullptr)
    {
        if (lines.empty())
        {
            return true;
        }

        try
        {
            const auto path = AceEngineLogPath();
            std::filesystem::create_directories(path.parent_path());
            std::ofstream file(path, std::ios::app);
            if (!file)
            {
                if (warning) { *warning = L"Could not open Build/Logs/ace_engine.log for append."; }
                return false;
            }

            const std::string timestamp = AceEngineTimestamp();
            for (const auto& line : lines)
            {
                file << '[' << timestamp << "] [" << tag << "] " << line << '\n';
            }
            return true;
        }
        catch (...)
        {
            if (warning) { *warning = L"Could not write multi-line Build/Logs/ace_engine.log entry."; }
            return false;
        }
    }
}
