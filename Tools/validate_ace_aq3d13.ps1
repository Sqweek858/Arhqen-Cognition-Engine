$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$UiCpp = Join-Path $Root "Source\Private\Ui\AceShellUi.cpp"
$UiH = Join-Path $Root "Source\Public\ArhqenCognitionEngine\Ui\AceShellUi.h"
$Doc = Join-Path $Root "Docs\ACE_AQ3D13.md"

function Pass($name) { "PASS|$name" }
function Fail($name, $why) { "FAIL|$name|$why"; exit 1 }
function Require-Text($path, $needle, $name) {
    $text = Get-Content $path -Raw
    if ($text.Contains($needle)) { Pass $name } else { Fail $name "missing '$needle' in $path" }
}

Require-Text $UiH "frozenNativeLiveResizeActive_" "frozen_native_resize_state_exists"
Require-Text $UiH "pendingNativeLiveResizeRect_" "pending_native_resize_rect_exists"
Require-Text $UiCpp "handleFrozenNativeResizeSizing" "wm_sizing_freeze_handler_exists"
Require-Text $UiCpp "return TRUE;" "wm_sizing_returns_true_when_frozen"
Require-Text $UiCpp "*proposed = frozenNativeLiveResizeRect_" "wm_sizing_returns_frozen_rect"
Require-Text $UiCpp "applyFrozenNativeResizeCommit" "final_native_resize_commit_exists"
Require-Text $UiCpp "SetWindowPos(" "final_commit_uses_setwindowpos"
Require-Text $UiCpp "shouldFreezeNativeLiveResize" "freeze_limited_to_3d_mode"
Require-Text $Doc "Frozen Native Resize Commit" "docs_aq3d13_exists"

$Probe = Join-Path $Root "Tools\AceAq3D13FrozenResizeProbe.cpp"
if (Test-Path $Probe) { Pass "aq3d13_probe_exists" } else { Fail "aq3d13_probe_exists" "probe missing" }

$bad = Get-ChildItem -Path $Root -File | Where-Object { $_.Extension -in ".obj",".exe",".pdb",".ilk",".log" }
if ($bad.Count -eq 0) { Pass "no_build_artifacts_in_repo_root" } else { Fail "no_build_artifacts_in_repo_root" ($bad | Select-Object -ExpandProperty Name -join ",") }

Pass "ace_aq3d13_static_validation_complete"
