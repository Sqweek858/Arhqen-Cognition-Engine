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
Test-Marker "ui8_invalidation_root_exists" "Source/Public/ArhqenCognitionEngine/Ui/Core/AceUiInvalidationRoot.h" "AceUiInvalidationRoot"
Test-Marker "ui8_dirty_flags_exist" "Source/Public/ArhqenCognitionEngine/Ui/Core/AceUiInvalidationRoot.h" "AceUiDirtyReason"
Test-Marker "ui8_invalidate_rect_marks_dirty" "Source/Private/Ui/AceShellUi.cpp" "MarkRect"
if ($script:failed) { exit 1 }
