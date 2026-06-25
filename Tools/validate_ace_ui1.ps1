$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot

$Required = @(
    "Source\Public\ArhqenCognitionEngine\Ui\AceShellUi.h",
    "Source\Private\Ui\AceShellUi.cpp",
    "Docs\ACE_UI1.md"
)

foreach ($rel in $Required) {
    if (-not (Test-Path (Join-Path $Root $rel))) {
        throw "Missing required file: $rel"
    }
}

$Header = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\Ui\AceShellUi.h") -Raw
$Cpp = Get-Content (Join-Path $Root "Source\Private\Ui\AceShellUi.cpp") -Raw

$Checks = @{
    "scroll_panel_state" = "AquariumScrollPanel"
    "scrollable_lines_renderer" = "renderAquariumScrollableLines"
    "scrollbar_renderer" = "renderAquariumScrollbar"
    "wheel_handler" = "handleAquariumWheel"
    "drag_start_handler" = "beginAquariumScrollbarDrag"
    "drag_update_handler" = "updateAquariumScrollbarDrag"
    "drag_end_handler" = "endAquariumScrollbarDrag"
    "clip_usage" = "PushAxisAlignedClip"
    "logs_scroll_state" = "aquariumLogScroll_"
    "debug_scroll_state" = "aquariumDebugScroll_"
    "counterfactual_scroll_state" = "aquariumCounterfactualScroll_"
    "metrics_scroll_state" = "aquariumMetricsScroll_"
    "vignette_overlay" = "ACE-UI1: subtle global vignette"
    "frosted_modal_glow" = "D2DCyberEffects::drawBorderGlow(ctx, environmentModalRect_"
}

foreach ($name in $Checks.Keys) {
    $needle = $Checks[$name]
    if (($Header -notlike "*$needle*") -and ($Cpp -notlike "*$needle*")) {
        throw "Missing ACE-UI1 marker: $name -> $needle"
    }
    Write-Host "PASS|$name|$needle"
}

if ($Cpp -like "*Panda3D*" -or $Cpp -like "*DearPyGui*" -or $Cpp -like "*python*" -or $Cpp -like "*Python*") {
    throw "Forbidden non-goal reference found in UI patch source."
}
Write-Host "PASS|no_forbidden_visual_runtime_refs"

Write-Host "PASS|ace_ui1_static_validation|frosted scroll panel markers present"
