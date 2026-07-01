#pragma once

#include "ArhqenCognitionEngine/Ui/D2D/AceD2DFlipFrameCompositor.h"
#include "ArhqenCognitionEngine/Ui/D2D/AceD2DViewportBridgeRuntime.h"
#include "ArhqenCognitionEngine/Ui/Slate/AceSlateWindowRendererPipeline.h"

#include <cstdint>
#include <string>
#include <vector>

namespace am::ui
{
    enum class AceD2DCompositorAuditSeverity : std::uint8_t
    {
        Info,
        Warning,
        Error
    };

    struct AceD2DCompositorAuditIssue
    {
        AceD2DCompositorAuditSeverity severity = AceD2DCompositorAuditSeverity::Info;
        std::string code;
        std::string message;
    };

    struct AceD2DCompositorAuditInput
    {
        AceD2DFramePlan framePlan{};
        AceD2DViewportBridgeRuntimeStats bridgeStats{};
        slate::AceSlatePipelineStats slateStats{};
        bool hasSwapChain = false;
        bool hasDeviceContext = false;
        bool readbackActive = false;
        bool legacyFallback = false;
        bool fastPartialPaintAllowed = false;
        bool viewportActive = false;
        bool fullFrameRedraw = true;
        std::uint64_t frameNumber = 0;
    };

    struct AceD2DCompositorAuditReport
    {
        bool passed = true;
        std::uint64_t warnings = 0;
        std::uint64_t errors = 0;
        std::vector<AceD2DCompositorAuditIssue> issues;
        std::string summary;
    };

    class AceD2DCompositorAudit
    {
    public:
        void Reset();
        AceD2DCompositorAuditReport Run(const AceD2DCompositorAuditInput& input);
        const AceD2DCompositorAuditReport& LastReport() const { return lastReport_; }
        std::string Diagnostics() const;
        std::wstring WideDiagnostics() const;
    private:
        void AddIssue(AceD2DCompositorAuditReport& report, AceD2DCompositorAuditSeverity severity, const char* code, const std::string& message) const;
        std::uint64_t runs_ = 0;
        std::uint64_t passed_ = 0;
        std::uint64_t failed_ = 0;
        AceD2DCompositorAuditReport lastReport_{};
    };

    const char* AceD2DCompositorAuditSeverityName(AceD2DCompositorAuditSeverity severity);
}
