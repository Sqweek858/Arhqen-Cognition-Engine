$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$Header = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\Ui\AceShellUi.h") -Raw
$Cpp = Get-Content (Join-Path $Root "Source\Private\Ui\AceShellUi.cpp") -Raw

$Markers = @{
    "logs_focus_state" = "aquariumLogsFocus_"
    "logs_focus_button_rect" = "aquariumLogsFocusRect_"
    "logs_focus_button_render" = "Focus Logs"
    "logs_focus_click" = "aquariumLogsFocus_ = !aquariumLogsFocus_"
    "focus_hides_upper_cards" = "Focus Logs hides upper dashboard cards"
    "full_remaining_area_logs" = "full remaining area belongs to Logs"
    "focus_log_top" = "const float logTop = aquariumLogsFocus_ ? top"
    "focus_min_log_height" = "const float minLogH = aquariumLogsFocus_ ? 220.0f"
    "logs_scrollable" = "renderAquariumScrollableLines(ctx, L`"Logs / Episodes`""
}

foreach ($name in $Markers.Keys) {
    $needle = $Markers[$name]
    if (($Header -notlike "*$needle*") -and ($Cpp -notlike "*$needle*")) {
        throw "Missing ACE-UI1R6 marker: $name -> $needle"
    }
    Write-Host "PASS|$name|$needle"
}

if ($Cpp -like "*Logs Focus Mode - dashboard cards hidden*") {
    throw "Focus mode banner still present; it steals log space."
}
Write-Host "PASS|no_focus_banner_stealing_log_space"

if ($Cpp -like "*&aquariumMetricsScroll_*") {
    throw "Metrics scroll panel still participates in hit testing."
}
Write-Host "PASS|metrics_removed_from_scroll_hit_testing"

if ($Cpp -like "*Panda3D*" -or $Cpp -like "*DearPyGui*" -or $Cpp -like "*python*" -or $Cpp -like "*Python*") {
    throw "Forbidden non-goal reference found in UI patch source."
}
Write-Host "PASS|no_forbidden_runtime_refs"

Write-Host "PASS|ace_ui1r6_static_validation|logs focus mode markers present"
