$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
function Test-Marker($name, $path, $marker) {
    $full = Join-Path $root $path
    if ((Test-Path $full) -and ((Get-Content $full -Raw) -like "*$marker*")) {
        "PASS|$name"
    } else {
        "FAIL|$name"
        $script:failed = $true
    }
}
$script:failed = $false
Test-Marker "ui5_text_layout_foundation_exists" "Source/Public/ArhqenCognitionEngine/Ui/D2D/D2DTextLayoutFoundation.h" "D2DTextLayoutFoundation"
Test-Marker "ui5_ellipsis_exists" "Source/Private/Ui/D2D/D2DTextLayoutFoundation.cpp" "EllipsizeToFit"
Test-Marker "ui5_clipping_exists" "Source/Private/Ui/D2D/D2DTextLayoutFoundation.cpp" "PushAxisAlignedClip"
Test-Marker "ui5_command_stats_wired" "Source/Private/Ui/AceShellUi.cpp" "ACE-UI5 text draws"
if ($script:failed) { exit 1 }
