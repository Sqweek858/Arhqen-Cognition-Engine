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
Test-Marker "ui6_retained_layout_tree_exists" "Source/Public/ArhqenCognitionEngine/Ui/Core/AceUiRetainedLayout.h" "AceUiRetainedLayoutTree"
Test-Marker "ui6_measure_arrange_hit_test_exists" "Source/Private/Ui/Core/AceUiRetainedLayout.cpp" "HitTest"
Test-Marker "ui6_shell_tracks_retained_layout" "Source/Private/Ui/AceShellUi.cpp" "uiRetainedLayout_"
if ($script:failed) { exit 1 }
