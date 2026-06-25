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
Test-Marker "ui11_style_set_exists" "Source/Public/ArhqenCognitionEngine/Ui/Core/AceUiStyleSet.h" "AceUiStyleSet"
Test-Marker "ui11_default_style_exists" "Source/Private/Ui/Core/AceUiStyleSet.cpp" "MakeDefaultArhqen"
Test-Marker "ui11_shell_has_style_set" "Source/Private/Ui/AceShellUi.cpp" "MakeDefaultArhqen"
if ($script:failed) { exit 1 }
