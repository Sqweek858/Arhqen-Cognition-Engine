$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$Header = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\Ui\AceShellUi.h") -Raw
$Cpp = Get-Content (Join-Path $Root "Source\Private\Ui\AceShellUi.cpp") -Raw

$Markers = @{
    "static_cards_not_scrollbars" = "static cards stay static"
    "strong_frosted_veil" = "strong frosted veil"
    "scrollable_logs" = "aquariumLogScroll_"
    "scrollable_debug" = "aquariumDebugScroll_"
    "scrollable_counterfactual" = "aquariumCounterfactualScroll_"
    "scrollable_metrics" = "aquariumMetricsScroll_"
    "scrollbar_renderer" = "renderAquariumScrollbar"
    "scrollbar_drag" = "beginAquariumScrollbarDrag"
    "mouse_wheel" = "handleAquariumWheel"
    "clipping" = "PushAxisAlignedClip"
    "strong_modal_alpha" = "modalGlass.fillAlpha = 0.72f"
    "strong_modal_glow" = "modalGlass.glowAlpha = 0.82f"
}

foreach ($name in $Markers.Keys) {
    $needle = $Markers[$name]
    if (($Header -notlike "*$needle*") -and ($Cpp -notlike "*$needle*")) {
        throw "Missing ACE-UI1R1 marker: $name -> $needle"
    }
    Write-Host "PASS|$name|$needle"
}

if ($Cpp -like "*Panda3D*" -or $Cpp -like "*DearPyGui*" -or $Cpp -like "*python*" -or $Cpp -like "*Python*") {
    throw "Forbidden non-goal reference found in UI patch source."
}

Write-Host "PASS|ace_ui1r1_static_validation|strong frost + real scroll markers present"
