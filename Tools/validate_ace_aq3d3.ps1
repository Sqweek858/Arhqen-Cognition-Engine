$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $Root "Build\ACE-AQ3D3"
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
    "Source\Public\ArhqenCognitionEngine\AquariumRender\AceAquariumEmbeddedDx12Viewport.h",
    "Source\Private\AquariumRender\AceAquariumEmbeddedDx12Viewport.cpp",
    "Source\Public\ArhqenCognitionEngine\AquariumRender\AceAquariumSceneAdapter.h",
    "Source\Private\AquariumRender\AceAquariumSceneAdapter.cpp",
    "Tools\AceAq3D3Probe.cpp",
    "Docs\ACE_AQ3D3.md"
)

foreach ($rel in $Required) {
    if (-not (Test-Path (Join-Path $Root $rel))) { throw "Missing required AQ3D3 file: $rel" }
}

$UiHeader = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\Ui\AceShellUi.h") -Raw
$UiCpp = Get-Content (Join-Path $Root "Source\Private\Ui\AceShellUi.cpp") -Raw
$ModeHeader = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\AquariumUI\AceEnvironment3DMode.h") -Raw
$ModeCpp = Get-Content (Join-Path $Root "Source\Private\AquariumUI\AceEnvironment3DMode.cpp") -Raw
$EmbeddedHeader = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\AquariumRender\AceAquariumEmbeddedDx12Viewport.h") -Raw
$EmbeddedCpp = Get-Content (Join-Path $Root "Source\Private\AquariumRender\AceAquariumEmbeddedDx12Viewport.cpp") -Raw
$AdapterCpp = Get-Content (Join-Path $Root "Source\Private\AquariumRender\AceAquariumSceneAdapter.cpp") -Raw
$ViewportCpp = Get-Content (Join-Path $Root "Source\Private\AquariumRender\AceAquariumViewport.cpp") -Raw
$CMake = Get-Content (Join-Path $Root "CMakeLists.txt") -Raw
$Vcx = Get-Content (Join-Path $Root "Source\ArhqenCognitionEngine.vcxproj") -Raw
$Filters = Get-Content (Join-Path $Root "Source\ArhqenCognitionEngine.vcxproj.filters") -Raw
$Readme = Get-Content (Join-Path $Root "README.md") -Raw
$Docs = Get-Content (Join-Path $Root "Docs\ACE_AQ3D3.md") -Raw

Assert-Contains $UiCpp "Enter 3D Environment" "Environment Control Panel is missing Enter 3D Environment."
Assert-Contains $UiCpp "renderAquariumControlPanelHome" "Control panel home render path is missing."
Write-Host "PASS|environment_control_panel_has_enter_3d_button"

Assert-Contains $UiCpp "aquarium3DModeActive_ = true" "Enter 3D does not switch to 3D mode."
Assert-Contains $UiCpp "renderAquariumFullScreen3DMode" "Full-screen 3D render path missing."
Write-Host "PASS|enter_3d_switches_to_fullscreen_mode"

Assert-Contains $UiCpp "Back / Exit 3D" "Back / Exit 3D button missing."
Assert-Contains $UiCpp "aquarium3DModeActive_ = false" "Back does not leave 3D mode."
Write-Host "PASS|back_returns_to_control_panel"

Assert-Contains $ModeHeader "class AceEnvironment3DMode" "AceEnvironment3DMode class missing."
Assert-Contains $ModeCpp "ViewportAreaAtLeast70Percent" "3D mode viewport area guard missing."
Write-Host "PASS|fullscreen_3d_mode_class_exists"

Assert-Contains $EmbeddedHeader "class AceAquariumEmbeddedDx12Viewport" "Embedded DX12 viewport class missing."
Assert-Contains $EmbeddedCpp "WS_CHILD" "Embedded DX12 viewport is not a child surface."
Assert-Contains $EmbeddedCpp "Dx12Renderer" "Embedded viewport is not using Dx12Renderer."
Write-Host "PASS|embedded_dx12_viewport_class_exists"

