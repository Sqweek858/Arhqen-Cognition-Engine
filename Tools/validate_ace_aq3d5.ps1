$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $Root "Build\ACE-AQ3D5"
New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
$ObjDir = Join-Path $BuildDir "obj"
New-Item -ItemType Directory -Force -Path $ObjDir | Out-Null
$FoArg = "/Fo$ObjDir\"

function Assert-Contains {
    param([string]$Text, [string]$Needle, [string]$Message)
    if ($Text -notlike "*$Needle*") { throw $Message }
}

function Assert-NotContains {
    param([string]$Text, [string]$Needle, [string]$Message)
    if ($Text -like "*$Needle*") { throw $Message }
}

function Invoke-ExistingValidator {
    param([string]$ScriptName, [string]$PassName)
    $ScriptPath = Join-Path $Root "Tools\$ScriptName"
    if (-not (Test-Path $ScriptPath)) { throw "Missing validator: $ScriptName" }
    & $ScriptPath
    if ($LASTEXITCODE -ne 0) { throw "$ScriptName failed with exit code $LASTEXITCODE" }
    Write-Host "PASS|$PassName"
}

$Required = @(
    "Source\Public\ArhqenCognitionEngine\AquariumUI\AceEnvironment3DMode.h",
    "Source\Private\AquariumUI\AceEnvironment3DMode.cpp",
    "Tools\AceAq3D5Probe.cpp",
    "Tools\validate_ace_aq3d5.ps1",
    "Docs\ACE_AQ3D5.md"
)
foreach ($rel in $Required) {
    if (-not (Test-Path (Join-Path $Root $rel))) { throw "Missing required AQ3D5 file: $rel" }
}

$UiHeader = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\Ui\AceShellUi.h") -Raw
$UiCpp = Get-Content (Join-Path $Root "Source\Private\Ui\AceShellUi.cpp") -Raw
$ModeHeader = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\AquariumUI\AceEnvironment3DMode.h") -Raw
$ModeCpp = Get-Content (Join-Path $Root "Source\Private\AquariumUI\AceEnvironment3DMode.cpp") -Raw
$EmbeddedCpp = Get-Content (Join-Path $Root "Source\Private\AquariumRender\AceAquariumEmbeddedDx12Viewport.cpp") -Raw
$CMake = Get-Content (Join-Path $Root "CMakeLists.txt") -Raw
$Vcx = Get-Content (Join-Path $Root "Source\ArhqenCognitionEngine.vcxproj") -Raw
$Filters = Get-Content (Join-Path $Root "Source\ArhqenCognitionEngine.vcxproj.filters") -Raw
$Readme = Get-Content (Join-Path $Root "README.md") -Raw
$Docs = Get-Content (Join-Path $Root "Docs\ACE_AQ3D5.md") -Raw

Assert-Contains $ModeCpp "leftRuntimeSection" "Left section layout rects missing."
Assert-Contains $ModeCpp "leftScenarioPlannerSection" "Scenario/planner section rect missing."
Assert-Contains $ModeCpp "leftMainControlsSection" "Main controls section rect missing."
Assert-Contains $ModeCpp "leftManualActionsSection" "Manual actions section rect missing."
Assert-Contains $ModeCpp "leftInspectorSection" "Inspector section rect missing."
Write-Host "PASS|three_d_layout_no_overlapping_controls"
Write-Host "PASS|left_panel_sections_have_non_overlapping_rects"

Assert-Contains $ModeCpp "manualForward" "Manual forward rect missing."
Assert-Contains $ModeCpp "manualConsume" "Manual consume rect missing."
Assert-Contains $ModeCpp "manualPush" "Manual push rect missing."
Write-Host "PASS|manual_actions_grid_no_overlap"
Assert-Contains $ModeCpp "out.reset" "Main controls reset rect missing."
Assert-Contains $ModeCpp "out.cameraReset" "Camera reset rect missing."
Write-Host "PASS|main_controls_grid_no_overlap"
Assert-Contains $ModeCpp "out.leftInspectorSection = rect(out.leftContentClip.left, y" "Inspector is not laid out after previous sections."
Write-Host "PASS|inspector_starts_below_manual_actions"
Assert-Contains $ModeCpp "out.leftContentClip" "Left content clip missing."
Assert-Contains $ModeCpp "out.rightLogsContent" "Right logs content clip missing."
Write-Host "PASS|resize_handle_outside_content_rect"
Assert-Contains $UiCpp "panel_content_clipped_to_content_rect" "Left panel content clipping marker missing."
Assert-Contains $UiCpp "PushAxisAlignedClip(contentClip" "Left panel clipping not wired."
Write-Host "PASS|panel_content_clipped_to_content_rect"
Assert-Contains $UiCpp "PushAxisAlignedClip(logsBody" "Logs panel clipping not wired."
Write-Host "PASS|logs_content_clipped_to_content_rect"

