$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$UiCpp = Join-Path $Root "Source\Private\Ui\AceShellUi.cpp"
$UiH = Join-Path $Root "Source\Public\ArhqenCognitionEngine\Ui\AceShellUi.h"
$Doc = Join-Path $Root "Docs\ACE_AQ3D14.md"
$Probe = Join-Path $Root "Tools\AceAq3D14OffscreenViewportProbe.cpp"

function Pass($name) { "PASS|$name" }
function Fail($name, $why) { "FAIL|$name|$why"; exit 1 }
function Require-Text($path, $needle, $name) {
    $text = Get-Content $path -Raw
    if ($text.Contains($needle)) { Pass $name } else { Fail $name "missing '$needle' in $path" }
}

Require-Text $UiH "aquariumUseSingleHwndCompositeViewport_" "single_hwnd_composite_state_exists"
Require-Text $UiCpp "renderAquariumSlateCompositeViewport" "slate_style_composite_render_function_exists"
Require-Text $UiCpp "Single-HWND composite viewport" "three_d_main_path_uses_single_hwnd_composite"
Require-Text $UiCpp "aquariumEmbeddedDx12Viewport_.Hide();" "legacy_child_hwnd_suppressed_in_composite_path"
Require-Text $UiCpp "!aquariumUseSingleHwndCompositeViewport_ && handleFrozenNativeResizeSizing" "native_resize_freeze_disabled_for_composite_path"
Require-Text $UiCpp "return !aquariumUseSingleHwndCompositeViewport_ && environmentOpen_ && aquarium3DModeActive_" "frozen_resize_limited_to_legacy_child_path"
Require-Text $UiCpp "aquariumController_.IsRunning() || windowLiveResizeActive_" "composite_path_repaints_for_run_and_resize"
Require-Text $UiCpp "BuildViewportModel" "composite_path_reuses_aquarium_viewport_model"
Require-Text $UiCpp "BuildPrimitives" "composite_path_reuses_scene_adapter"
Require-Text $UiCpp "aquariumCompositeFrameCount_" "composite_frame_counter_exists"
Require-Text $UiCpp "aquariumLegacyChildSuppressedCount_" "legacy_child_suppressed_counter_exists"
Require-Text $Doc "Slate-style Offscreen Viewport Composite" "docs_aq3d14_exists"
if (Test-Path $Probe) { Pass "aq3d14_probe_exists" } else { Fail "aq3d14_probe_exists" "probe missing" }

$bad = Get-ChildItem -Path $Root -File | Where-Object { $_.Extension -in ".obj",".exe",".pdb",".ilk",".log" }
if ($bad.Count -eq 0) { Pass "no_build_artifacts_in_repo_root" } else { Fail "no_build_artifacts_in_repo_root" ($bad | Select-Object -ExpandProperty Name -join ",") }

Pass "ace_aq3d14_static_validation_complete"