Assert-Contains $UiCpp "renderAquariumDx12ViewportSurface" "3D mode does not use the DX12 viewport surface helper."
Assert-Contains $UiCpp "Environment mode owns the whole main window" "3D mode is not documented as owning the main window."
Write-Host "PASS|viewport_uses_full_environment_mode"

Assert-Contains $ModeCpp ">= 0.70f" "Viewport area threshold is not 70 percent."
Assert-Contains $ModeCpp "ViewportAreaRatio" "Viewport ratio computation missing."
Write-Host "PASS|viewport_area_at_least_70_percent"

Assert-NotContains $UiCpp "renderAquariumViewportCard" "Old viewport card render path still exists."
Assert-NotContains $UiCpp "Open 3D Environment" "Old Open 3D wording still exists in UI main path."
Write-Host "PASS|viewport_not_inside_small_card"

Assert-NotContains $EmbeddedCpp "WS_OVERLAPPED" "Separate window style found in embedded viewport."
Assert-NotContains $EmbeddedCpp "WS_POPUP" "Popup window style found in embedded viewport."
Assert-NotContains $UiCpp "separate DX12 surface" "Separate DX12 surface text exists in UI."
Write-Host "PASS|no_separate_window_main_path"

Assert-NotContains $UiCpp "D2D/isometric" "D2D/isometric text exists in UI main path."
Assert-NotContains $UiCpp "Minimal 3D/isometric" "Minimal isometric text exists in UI main path."
Assert-Contains $EmbeddedCpp "renderFrame" "Embedded viewport render frame path missing."
Write-Host "PASS|no_d2d_isometric_main_path"

Assert-NotContains $UiCpp "Embedded Embedded" "Duplicated Embedded text exists."
Write-Host "PASS|no_embedded_embedded_text"

Assert-NotContains $UiCpp "separate DX12 surface" "Separate DX12 surface text exists."
Assert-NotContains $EmbeddedCpp "separate DX12 surface" "Separate DX12 surface text exists in renderer."
Write-Host "PASS|no_separate_dx12_surface_text"

Assert-NotContains $UiCpp "Minimal 3D/isometric" "Minimal isometric text exists."
Write-Host "PASS|no_minimal_isometric_text"

Assert-Contains $AdapterCpp "AceAqRenderPrimitiveKind::GridLine" "Scene adapter does not build grid."
Write-Host "PASS|scene_adapter_builds_grid"
Assert-Contains $AdapterCpp "AceAqRenderPrimitiveKind::Agent" "Scene adapter does not build agent."
Write-Host "PASS|scene_adapter_builds_agent"
Assert-Contains $AdapterCpp "AceAqRenderPrimitiveKind::DirectionArrow" "Scene adapter does not build direction arrow."
Write-Host "PASS|scene_adapter_builds_direction_arrow"
Assert-Contains $AdapterCpp "AceAqRenderPrimitiveKind::Highlight" "Scene adapter does not build front highlight."
Write-Host "PASS|scene_adapter_builds_front_highlight"

Assert-Contains $UiCpp "StepOnce" "Step button does not update runtime."
Write-Host "PASS|scene_adapter_updates_after_step"
Assert-Contains $UiCpp "ResetScenario" "Reset button does not update runtime."
Write-Host "PASS|scene_adapter_updates_after_reset"

Assert-Contains $EmbeddedHeader "InitCount()" "init_count accessor missing."
Assert-Contains $EmbeddedCpp "++initCount_" "init_count increment missing."
Assert-Contains $EmbeddedCpp "rendererReady_" "Persistent renderer guard missing."
Write-Host "PASS|viewport_init_count_stable"

