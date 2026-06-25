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
Test-Marker "aqui1_telemetry_widgets_exist" "Source/Public/ArhqenCognitionEngine/Ui/D2D/D2DAquariumTelemetryWidgets.h" "D2DAquariumTelemetryWidgets"
Test-Marker "aqui1_bars_exist" "Source/Private/Ui/D2D/D2DAquariumTelemetryWidgets.cpp" "RenderBar"
Test-Marker "aqui1_render_wired" "Source/Private/Ui/AceShellUi.cpp" "aquariumTelemetryWidgets_"
if ($script:failed) { exit 1 }
