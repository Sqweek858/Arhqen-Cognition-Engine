#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

static std::string Read(const fs::path& p)
{
    std::ifstream in(p, std::ios::binary);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

static bool Has(const std::string& haystack, const std::string& needle)
{
    return haystack.find(needle) != std::string::npos;
}

static bool Check(const char* name, bool pass)
{
    std::cout << (pass ? "PASS|" : "FAIL|") << name << "\n";
    return pass;
}

int main()
{
    const std::string shell = Read("Source/Private/Ui/AceShellUi.cpp");
    const std::string shellH = Read("Source/Public/ArhqenCognitionEngine/Ui/AceShellUi.h");
    const std::string docs = Read("Docs/ACE_UI12_VIEWPORT_OVERLAY_POLISH.md");
    const std::string layeringDocs = Read("Docs/ACE_AQ3D15_VIEWPORT_LAYERING.md");

    bool ok = true;
    ok &= Check("ui12_docs_exist", fs::exists("Docs/ACE_UI12_VIEWPORT_OVERLAY_POLISH.md") && Has(docs, "Stable Viewport Overlay Layering"));
    ok &= Check("ui12_slate_principle_documented", Has(docs, "SViewport") && Has(docs, "viewport draw element") && Has(layeringDocs, "UE-style Viewport Layering"));
    ok &= Check("ui12_viewport_local_overlay_state", Has(shell, "isViewportLocalOverlayActive") && Has(shell, "engineLogOverlayVisible_") && Has(shell, "commandPalette_.active()"));
    ok &= Check("ui12_parent_composited_hold", Has(shellH, "aquariumParentCompositedHoldFrames_") && Has(shell, "requestParentCompositedViewportHold") && Has(shell, "overlay-open") && Has(shell, "overlay-close"));
    ok &= Check("ui12_deferred_dcomp_reset", Has(shellH, "aquariumResetDirectCompositionAfterPaint_") && Has(shell, "defer DirectComposition teardown") && Has(shell, "after EndDraw") && Has(shell, "resetAquariumDirectCompositionIfActive();"));
    ok &= Check("ui12_no_toast_on_backtick_toggle", Has(shell, "no toast on console toggle") && !Has(shell, "Docked engine log console opened. Press ` to close."));
    ok &= Check("ui12_viewport_hud_layer", Has(shell, "renderAquariumViewportHudLayer") && Has(shell, "viewport HUD is a stable layer") && Has(shell, "aquariumTelemetryWidgets_.RenderPanel"));
    ok &= Check("ui12_telemetry_avoids_log_console", Has(shell, "computeAquariumTelemetryOverlayRect") && Has(shell, "avoidOverlapsBottomHud") && Has(shell, "avoidRect.top - 12.0f"));
    ok &= Check("ui12_log_console_local_input_preserved", Has(shell, "submitEngineLogOverlayInput") && Has(shell, "backend_submit=false") && Has(shell, "source=engine_log_overlay"));
    ok &= Check("ui12_child_hwnd_clip_not_reintroduced", !Has(shell, "DX12 child viewport clipped by engine log UI") && !Has(shell, "overlay.top - 8.0f"));
    ok &= Check("ui12_r1_parent_composited_while_hud_visible", Has(shell, "ACE-PERF2") && Has(shell, "buildAquariumGpuViewportOverlay") && Has(docs, "GPU-composited viewport overlays"));
    ok &= Check("ui12_r1_no_dcomp_reactivation_on_rmb", Has(shell, "Global parent overlays") && Has(docs, "RMB/WASD"));

    std::cout << (ok ? "PASS|ace_ui12_viewport_overlay_probe" : "FAIL|ace_ui12_viewport_overlay_probe") << "\n";
    return ok ? 0 : 1;
}
