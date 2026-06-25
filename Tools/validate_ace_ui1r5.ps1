$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$Header = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\Ui\AceShellUi.h") -Raw
$Cpp = Get-Content (Join-Path $Root "Source\Private\Ui\AceShellUi.cpp") -Raw

$Markers = @{
    "contained_dashboard_math" = "contained dashboard math"
    "logs_clamped_first" = "Logs are clamped inside"
    "log_top_clamped" = "std::min(row3Bottom + gap, bottom - minLogH)"
    "logs_never_escape" = "Logs must never escape"
    "logs_rect_inside" = "const UiRect logsRect = makeUiRect(left, logTop, right, logBottom)"
    "logs_scrollable" = "renderAquariumScrollableLines(ctx, L`"Logs / Episodes`""
    "debug_scrollable" = "renderAquariumScrollableLines(ctx, L`"DEBUG TRUTH - NOT AGENT INPUT`""
    "upper_cards_shrink" = "upper cards shrink"
}

foreach ($name in $Markers.Keys) {
    $needle = $Markers[$name]
    if (($Header -notlike "*$needle*") -and ($Cpp -notlike "*$needle*")) {
        throw "Missing ACE-UI1R5 marker: $name -> $needle"
    }
    Write-Host "PASS|$name|$needle"
}

if ($Cpp -like "*std::max(logTop + 76.0f, logBottom)*") {
    throw "Old log rectangle expression still present; it can escape the modal."
}
Write-Host "PASS|old_escape_expression_removed"

if ($Cpp -like "*Panda3D*" -or $Cpp -like "*DearPyGui*" -or $Cpp -like "*python*" -or $Cpp -like "*Python*") {
    throw "Forbidden non-goal reference found in UI patch source."
}
Write-Host "PASS|no_forbidden_runtime_refs"

Write-Host "PASS|ace_ui1r5_static_validation|logs contained and scrollable"
