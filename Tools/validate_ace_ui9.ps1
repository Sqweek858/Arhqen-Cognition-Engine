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
Test-Marker "ui9_debug_overlay_exists" "Source/Public/ArhqenCognitionEngine/Ui/D2D/D2DUiDebugOverlay.h" "D2DUiDebugOverlay"
Test-Marker "ui9_f9_toggle_wired" "Source/Private/Ui/AceShellUi.cpp" "VK_F9"
Test-Marker "ui9_command_palette_wired" "Source/Private/Ui/AceShellUi.cpp" "ui_debug"
if ($script:failed) { exit 1 }