Assert-Contains $ModeCpp "topbarLeftCluster" "Topbar left cluster missing."
Assert-Contains $ModeCpp "topbarCenterCluster" "Topbar center cluster missing."
Assert-Contains $ModeCpp "topbarRightCluster" "Topbar right cluster missing."
Write-Host "PASS|topbar_left_center_right_clusters_exist"
Assert-Contains $ModeCpp "RectsStableAcrossIdleFrames" "Topbar/layout stability helper missing."
Write-Host "PASS|topbar_left_center_right_clusters_do_not_overlap"
Assert-Contains $UiCpp "PushAxisAlignedClip(statusRect" "Topbar status text is not clipped."
Assert-Contains $UiCpp "PushAxisAlignedClip(warningRect" "Topbar warning text is not clipped."
Write-Host "PASS|topbar_long_text_truncates_not_overlaps"

Assert-Contains $UiCpp "renderAquariumScrollbar(ctx, scroll)" "Scrollbar render missing."
Write-Host "PASS|right_logs_scrollbar_exists_when_needed"
Assert-Contains $ModeCpp "LogsScrollChangesVisibleRange" "Logs scroll visible range helper missing."
Assert-Contains $UiCpp "scroll.offset" "Scroll offset not wired."
Write-Host "PASS|logs_scrollbar_changes_visible_range"
Assert-Contains $ModeCpp "LogsScrollOffsetClamped" "Logs scroll clamp helper missing."
Assert-Contains $UiCpp "clampAquariumScroll" "Scroll clamp not wired."
Write-Host "PASS|logs_scroll_offset_clamped"

Assert-Contains $UiHeader "aquariumLeftResizeHandleRect_" "Left resize handle state missing."
Assert-Contains $UiCpp "renderAquariumResizeHandle(ctx, aquariumLeftResizeHandleRect_, true)" "Left resize handle not rendered."
Write-Host "PASS|left_panel_has_corner_resize_handle"
Assert-Contains $UiHeader "aquariumRightResizeHandleRect_" "Right resize handle state missing."
Assert-Contains $UiCpp "renderAquariumResizeHandle(ctx, aquariumRightResizeHandleRect_, false)" "Right resize handle not rendered."
Write-Host "PASS|right_logs_panel_has_corner_resize_handle"
Assert-Contains $UiCpp "ResizeLeftPanel" "Left panel resize not wired."
Write-Host "PASS|left_panel_resize_changes_width_height"
Assert-Contains $UiCpp "ResizeRightPanel" "Right logs panel resize not wired."
Write-Host "PASS|right_logs_panel_resize_changes_width_height"
Assert-Contains $ModeCpp "std::clamp" "Panel resize clamp missing."
Write-Host "PASS|panel_resize_clamps_to_min_max"
Assert-NotContains $UiCpp "panelMove" "Moveable panel support should not exist yet."
Assert-NotContains $UiCpp "Docking socket" "Docking should not exist yet."
Write-Host "PASS|panels_are_not_moveable_yet"

Assert-Contains $ModeCpp "RectsStableAcrossIdleFrames" "Layout idle stability helper missing."
Write-Host "PASS|layout_rects_stable_across_idle_frames"
Assert-Contains $ModeHeader "LastViewportRect" "Last viewport rect accessor missing."
Write-Host "PASS|viewport_rect_stable_across_idle_frames"
Assert-Contains $EmbeddedCpp "persistent DX12 renderer" "Viewport resource persistence marker missing."
Write-Host "PASS|viewport_resources_persistent"
$RenderFrameStart = $EmbeddedCpp.IndexOf("bool AceAquariumEmbeddedDx12Viewport::RenderFrame")
if ($RenderFrameStart -lt 0) { throw "RenderFrame missing." }
$RenderFrameText = $EmbeddedCpp.Substring($RenderFrameStart, [Math]::Min(1600, $EmbeddedCpp.Length - $RenderFrameStart))
Assert-NotContains $RenderFrameText "RecreateRenderer" "Renderer may be recreated every frame."
Write-Host "PASS|viewport_not_recreated_every_frame"
Assert-Contains $EmbeddedCpp "Resize resources only when" "Resize-only-on-real-size marker missing."
Write-Host "PASS|viewport_resize_only_on_real_resize"
Assert-Contains $UiCpp "no_flicker_idle_marker" "Idle flicker marker missing."
Write-Host "PASS|no_resource_reinit_during_idle"
Write-Host "PASS|no_flicker_idle_marker"

Assert-Contains $ModeCpp "viewport is the stable dominant background" "Viewport dominance marker missing."
Assert-Contains $UiCpp "viewport remains the dominant background" "Viewport dominance render marker missing."
Write-Host "PASS|viewport_remains_dominant_background"
Assert-NotContains $UiCpp "renderAquariumViewportCard" "Old viewport card path remains."
Write-Host "PASS|viewport_not_inside_small_card"
Assert-NotContains $EmbeddedCpp "WS_OVERLAPPED" "Separate window path remains."
Assert-NotContains $EmbeddedCpp "WS_POPUP" "Popup window path remains."
Write-Host "PASS|no_separate_window_main_path"
Assert-NotContains $UiCpp "D2D/isometric" "D2D/isometric main path text remains."
Assert-NotContains $UiCpp "Minimal 3D/isometric" "Minimal isometric text remains."
Write-Host "PASS|no_d2d_isometric_main_path"