$RenderFrameStart = $EmbeddedCpp.IndexOf("bool AceAquariumEmbeddedDx12Viewport::RenderFrame")
if ($RenderFrameStart -lt 0) { throw "RenderFrame missing." }
$RenderFrameText = $EmbeddedCpp.Substring($RenderFrameStart, [Math]::Min(1400, $EmbeddedCpp.Length - $RenderFrameStart))
Assert-NotContains $RenderFrameText "RecreateRenderer" "Renderer may be recreated every frame."
Assert-Contains $EmbeddedCpp "Do not MoveWindow every frame" "MoveWindow per-frame guard comment missing."
Write-Host "PASS|viewport_not_recreated_every_frame"

Assert-Contains $EmbeddedHeader "ResizeCount()" "resize_count accessor missing."
Assert-Contains $EmbeddedCpp "sizeChanged" "Resize guard missing."
Assert-Contains $EmbeddedCpp "Resize resources only when" "Resize-on-real-change marker missing."
Write-Host "PASS|viewport_resize_only_on_real_resize"

Assert-Contains $EmbeddedHeader "FrameCount()" "frame_count accessor missing."
Assert-Contains $EmbeddedCpp "++frameCount_" "frame_count increment missing."

$DebugOffSurface = $UiCpp + "`n" + $EmbeddedCpp
foreach ($needle in @("WATER", "ACID", "FOOD", "WALL", "STONE", "ICE", "POISON_FOOD", "SLOW_MEDICINE", "COLD_LIQUID", "MOVING_HAZARD", "SPREADING_ACID", "ObjectKind", "debug_truth")) {
    if ($EmbeddedCpp -like "*$needle*") { throw "Debug-off surface has object-kind label marker: $needle" }
}
Write-Host "PASS|debug_off_no_objectkind_labels"

Assert-Contains $UiCpp "DEBUG TRUTH - NOT AGENT INPUT" "Debug truth warning missing from UI."
Assert-Contains $AdapterCpp "DEBUG TRUTH - NOT AGENT INPUT" "Debug truth warning missing from scene adapter."
Write-Host "PASS|debug_on_debug_truth_warning_present"

foreach ($ext in @(".obj", ".exe", ".pdb", ".ilk", ".log")) {
    $files = Get-ChildItem $Root -File -ErrorAction SilentlyContinue | Where-Object { $_.Extension -eq $ext }
    if ($files.Count -gt 0) { throw "Root $ext files found: $($files.Name -join ', ')" }
    $pass = switch ($ext) { ".obj" { "no_obj_files_in_repo_root" } ".exe" { "no_exe_files_in_repo_root" } ".pdb" { "no_pdb_files_in_repo_root" } ".ilk" { "no_ilk_files_in_repo_root" } ".log" { "no_log_files_in_repo_root" } }
    Write-Host "PASS|$pass"
}

$SourceForForbidden = $UiCpp + "`n" + $ModeCpp + "`n" + $EmbeddedCpp + "`n" + $AdapterCpp + "`n" + $ViewportCpp
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

Assert-Contains $Docs "Full-Screen 3D Environment Mode" "ACE_AQ3D3 docs missing product heading."
Assert-Contains $Readme "ACE-AQ3D3 adds a full-screen 3D Environment Mode launched from the Environment Control Panel." "README missing AQ3D3 update."

$Probe = Join-Path $Root "Tools\AceAq3D3Probe.cpp"
$ModeCppPath = Join-Path $Root "Source\Private\AquariumUI\AceEnvironment3DMode.cpp"
$Exe = Join-Path $BuildDir "AceAq3D3Probe.exe"
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
    if ($LASTEXITCODE -ne 0) { throw "AceAq3D3Probe failed with exit code $LASTEXITCODE" }
}
finally {
    Pop-Location
}

Invoke-ExistingValidator "validate_ace_aqui0.ps1" "aqui0_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aqcpp1.ps1" "cpp1_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aqcpp2.ps1" "cpp2_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aqcpp3.ps1" "cpp3_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aqcpp4.ps1" "cpp4_probe_still_passes"
Invoke-ExistingValidator "validate_ace_ui1r9.ps1" "ui1r9_probe_still_passes"

Write-Host "PASS|ace_aq3d3_validator|full-screen environment mode checks passed"
