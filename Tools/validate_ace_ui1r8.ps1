$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$Header = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\Ui\AceShellUi.h") -Raw
$Cpp = Get-Content (Join-Path $Root "Source\Private\Ui\AceShellUi.cpp") -Raw

$Markers = @{
    "log_end_padding" = "logEndPadding"
    "content_bottom_moves" = "const float contentBottom = contentTop + contentHeight"
    "ui1r8_comment" = "ACE-UI1R8"
    "logs_rect_bottom_inset" = "logBottom - 8.0f"
    "content_clip" = "PushAxisAlignedClip(aquariumContentScroll_.viewport"
    "container_scrollbar" = "renderAquariumScrollbar(ctx, aquariumContentScroll_)"
}

foreach ($name in $Markers.Keys) {
    $needle = $Markers[$name]
    if (($Header -notlike "*$needle*") -and ($Cpp -notlike "*$needle*")) {
        throw "Missing ACE-UI1R8 marker: $name -> $needle"
    }
    Write-Host "PASS|$name|$needle"
}

if ($Cpp -like "*const float contentBottom = viewportTop + contentHeight*") {
    throw "Old anchored contentBottom calculation still present."
}
Write-Host "PASS|old_content_bottom_anchor_removed"

if ($Cpp -like "*Panda3D*" -or $Cpp -like "*DearPyGui*" -or $Cpp -like "*python*" -or $Cpp -like "*Python*") {
    throw "Forbidden non-goal reference found in UI patch source."
}
Write-Host "PASS|no_forbidden_runtime_refs"

Write-Host "PASS|ace_ui1r8_static_validation|log end padding and content bottom fix present"