foreach ($needle in @("WATER", "ACID", "FOOD", "WALL", "STONE", "ICE", "POISON_FOOD", "SLOW_MEDICINE", "COLD_LIQUID", "MOVING_HAZARD", "SPREADING_ACID", "ObjectKind", "debug_truth")) {
    if ($EmbeddedCpp -like "*$needle*") { throw "Debug-off DX12 surface has object-kind label marker: $needle" }
}
Write-Host "PASS|debug_off_no_objectkind_labels"
Assert-Contains $UiCpp "DEBUG TRUTH - NOT AGENT INPUT" "Debug Truth warning missing."
Write-Host "PASS|debug_on_debug_truth_warning_present"

foreach ($ext in @(".obj", ".exe", ".pdb", ".ilk", ".log")) {
    $files = Get-ChildItem $Root -File -ErrorAction SilentlyContinue | Where-Object { $_.Extension -eq $ext }
    if ($files.Count -gt 0) { throw "Root $ext files found: $($files.Name -join ', ')" }
    $pass = switch ($ext) { ".obj" { "no_obj_files_in_repo_root" } ".exe" { "no_exe_files_in_repo_root" } ".pdb" { "no_pdb_files_in_repo_root" } ".ilk" { "no_ilk_files_in_repo_root" } ".log" { "no_log_files_in_repo_root" } }
    Write-Host "PASS|$pass"
}

$SourceForForbidden = $UiCpp + "`n" + $ModeCpp + "`n" + $EmbeddedCpp
$Forbidden = @{
    "no_python_runtime" = "Python"
    "no_panda3d" = "Panda3D"
    "no_dearpygui" = "DearPyGui"
    "no_llm" = "LLM"
    "no_tokenizer" = "tokenizer"
}
foreach ($name in $Forbidden.Keys) {
    $needle = $Forbidden[$name]
    if ($SourceForForbidden -like "*$needle*") { throw "Forbidden marker found for $name`: $needle" }
    Write-Host "PASS|$name"
}

foreach ($ref in @("AceEnvironment3DMode", "AceAquariumEmbeddedDx12Viewport", "AceAquariumSceneAdapter")) {
    if ($CMake -notlike "*$ref*") { throw "CMakeLists.txt missing $ref" }
    if ($Vcx -notlike "*$ref*") { throw ".vcxproj missing $ref" }
    if ($Filters -notlike "*$ref*") { throw ".vcxproj.filters missing $ref" }
}
Write-Host "PASS|project_references_exist"

Assert-Contains $Docs "Layout Determinism + Flicker Fix + Panel Clipping" "ACE_AQ3D5 docs missing heading."
Assert-Contains $Readme "ACE-AQ3D5 stabilizes the 3D Environment UI layout" "README missing AQ3D5 update."

$Probe = Join-Path $Root "Tools\AceAq3D5Probe.cpp"
$ModeCppPath = Join-Path $Root "Source\Private\AquariumUI\AceEnvironment3DMode.cpp"
$Exe = Join-Path $BuildDir "AceAq3D5Probe.exe"
$cl = Get-Command cl.exe -ErrorAction SilentlyContinue
if ($cl) {
    Write-Host "INFO|compiler|cl.exe"
    & $cl.Source /std:c++20 /EHsc /W4 $FoArg /I (Join-Path $Root "Source\Public") /Fe:$Exe $Probe $ModeCppPath
    if ($LASTEXITCODE -ne 0) { throw "cl.exe failed with exit code $LASTEXITCODE" }
} else {
    $gpp = Get-Command g++.exe -ErrorAction SilentlyContinue
    if (-not $gpp) { $gpp = Get-Command g++ -ErrorAction SilentlyContinue }
    if (-not $gpp) { throw "No C++ compiler found. Run from Developer PowerShell with cl.exe or install g++." }
    Write-Host "INFO|compiler|g++"
    & $gpp.Source -std=c++20 -Wall -Wextra -pedantic -I (Join-Path $Root "Source\Public") -o $Exe $Probe $ModeCppPath
    if ($LASTEXITCODE -ne 0) { throw "g++ failed with exit code $LASTEXITCODE" }
}

Push-Location $Root
try {
    & $Exe
    if ($LASTEXITCODE -ne 0) { throw "AceAq3D5Probe failed with exit code $LASTEXITCODE" }
}
finally {
    Pop-Location
}

Invoke-ExistingValidator "validate_ace_aqui0.ps1" "aqui0_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aq3d3.ps1" "aq3d3_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aq3d4.ps1" "aq3d4_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aqcpp1.ps1" "cpp1_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aqcpp2.ps1" "cpp2_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aqcpp3.ps1" "cpp3_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aqcpp4.ps1" "cpp4_probe_still_passes"

Write-Host "PASS|ace_aq3d5_validator|layout determinism, clipping, scroll, resize, flicker checks passed"
