$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $Root "Build\ACE-AQ3D7"
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
    "Tools\AceAq3D7FlickerProbe.cpp",
    "Tools\validate_ace_aq3d7.ps1",
    "Docs\ACE_AQ3D7.md"
)
foreach ($rel in $Required) {
    if (-not (Test-Path (Join-Path $Root $rel))) { throw "Missing required AQ3D7 file: $rel" }
}

$NativeWindow = Get-Content (Join-Path $Root "Source\Private\Renderer\NativeWindow.cpp") -Raw
$UiHeader = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\Ui\AceShellUi.h") -Raw
$UiCpp = Get-Content (Join-Path $Root "Source\Private\Ui\AceShellUi.cpp") -Raw
$EmbeddedHeader = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\AquariumRender\AceAquariumEmbeddedDx12Viewport.h") -Raw
$EmbeddedCpp = Get-Content (Join-Path $Root "Source\Private\AquariumRender\AceAquariumEmbeddedDx12Viewport.cpp") -Raw
$Docs = Get-Content (Join-Path $Root "Docs\ACE_AQ3D7.md") -Raw

Assert-NotContains $NativeWindow "CS_HREDRAW" "Native window still references CS_HREDRAW."
Assert-NotContains $NativeWindow "CS_VREDRAW" "Native window still references CS_VREDRAW."
Assert-Contains $NativeWindow "wc.style = 0" "Native window class style is not zero."
Write-Host "PASS|native_window_no_hredraw_vredraw"

Assert-NotContains $NativeWindow "COLOR_WINDOW" "Native window still uses classic COLOR_WINDOW background brush."
Assert-Contains $NativeWindow "wc.hbrBackground = nullptr" "Native window background brush is not null."
Write-Host "PASS|native_window_no_background_brush"

Assert-Contains $NativeWindow "case WM_ERASEBKGND" "Native window does not handle WM_ERASEBKGND."
Assert-Contains $NativeWindow "return 1" "WM_ERASEBKGND does not return 1."
Write-Host "PASS|wm_erasebkgnd_handled"

Assert-Contains $UiHeader "invalidateRect" "AceShellUi partial invalidate helper missing from header."
Assert-Contains $UiCpp "RedrawWindow(parent_, &r" "Partial RedrawWindow helper missing."
Assert-Contains $UiCpp "RDW_INVALIDATE | RDW_NOERASE | RDW_NOCHILDREN" "Dirty rect RedrawWindow flags are wrong."
Write-Host "PASS|ui_has_partial_invalidate_rect"

Assert-Contains $UiCpp "hoverInvalidationCount_" "Hover invalidation counter missing."
Assert-Contains $UiCpp "invalidateRect(inflateRect(aquariumHotRectById(oldHot)" "Old hover rect not invalidated partially."
Assert-Contains $UiCpp "invalidateRect(inflateRect(aquariumHotRectById(aquariumHoverHotId_)" "New hover rect not invalidated partially."
Write-Host "PASS|hover_uses_partial_invalidation"

$MouseMoveStart = $UiCpp.IndexOf("case WM_MOUSEMOVE:")
if ($MouseMoveStart -lt 0) { throw "WM_MOUSEMOVE handler missing." }
$MouseMoveText = $UiCpp.Substring($MouseMoveStart, [Math]::Min(4200, $UiCpp.Length - $MouseMoveStart))
Assert-NotContains $MouseMoveText "InvalidateRect(parent_, nullptr" "Hover path still full-invalidates with InvalidateRect."
Write-Host "PASS|hover_does_not_full_invalidate"
Assert-NotContains $MouseMoveText "SetWindowPos" "Hover path moves child viewport."
Assert-NotContains $MouseMoveText "MoveWindow" "Hover path moves child viewport."
Write-Host "PASS|hover_does_not_resize_child_viewport"
Assert-NotContains $MouseMoveText "RecreateRenderer" "Hover path recreates renderer."
Write-Host "PASS|hover_does_not_recreate_renderer"

$RenderSurfaceStart = $UiCpp.IndexOf("void AceShellUi::renderAquariumDx12ViewportSurface")
$SyncStart = $UiCpp.IndexOf("void AceShellUi::syncAquariumEmbeddedViewportWindow")
if ($RenderSurfaceStart -lt 0 -or $SyncStart -lt 0) { throw "Render/sync viewport functions missing." }
$RenderSurfaceText = $UiCpp.Substring($RenderSurfaceStart, $SyncStart - $RenderSurfaceStart)
Assert-NotContains $RenderSurfaceText "ShowWindow(" "Render path calls ShowWindow."
Assert-NotContains $RenderSurfaceText "SetWindowPos(" "Render path calls SetWindowPos."
Assert-NotContains $RenderSurfaceText "MoveWindow(" "Render path calls MoveWindow."
Assert-NotContains $RenderSurfaceText "aquariumEmbeddedDx12Viewport_.Show(" "Render path calls child Show."
Assert-NotContains $RenderSurfaceText "aquariumEmbeddedDx12Viewport_.Hide(" "Render path calls child Hide."
Assert-Contains $UiCpp "syncAquariumEmbeddedViewportWindow()" "Child HWND sync function not wired."
Write-Host "PASS|child_hwnd_sync_not_called_from_render_path"

