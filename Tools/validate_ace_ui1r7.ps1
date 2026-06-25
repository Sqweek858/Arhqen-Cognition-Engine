$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$Header = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\Ui\AceShellUi.h") -Raw
$Cpp = Get-Content (Join-Path $Root "Source\Private\Ui\AceShellUi.cpp") -Raw

$Markers = @{
    "content_scroll_state" = "aquariumContentScroll_"
    "real_content_viewport" = "real content viewport"
    "controls_fixed_content_clipped" = "Controls stay fixed"
    "content_clip_push" = "PushAxisAlignedClip(aquariumContentScroll_.viewport"
    "content_clip_pop" = "PopAxisAlignedClip"
    "container_scrollbar" = "renderAquariumScrollbar(ctx, aquariumContentScroll_)"
    "content_scroll_hit_target" = "&aquariumContentScroll_"
    "dynamic_log_height" = "logContentH"
    "full_log_line_rendering" = "snapshot.logLines.size() + 2"
}

foreach ($name in $Markers.Keys) {
    $needle = $Markers[$name]
    if (($Header -notlike "*$needle*") -and ($Cpp -notlike "*$needle*")) {
        throw "Missing ACE-UI1R7 marker: $name -> $needle"
    }
    Write-Host "PASS|$name|$needle"
}

if ($Cpp -like "*renderAquariumScrollableLines(ctx, L`"Logs / Episodes`"*") {
    throw "Logs still uses child scrollbar instead of container scroll."
}
Write-Host "PASS|logs_child_scroll_removed"

if ($Cpp -like "*&aquariumMetricsScroll_*" -or $Cpp -like "*&aquariumLogScroll_*" -or $Cpp -like "*&aquariumDebugScroll_*" -or $Cpp -like "*&aquariumCounterfactualScroll_*") {
    throw "Old child scroll panels still participate in hit testing."
}
Write-Host "PASS|only_container_scroll_in_hit_testing"

if ($Cpp -like "*Panda3D*" -or $Cpp -like "*DearPyGui*" -or $Cpp -like "*python*" -or $Cpp -like "*Python*") {
    throw "Forbidden non-goal reference found in UI patch source."
}
Write-Host "PASS|no_forbidden_runtime_refs"

Write-Host "PASS|ace_ui1r7_static_validation|content clip and container scrollbar present"
