$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$Header = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\Ui\AceShellUi.h") -Raw
$Cpp = Get-Content (Join-Path $Root "Source\Private\Ui\AceShellUi.cpp") -Raw

$Markers = @{
    "centered_dashboard" = "centered dashboard"
    "two_row_command_strip" = "clean two-row command strip"
    "dashboard_below_controls" = "dashboard layout starts below both control rows"
    "hidden_chat_layer" = "while Environment is open"
    "metrics_static" = "renderAquariumLines(ctx, L`"Metrics`""
    "logs_scrollable" = "renderAquariumScrollableLines(ctx, L`"Logs / Episodes`""
    "debug_scrollable" = "renderAquariumScrollableLines(ctx, L`"DEBUG TRUTH - NOT AGENT INPUT`""
    "hit_test_visible_scroll" = "panel->thumb.contains"
    "no_fake_vignette" = "no fake vignette bars"
}

foreach ($name in $Markers.Keys) {
    $needle = $Markers[$name]
    if (($Header -notlike "*$needle*") -and ($Cpp -notlike "*$needle*")) {
        throw "Missing ACE-UI1R3 marker: $name -> $needle"
    }
    Write-Host "PASS|$name|$needle"
}

if ($Cpp -like "*environmentModalRect_ = settingsModalRect_*") {
    throw "Environment still uses settings modal rect."
}
Write-Host "PASS|environment_not_settings_modal"

if ($Cpp -like "*ctx.height - 96.0f*" -or $Cpp -like "*0.0f, 0.0f, 72.0f*") {
    throw "Fake vignette bars still present."
}
Write-Host "PASS|fake_vignette_bars_removed"

if ($Cpp -like "*Panda3D*" -or $Cpp -like "*DearPyGui*" -or $Cpp -like "*python*" -or $Cpp -like "*Python*") {
    throw "Forbidden non-goal reference found in UI patch source."
}
Write-Host "PASS|no_forbidden_runtime_refs"

Write-Host "PASS|ace_ui1r3_static_validation|centered dashboard layout markers present"
