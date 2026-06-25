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
Test-Marker "ui5_foundation" "Source/Public/ArhqenCognitionEngine/Ui/D2D/D2DTextLayoutFoundation.h" "D2DTextLayoutFoundation"
Test-Marker "ui6_retained" "Source/Public/ArhqenCognitionEngine/Ui/Core/AceUiRetainedLayout.h" "AceUiRetainedLayoutTree"
Test-Marker "ui7_commands" "Source/Public/ArhqenCognitionEngine/Ui/D2D/D2DDrawCommandBuffer.h" "D2DDrawCommandBuffer"
Test-Marker "ui8_dirty" "Source/Public/ArhqenCognitionEngine/Ui/Core/AceUiInvalidationRoot.h" "AceUiInvalidationRoot"
Test-Marker "ui9_overlay" "Source/Public/ArhqenCognitionEngine/Ui/D2D/D2DUiDebugOverlay.h" "D2DUiDebugOverlay"
Test-Marker "aqui1_telemetry" "Source/Public/ArhqenCognitionEngine/Ui/D2D/D2DAquariumTelemetryWidgets.h" "D2DAquariumTelemetryWidgets"
Test-Marker "ui11_style" "Source/Public/ArhqenCognitionEngine/Ui/Core/AceUiStyleSet.h" "AceUiStyleSet"
Test-Marker "project_references" "Source/ArhqenCognitionEngine.vcxproj" "D2DTextLayoutFoundation.cpp"
if ($script:failed) { exit 1 }
