#include "ArhqenCognitionEngine/Ui/D2D/AceD2DCompositorAudit.h"

#include <sstream>

namespace am::ui
{
    void AceD2DCompositorAudit::Reset()
    {
        runs_ = 0;
        passed_ = 0;
        failed_ = 0;
        lastReport_ = {};
    }

    AceD2DCompositorAuditReport AceD2DCompositorAudit::Run(const AceD2DCompositorAuditInput& input)
    {
        ++runs_;
        AceD2DCompositorAuditReport report{};
        if (!input.hasSwapChain)
        {
            AddIssue(report, AceD2DCompositorAuditSeverity::Error, "NO_SWAPCHAIN", "D2D DeviceContext compositor requires a DXGI swapchain target.");
        }
        if (!input.hasDeviceContext)
        {
            AddIssue(report, AceD2DCompositorAuditSeverity::Error, "NO_DEVICE_CONTEXT", "D2D DeviceContext compositor has no live ID2D1DeviceContext.");
        }
        if (input.readbackActive)
        {
            AddIssue(report, AceD2DCompositorAuditSeverity::Error, "READBACK_ACTIVE", "Viewport path regressed to CPU readback.");
        }
        if (input.legacyFallback)
        {
            AddIssue(report, AceD2DCompositorAuditSeverity::Error, "LEGACY_FALLBACK", "Legacy HwndRenderTarget fallback is not allowed in VTBRIDGE5.");
        }
        if (input.viewportActive && !input.fullFrameRedraw)
        {
            AddIssue(report, AceD2DCompositorAuditSeverity::Error, "PARTIAL_VIEWPORT_FRAME", "Viewport element is active but frame plan is not full-frame.");
        }
        if (input.fastPartialPaintAllowed && input.hasSwapChain)
        {
            AddIssue(report, AceD2DCompositorAuditSeverity::Error, "FAST_PARTIAL_ON_FLIP", "Fast partial repaint is forbidden on the flip-model DeviceContext path.");
        }
        if (input.bridgeStats.sameSlotDraws > 1 && input.viewportActive)
        {
            AddIssue(report, AceD2DCompositorAuditSeverity::Warning, "SAME_SLOT_DRAWS", "Shared viewport bridge drew from the slot being written more than once.");
        }
        if (input.slateStats.viewportElements == 0 && input.viewportActive)
        {
            AddIssue(report, AceD2DCompositorAuditSeverity::Warning, "NO_VIEWPORT_ELEMENT", "Viewport is active but Slate-style element list has no viewport element.");
        }
        if (input.slateStats.layerValidationFailures > 0)
        {
            AddIssue(report, AceD2DCompositorAuditSeverity::Warning, "LAYER_ORDER", "Slate-style element layer validation reported ordering issues.");
        }
        report.passed = report.errors == 0;
        std::ostringstream summary;
        summary << "audit"
            << ";frame=" << input.frameNumber
            << ";passed=" << (report.passed ? "true" : "false")
            << ";warnings=" << report.warnings
            << ";errors=" << report.errors
            << ";issues=" << report.issues.size();
        for (const auto& issue : report.issues)
        {
            summary << ';' << issue.code << '=' << AceD2DCompositorAuditSeverityName(issue.severity);
        }
        report.summary = summary.str();
        if (report.passed)
        {
            ++passed_;
        }
        else
        {
            ++failed_;
        }
        lastReport_ = report;
        return report;
    }

    void AceD2DCompositorAudit::AddIssue(AceD2DCompositorAuditReport& report, AceD2DCompositorAuditSeverity severity, const char* code, const std::string& message) const
    {
        AceD2DCompositorAuditIssue issue{};
        issue.severity = severity;
        issue.code = code ? code : "UNKNOWN";
        issue.message = message;
        report.issues.push_back(issue);
        if (severity == AceD2DCompositorAuditSeverity::Error)
        {
            ++report.errors;
        }
        else if (severity == AceD2DCompositorAuditSeverity::Warning)
        {
            ++report.warnings;
        }
    }

    std::string AceD2DCompositorAudit::Diagnostics() const
    {
        std::ostringstream oss;
        oss << "runs=" << runs_
            << ";passed=" << passed_
            << ";failed=" << failed_
            << ";last={" << lastReport_.summary << "}";
        return oss.str();
    }

    std::wstring AceD2DCompositorAudit::WideDiagnostics() const
    {
        const std::string text = Diagnostics();
        return std::wstring(text.begin(), text.end());
    }

    const char* AceD2DCompositorAuditSeverityName(AceD2DCompositorAuditSeverity severity)
    {
        switch (severity)
        {
        case AceD2DCompositorAuditSeverity::Info: return "info";
        case AceD2DCompositorAuditSeverity::Warning: return "warning";
        case AceD2DCompositorAuditSeverity::Error: return "error";
        default: return "unknown";
        }
    }
}
