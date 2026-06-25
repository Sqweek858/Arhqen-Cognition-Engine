$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $Root "Build\ACE-AQ3D4"
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
    "Tools\AceAq3D4Probe.cpp",
    "Tools\validate_ace_aq3d4.ps1",
    "Docs\ACE_AQ3D4.md"
)
foreach ($rel in $Required) {
    if (-not (Test-Path (Join-Path $Root $rel))) { throw "Missing required AQ3D4 file: $rel" }
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
$Docs = Get-Content (Join-Path $Root "Docs\ACE_AQ3D4.md") -Raw

Assert-Contains $UiCpp "renderAquariumMiniButton(ctx, environmentModalCloseRect_, L\"X\"" "Control Panel is missing X close."
Write-Host "PASS|environment_control_panel_has_x_close"
Assert-NotContains $UiCpp "L\"Flow\"" "Flow tutorial section still exists in UI."
Assert-NotContains $UiCpp "Environment -> Enter 3D Environment" "Control Panel tutorial copy still exists."
Write-Host "PASS|environment_control_panel_no_flow_tutorial"
Assert-Contains $UiCpp "Enter 3D Environment >" "Enter 3D button missing or not promoted."
Write-Host "PASS|environment_control_panel_has_enter_3d_button"
Assert-Contains $UiCpp "L\"Reset\"" "Reset missing."
Assert-Contains $UiCpp "L\"Step\"" "Step missing."
Assert-Contains $UiCpp "snapshot.running ? L\"Pause\" : L\"Run\"" "Run/Pause missing."
Write-Host "PASS|environment_control_panel_has_reset_step_run"
Assert-Contains $UiCpp "renderAquariumScrollableLines(ctx, L\"Logs / Episodes\", snapshot.logLines, logs, aquariumLogScroll_" "Control Panel logs are not wired to scrollable renderer."
Write-Host "PASS|environment_control_panel_logs_scroll_functional"

Assert-Contains $ModeCpp "constexpr float kTopbarHeight = 24.0f" "3D topbar is not thin."
Assert-Contains $UiCpp "renderAquariumMiniButton(ctx, aquariumDetailsToggleRect_" "Details topbar toggle missing."
Write-Host "PASS|three_d_mode_has_thin_topbar"
Assert-Contains $UiCpp "renderAquariumMiniButton(ctx, aquariumLogsToggleRect_" "Logs topbar toggle missing."
Write-Host "PASS|three_d_mode_topbar_not_overcrowded"
$TopbarStart = $UiCpp.IndexOf("D2DGlassEffects::drawGlassPanel(ctx, topbar")
$ManualStart = $UiCpp.IndexOf("Manual Actions", $TopbarStart)
if ($TopbarStart -lt 0 -or $ManualStart -lt 0) { throw "Cannot locate topbar/manual sections." }
$TopbarBlock = $UiCpp.Substring($TopbarStart, $ManualStart - $TopbarStart)
foreach ($needle in @("aquariumManualForwardRect_", "aquariumManualLeftRect_", "aquariumManualRightRect_", "aquariumManualWaitRect_", "aquariumManualTouchRect_", "aquariumManualConsumeRect_", "aquariumManualPushRect_")) {
    if ($TopbarBlock -like "*$needle*") { throw "Manual action found in 3D topbar: $needle" }
}
Write-Host "PASS|manual_actions_not_in_topbar"

Assert-Contains $UiHeader "aquariumLeftPanelRect_" "Left details panel state missing."
Assert-Contains $UiCpp "Details / Controls" "Left details/control panel render missing."
Write-Host "PASS|left_details_panel_exists"
Assert-Contains $UiHeader "aquariumRightLogsPanelRect_" "Right logs panel state missing."
Assert-Contains $UiCpp "renderPanelShell(aquariumRightLogsPanelRect_, L\"Logs / Episodes\")" "Right logs panel render missing."
Write-Host "PASS|right_logs_panel_exists"
Assert-Contains $UiCpp "aquariumDetailsPanelVisible_ = !aquariumDetailsPanelVisible_" "Details toggle not wired."
Write-Host "PASS|details_toggle_shows_hides_left_panel"
Assert-Contains $UiCpp "aquariumLogsPanelVisible_ = !aquariumLogsPanelVisible_" "Logs toggle not wired."
Write-Host "PASS|logs_toggle_shows_hides_right_panel"

Assert-Contains $UiHeader "aquariumLeftResizeHandleRect_" "Left resize handle missing."
Assert-Contains $UiCpp "renderAquariumResizeHandle(ctx, aquariumLeftResizeHandleRect_, true)" "Left resize handle not rendered."
Write-Host "PASS|left_panel_has_corner_resize_handle"
Assert-Contains $UiHeader "aquariumRightResizeHandleRect_" "Right resize handle missing."
Assert-Contains $UiCpp "renderAquariumResizeHandle(ctx, aquariumRightResizeHandleRect_, false)" "Right resize handle not rendered."
Write-Host "PASS|right_logs_panel_has_corner_resize_handle"
Assert-Contains $UiCpp "ResizeLeftPanel" "Left panel resize is not wired."
Write-Host "PASS|left_panel_resize_changes_width_height"
Assert-Contains $UiCpp "ResizeRightPanel" "Right logs panel resize is not wired."
Write-Host "PASS|right_logs_panel_resize_changes_width_height"
Assert-Contains $ModeCpp "ClampPanelState" "Panel resize clamps missing."
Assert-Contains $ModeCpp "std::clamp" "Panel resize clamp implementation missing."
Write-Host "PASS|panel_resize_clamps_to_min_max"
Assert-NotContains $UiCpp "panelMove" "Moveable panel support should not be implemented yet."
Assert-NotContains $UiCpp "Docking socket" "Docking should not be implemented yet."
Write-Host "PASS|panels_are_not_moveable_yet"

Assert-Contains $ModeCpp "DX12 surface is the dominant working background" "Viewport dominance marker missing."
Assert-Contains $UiCpp "renderAquariumDx12ViewportSurface(ctx, viewportSurface" "Viewport surface not rendered as main mode background."
Write-Host "PASS|viewport_remains_dominant_background"
Assert-NotContains $UiCpp "renderAquariumViewportCard" "Old viewport card path still exists."
Assert-NotContains $UiCpp "Open 3D Environment" "Old Open 3D wording remains."
Write-Host "PASS|viewport_not_inside_small_card"
Assert-NotContains $EmbeddedCpp "WS_OVERLAPPED" "Separate window style still exists in embedded viewport."
Assert-NotContains $EmbeddedCpp "WS_POPUP" "Popup window style still exists in embedded viewport."
Write-Host "PASS|no_separate_window_main_path"
Assert-NotContains $UiCpp "D2D/isometric" "D2D/isometric text exists in UI main path."
Assert-NotContains $UiCpp "Minimal 3D/isometric" "Minimal isometric text exists in UI main path."
Write-Host "PASS|no_d2d_isometric_main_path"
$RenderFrameStart = $EmbeddedCpp.IndexOf("bool AceAquariumEmbeddedDx12Viewport::RenderFrame")
if ($RenderFrameStart -lt 0) { throw "RenderFrame missing." }
$RenderFrameText = $EmbeddedCpp.Substring($RenderFrameStart, [Math]::Min(1400, $EmbeddedCpp.Length - $RenderFrameStart))
Assert-NotContains $RenderFrameText "RecreateRenderer" "Renderer may be recreated every frame."
Write-Host "PASS|viewport_not_recreated_every_frame"

Assert-Contains $ModeCpp "LogsScrollChangesVisibleRange" "Logs scroll range probe helper missing."
Assert-Contains $UiCpp "scroll.offset" "Logs scrollbar does not change content offset."
Write-Host "PASS|logs_scrollbar_changes_visible_range"
Assert-NotContains $UiCpp "L\"Flow\"" "Flow section text remains."
Write-Host "PASS|no_flow_tutorial_text"
Assert-NotContains $UiCpp "Embedded Embedded" "Duplicated Embedded text remains."
Write-Host "PASS|no_embedded_embedded_text"
Assert-NotContains $UiCpp "separate DX12 surface" "Separate DX12 surface text remains."
Write-Host "PASS|no_separate_dx12_surface_text"
Assert-NotContains $UiCpp "Minimal 3D/isometric" "Minimal isometric text remains."
Write-Host "PASS|no_minimal_isometric_text"

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

Assert-Contains $Docs "3D Environment UI Layout Rework" "ACE_AQ3D4 docs missing heading."
Assert-Contains $Readme "ACE-AQ3D4 reworks the 3D Environment UI layout" "README missing AQ3D4 update."

$Probe = Join-Path $Root "Tools\AceAq3D4Probe.cpp"
$ModeCppPath = Join-Path $Root "Source\Private\AquariumUI\AceEnvironment3DMode.cpp"
$Exe = Join-Path $BuildDir "AceAq3D4Probe.exe"
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
    if ($LASTEXITCODE -ne 0) { throw "AceAq3D4Probe failed with exit code $LASTEXITCODE" }
}
finally {
    Pop-Location
}

Invoke-ExistingValidator "validate_ace_aqui0.ps1" "aqui0_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aq3d3.ps1" "aq3d3_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aqcpp1.ps1" "cpp1_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aqcpp2.ps1" "cpp2_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aqcpp3.ps1" "cpp3_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aqcpp4.ps1" "cpp4_probe_still_passes"

Write-Host "PASS|ace_aq3d4_validator|layout rework checks passed"
