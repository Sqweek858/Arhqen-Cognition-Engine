$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$Header = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\Ui\AceShellUi.h") -Raw
$Cpp = Get-Content (Join-Path $Root "Source\Private\Ui\AceShellUi.cpp") -Raw

$Markers = @{
    "environment_workspace_panel" = "Environment is an actual workspace panel"
    "environment_hides_chat_layer" = "while Environment is open"
    "button_rects_visible" = "button hit rects are computed"
    "responsive_dashboard_layout" = "responsive dashboard layout"
    "logs_inside_panel" = "never collide with the chat input"
    "no_fake_vignette_bars" = "no fake vignette bars"
    "scrollbar_renderer" = "renderAquariumScrollbar"
    "mouse_wheel" = "handleAquariumWheel"
    "drag_scrollbar" = "beginAquariumScrollbarDrag"
}

foreach ($name in $Markers.Keys) {
    $needle = $Markers[$name]
    if (($Header -notlike "*$needle*") -and ($Cpp -notlike "*$needle*")) {
        throw "Missing ACE-UI1R2 marker: $name -> $needle"
    }
    Write-Host "PASS|$name|$needle"
}

if ($Cpp -like "*ctx.height - 96.0f*" -or $Cpp -like "*0.0f, 0.0f, 72.0f*") {
    throw "Fake vignette bars still present."
}
Write-Host "PASS|fake_vignette_bars_removed"

if ($Cpp -like "*Panda3D*" -or $Cpp -like "*DearPyGui*" -or $Cpp -like "*python*" -or $Cpp -like "*Python*") {
    throw "Forbidden non-goal reference found in UI patch source."
}
Write-Host "PASS|no_forbidden_runtime_refs"

Write-Host "PASS|ace_ui1r2_static_validation|actual layout repair markers present"