Assert-Contains $EmbeddedHeader "pendingResize_" "Embedded viewport pending resize state missing."
Assert-Contains $EmbeddedCpp "QueueResize" "Embedded viewport queue resize helper missing."
Assert-Contains $EmbeddedCpp "ApplyPendingResizeIfNeeded" "Embedded viewport pending resize apply helper missing."
Write-Host "PASS|viewport_resize_is_debounced_or_pending"

$WmSizeStart = $EmbeddedCpp.IndexOf("case WM_SIZE:")
if ($WmSizeStart -lt 0) { throw "Embedded viewport WM_SIZE missing." }
$WmSizeText = $EmbeddedCpp.Substring($WmSizeStart, [Math]::Min(700, $EmbeddedCpp.Length - $WmSizeStart))
Assert-Contains $WmSizeText "QueueResize" "WM_SIZE does not queue resize."
Assert-NotContains $WmSizeText "RecreateRenderer" "WM_SIZE directly recreates renderer."
Write-Host "PASS|renderer_recreate_not_directly_spammed_from_wm_size"

Assert-Contains $UiCpp "RDW_INVALIDATE | RDW_NOERASE | RDW_NOCHILDREN" "RedrawWindow is missing NOERASE/NOCHILDREN flags."
Write-Host "PASS|redrawwindow_uses_noerase_nochildren"

foreach ($needle in @("parentPaintCount_", "fullInvalidationCount_", "partialInvalidationCount_", "hoverInvalidationCount_", "childShowCount_", "childHideCount_", "childMoveCount_", "childResizeCount_", "rendererRecreateCount_")) {
    if (($UiHeader + $EmbeddedHeader + $UiCpp + $EmbeddedCpp) -notlike "*$needle*") { throw "Missing diagnostic counter: $needle" }
}
Write-Host "PASS|diagnostic_counters_exist"

Assert-NotContains $UiCpp "InvalidateRect(parent_, nullptr" "Full InvalidateRect(parent_, nullptr) remains."
Write-Host "PASS|no_full_invalidate_rect_parent_null"

foreach ($ext in @(".obj", ".exe", ".pdb", ".ilk", ".log")) {
    $files = Get-ChildItem $Root -File -ErrorAction SilentlyContinue | Where-Object { $_.Extension -eq $ext }
    if ($files.Count -gt 0) { throw "Root $ext files found: $($files.Name -join ', ')" }
    $pass = switch ($ext) { ".obj" { "no_obj_files_in_repo_root" } ".exe" { "no_exe_files_in_repo_root" } ".pdb" { "no_pdb_files_in_repo_root" } ".ilk" { "no_ilk_files_in_repo_root" } ".log" { "no_log_files_in_repo_root" } }
    Write-Host "PASS|$pass"
}

foreach ($needle in @("Python", "Panda3D", "DearPyGui", "LLM", "tokenizer")) {
    if (($UiCpp + "`n" + $EmbeddedCpp) -like "*$needle*") { throw "Forbidden dependency marker found: $needle" }
}
Write-Host "PASS|no_python_runtime"
Write-Host "PASS|no_panda3d"
Write-Host "PASS|no_dearpygui"
Write-Host "PASS|no_llm"
Write-Host "PASS|no_tokenizer"

Assert-Contains $Docs "Flicker Fix / Dirty Rects / Stable Child HWND" "AQ3D7 docs heading missing."

$Probe = Join-Path $Root "Tools\AceAq3D7FlickerProbe.cpp"
$Exe = Join-Path $BuildDir "AceAq3D7FlickerProbe.exe"
$cl = Get-Command cl.exe -ErrorAction SilentlyContinue
if ($cl) {
    Write-Host "INFO|compiler|cl.exe"
    & $cl.Source /std:c++20 /EHsc /W4 $FoArg /I (Join-Path $Root "Source\Public") /Fe:$Exe $Probe
    if ($LASTEXITCODE -ne 0) { throw "cl.exe failed with exit code $LASTEXITCODE" }
} else {
    $gpp = Get-Command g++.exe -ErrorAction SilentlyContinue
    if (-not $gpp) { $gpp = Get-Command g++ -ErrorAction SilentlyContinue }
    if (-not $gpp) { throw "No C++ compiler found. Run from Developer PowerShell with cl.exe or install g++." }
    Write-Host "INFO|compiler|g++"
    & $gpp.Source -std=c++20 -Wall -Wextra -pedantic -I (Join-Path $Root "Source\Public") -o $Exe $Probe
    if ($LASTEXITCODE -ne 0) { throw "g++ failed with exit code $LASTEXITCODE" }
}

Push-Location $Root
try {
    & $Exe
    if ($LASTEXITCODE -ne 0) { throw "AceAq3D7FlickerProbe failed with exit code $LASTEXITCODE" }
}
finally {
    Pop-Location
}

Invoke-ExistingValidator "validate_ace_aq3d0.ps1" "aq3d0_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aqui0.ps1" "aqui0_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aqcpp1.ps1" "cpp1_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aqcpp2.ps1" "cpp2_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aqcpp3.ps1" "cpp3_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aqcpp4.ps1" "cpp4_probe_still_passes"

Write-Host "PASS|ace_aq3d7_validator|dirty rect, stable child HWND and pending resize checks passed"
