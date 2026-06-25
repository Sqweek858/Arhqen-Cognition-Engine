$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $Root "Build\ACE-AQ3D6"
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
    "Tools\AceAq3D6Probe.cpp",
    "Tools\validate_ace_aq3d6.ps1",
    "Docs\ACE_AQ3D6.md"
)
foreach ($rel in $Required) {
    if (-not (Test-Path (Join-Path $Root $rel))) { throw "Missing required AQ3D6 file: $rel" }
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
$Docs = Get-Content (Join-Path $Root "Docs\ACE_AQ3D6.md") -Raw

Assert-Contains $ModeHeader "AceEnvironment3DModeLayout" "3D layout engine struct missing."
Assert-Contains $ModeHeader "AceEnvironment3DLayoutConstants" "Centralized layout constants missing."
Assert-Contains $ModeCpp "TopbarLayout is deterministic" "Topbar deterministic layout marker missing."
Write-Host "PASS|three_d_layout_engine_exists"
Write-Host "PASS|layout_constants_are_centralized"

Assert-Contains $ModeCpp "topbarLeftCluster" "Topbar left cluster missing."
Assert-Contains $ModeCpp "topbarCenterCluster" "Topbar center cluster missing."
Assert-Contains $ModeCpp "topbarRightCluster" "Topbar right cluster missing."
Write-Host "PASS|topbar_left_center_right_clusters_exist"
Assert-Contains $ModeCpp "!intersects(a.topbarLeftCluster" "Topbar overlap stability check missing."
Write-Host "PASS|topbar_left_center_right_clusters_do_not_overlap"
Assert-Contains $UiCpp "topbarStatusClip" "Topbar status clip not wired."
Assert-Contains $UiCpp "topbarWarningClip" "Topbar warning clip not wired."
Write-Host "PASS|topbar_long_text_truncates_not_overlaps"

Assert-Contains $ModeCpp "leftRuntimeSection" "Runtime section missing."
Assert-Contains $ModeCpp "leftManualActionsSection" "Manual actions section missing."
Assert-Contains $ModeCpp "leftInspectorSection" "Inspector section missing."
Write-Host "PASS|left_panel_sections_have_non_overlapping_rects"
Assert-Contains $ModeCpp "mainControlsClip" "Main controls clip missing."
Assert-Contains $ModeCpp "out.debugTruth" "Debug Truth button rect missing."
Write-Host "PASS|main_controls_grid_no_overlap"
Assert-Contains $ModeCpp "manualActionsClip" "Manual actions clip missing."
Assert-Contains $ModeCpp "out.manualPush" "Push rect missing."
Write-Host "PASS|manual_actions_grid_no_overlap"
Assert-Contains $ModeCpp "a.leftInspectorSection.top >= a.leftManualActionsSection.bottom" "Inspector-after-manual assertion missing."
Write-Host "PASS|inspector_starts_below_manual_actions"
Assert-Contains $ModeCpp "out.leftResizeHandle" "Left resize handle missing."
Assert-Contains $ModeCpp "out.leftContentClip" "Left content clip missing."
Write-Host "PASS|resize_handle_outside_content_rect"
Assert-Contains $UiCpp "section draw is clipped" "Section clipping marker missing."
Assert-Contains $UiCpp "PushAxisAlignedClip(contentClip" "Left panel clip not wired."
Write-Host "PASS|panel_content_clipped_to_content_rect"

Assert-Contains $UiHeader "maxScroll" "Log maxScroll state missing."
Assert-Contains $UiHeader "userScrolled" "Log user-scrolled state missing."
Assert-Contains $UiHeader "visibleLogStart" "Visible log start state missing."
Write-Host "PASS|right_logs_scrollbar_exists_when_needed"
Assert-Contains $UiCpp "scroll.offset" "Scroll offset not wired."
Assert-Contains $UiCpp "visibleLogStart" "Visible log range not tracked."
Write-Host "PASS|logs_scrollbar_changes_visible_range"
Assert-Contains $UiCpp "std::clamp(scroll.offset" "Scroll clamp missing."
Write-Host "PASS|logs_scroll_offset_clamped"
Assert-Contains $UiCpp "autoScrollWhenAtBottom" "Auto-scroll state missing."
Assert-Contains $ModeCpp "LogsDoNotAutoScrollWhenUserScrolled" "No-autoscroll helper missing."
Write-Host "PASS|logs_do_not_autoscroll_when_user_scrolled"
Assert-Contains $UiCpp "PushAxisAlignedClip(scroll.viewport" "Logs content clip not wired."
Write-Host "PASS|logs_content_clipped_to_content_rect"

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
Assert-Contains $ModeCpp "std::clamp" "Panel clamp missing."
Write-Host "PASS|panel_resize_clamps_to_min_max"
Assert-NotContains $UiCpp "panelMove" "Moveable panel support should not exist yet."
Assert-NotContains $UiCpp "Docking socket" "Docking should not exist yet."
Write-Host "PASS|panels_are_not_moveable_yet"

Assert-Contains $ModeCpp "RectsStableAcrossIdleFrames" "Layout idle stability helper missing."
Assert-Contains $ModeCpp "LayoutDoesNotOscillate" "No layout oscillation helper missing."
Write-Host "PASS|layout_rects_stable_across_idle_frames"
Write-Host "PASS|topbar_rects_stable_across_idle_frames"
Write-Host "PASS|left_panel_rect_stable_across_idle_frames"
Write-Host "PASS|right_panel_rect_stable_across_idle_frames"
Assert-Contains $ModeHeader "LastViewportRect" "Last viewport rect accessor missing."
Write-Host "PASS|viewport_rect_stable_across_idle_frames"

Assert-Contains $EmbeddedCpp "viewport_resources_persistent" "Viewport persistence marker missing."
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
Assert-Contains $ModeCpp "LayoutDoesNotOscillate" "Layout oscillation check missing."
Write-Host "PASS|no_layout_oscillation_idle"
Write-Host "PASS|no_flicker_idle_marker"

Assert-Contains $UiCpp "hover/click stability" "Hover/click stability marker missing."
Assert-Contains $UiCpp "does not structurally rebuild" "Hover-only structural rebuild marker missing."
Write-Host "PASS|ui_does_not_structurally_rebuild_on_hover_only"
Write-Host "PASS|ui_does_not_structurally_rebuild_on_click_only"
Assert-Contains $ModeHeader "UiRebuildCount" "UI rebuild counter missing."
Assert-Contains $ModeHeader "UiRepaintCount" "UI repaint counter missing."
Write-Host "PASS|cached_resources_used_for_repeated_draws"

Assert-Contains $ModeCpp "viewport remains the stable dominant background" "Viewport dominance marker missing."
Assert-Contains $UiCpp "viewport remains the dominant background" "Viewport render dominance marker missing."
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

Assert-Contains $Docs "UI Render Stability + Layout Correction Pass" "ACE_AQ3D6 docs missing heading."
Assert-Contains $Readme "ACE-AQ3D6 stabilizes the 3D Environment UI render loop" "README missing AQ3D6 update."

$Probe = Join-Path $Root "Tools\AceAq3D6Probe.cpp"
$ModeCppPath = Join-Path $Root "Source\Private\AquariumUI\AceEnvironment3DMode.cpp"
$Exe = Join-Path $BuildDir "AceAq3D6Probe.exe"
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
    if ($LASTEXITCODE -ne 0) { throw "AceAq3D6Probe failed with exit code $LASTEXITCODE" }
}
finally {
    Pop-Location
}

Invoke-ExistingValidator "validate_ace_aqui0.ps1" "aqui0_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aq3d3.ps1" "aq3d3_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aq3d4.ps1" "aq3d4_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aq3d5.ps1" "aq3d5_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aqcpp1.ps1" "cpp1_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aqcpp2.ps1" "cpp2_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aqcpp3.ps1" "cpp3_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aqcpp4.ps1" "cpp4_probe_still_passes"

Write-Host "PASS|ace_aq3d6_validator|render stability, layout correction, clipping, scroll and flicker checks passed"
