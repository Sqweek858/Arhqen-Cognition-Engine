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
Test-Marker "ui7_draw_command_buffer_exists" "Source/Public/ArhqenCognitionEngine/Ui/D2D/D2DDrawCommandBuffer.h" "D2DDrawCommandBuffer"
Test-Marker "ui7_command_layers_exist" "Source/Public/ArhqenCognitionEngine/Ui/D2D/D2DDrawCommandBuffer.h" "layer"
Test-Marker "ui7_shell_begins_draw_frame" "Source/Private/Ui/AceShellUi.cpp" "BeginFrame"
if ($script:failed) { exit 1 }
