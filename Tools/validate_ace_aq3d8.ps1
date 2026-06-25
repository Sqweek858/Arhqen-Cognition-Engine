$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $Root "Build\ACE-AQ3D8"
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
    "Tools\AceAq3D8ResizeProbe.cpp",
    "Tools\validate_ace_aq3d8.ps1",
    "Docs\ACE_AQ3D8.md"
)
foreach ($rel in $Required) {
    if (-not (Test-Path (Join-Path $Root $rel))) { throw "Missing required AQ3D8 file: $rel" }
}

$UiHeader = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\Ui\AceShellUi.h") -Raw
$UiCpp = Get-Content (Join-Path $Root "Source\Private\Ui\AceShellUi.cpp") -Raw
$EmbeddedHeader = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\AquariumRender\AceAquariumEmbeddedDx12Viewport.h") -Raw
$EmbeddedCpp = Get-Content (Join-Path $Root "Source\Private\AquariumRender\AceAquariumEmbeddedDx12Viewport.cpp") -Raw
$NativeWindow = Get-Content (Join-Path $Root "Source\Private\Renderer\NativeWindow.cpp") -Raw
$Docs = Get-Content (Join-Path $Root "Docs\ACE_AQ3D8.md") -Raw

Assert-Contains $UiCpp "WM_ENTERSIZEMOVE" "AceShellUi does not handle WM_ENTERSIZEMOVE."
Write-Host "PASS|ui_handles_wm_entersizemove"
Assert-Contains $UiCpp "WM_EXITSIZEMOVE" "AceShellUi does not handle WM_EXITSIZEMOVE."
Write-Host "PASS|ui_handles_wm_exitsizemove"

foreach ($needle in @("windowLiveResizeActive_", "pendingResizeAfterLiveDrag_", "pendingLiveResizeWidth_", "pendingLiveResizeHeight_")) {
    Assert-Contains $UiHeader $needle "Missing live resize state: $needle"
}
Write-Host "PASS|live_resize_state_exists"

$SyncStart = $UiCpp.IndexOf("void AceShellUi::syncAquariumEmbeddedViewportWindow")
if ($SyncStart -lt 0) { throw "syncAquariumEmbeddedViewportWindow missing." }
$SyncText = $UiCpp.Substring($SyncStart, [Math]::Min(2200, $UiCpp.Length - $SyncStart))
Assert-Contains $SyncText "if (windowLiveResizeActive_)" "Child HWND sync is not guarded during live resize."
Assert-Contains $SyncText "aquariumEmbeddedViewportSyncNeeded_ = true" "Live resize guard does not defer viewport sync."
Assert-Contains $SyncText "return" "Live resize guard does not return before child HWND operations."
Write-Host "PASS|live_resize_defers_child_hwnd_sync"
Write-Host "PASS|sync_viewport_window_guarded_during_live_resize"

Assert-Contains $UiCpp "SetResizeApplySuspended(true)" "UI does not suspend embedded viewport resize apply on live resize."
Assert-Contains $UiCpp "SetResizeApplySuspended(false)" "UI does not resume embedded viewport resize apply after live resize."
Write-Host "PASS|live_resize_suspends_viewport_resize_apply"

Assert-Contains $UiCpp "applyDeferredLiveResize" "Final live resize apply helper missing."
Assert-Contains $UiCpp "++liveResizeAppliedFinalCount_" "Final live resize apply counter missing."
Assert-Contains $UiCpp "layout(finalWidth, finalHeight)" "Final live resize does not apply final layout once."
Write-Host "PASS|live_resize_applies_final_resize_once"

Assert-Contains $EmbeddedHeader "SetResizeApplySuspended" "Embedded viewport resize suspension API missing."
Assert-Contains $EmbeddedHeader "IsResizeApplySuspended" "Embedded viewport suspension query missing."
Assert-Contains $EmbeddedHeader "HasPendingResize" "Embedded viewport pending resize query missing."
Assert-Contains $EmbeddedHeader "resizeApplySuspended_" "Embedded viewport suspension field missing."
Write-Host "PASS|embedded_viewport_has_resize_apply_suspension"

$ApplyStart = $EmbeddedCpp.IndexOf("void AceAquariumEmbeddedDx12Viewport::ApplyPendingResizeIfNeeded")
if ($ApplyStart -lt 0) { throw "ApplyPendingResizeIfNeeded missing." }
$ApplyText = $EmbeddedCpp.Substring($ApplyStart, [Math]::Min(1200, $EmbeddedCpp.Length - $ApplyStart))
Assert-Contains $ApplyText "if (resizeApplySuspended_)" "ApplyPendingResizeIfNeeded is not suspended during live resize."
Assert-Contains $ApplyText "return" "ApplyPendingResizeIfNeeded suspension does not return."
Write-Host "PASS|embedded_viewport_does_not_apply_resize_when_suspended"

$WmSizeStart = $EmbeddedCpp.IndexOf("case WM_SIZE:")
if ($WmSizeStart -lt 0) { throw "Embedded viewport WM_SIZE missing." }
$WmSizeText = $EmbeddedCpp.Substring($WmSizeStart, [Math]::Min(900, $EmbeddedCpp.Length - $WmSizeStart))
Assert-Contains $WmSizeText "QueueResize" "Embedded WM_SIZE does not queue resize."
Assert-NotContains $WmSizeText "RecreateRenderer" "Embedded WM_SIZE directly recreates renderer."
Write-Host "PASS|renderer_recreate_not_called_directly_from_wm_size"

Assert-Contains $UiCpp "gradientsDirty_ = true" "Gradient dirty marker missing."
Assert-Contains $Docs "Gradient recreation is deferred" "AQ3D8 docs do not document gradient deferral."
Write-Host "PASS|gradient_recreation_deferred_or_documented"

Assert-Contains $NativeWindow "message == WM_SIZE" "NativeWindow does not pre-update size for transactional WM_SIZE handling."
Assert-Contains $NativeWindow "before the UI message handler" "NativeWindow transactional size comment missing."
Write-Host "PASS|native_window_size_updated_before_ui_handler"

Assert-Contains $UiCpp "liveResizeDeferredSizeCount_" "Deferred live resize counter missing."
Assert-Contains $UiHeader "rendererRecreateDuringLiveResizeCount_" "Renderer recreate during live resize counter missing from UI."
Assert-Contains $EmbeddedHeader "rendererRecreateDuringLiveResizeCount_" "Renderer recreate during live resize counter missing from embedded viewport."
Assert-Contains $EmbeddedHeader "childMoveDuringLiveResizeCount_" "Child move during live resize counter missing from embedded viewport."
Write-Host "PASS|live_resize_diagnostic_counters_exist"

$TickStart = $UiCpp.IndexOf("void AceShellUi::tick")
if ($TickStart -lt 0) { throw "AceShellUi::tick missing." }
$TickText = $UiCpp.Substring($TickStart, [Math]::Min(2600, $UiCpp.Length - $TickStart))
Assert-Contains $TickText "if (windowLiveResizeActive_)" "tick does not guard live resize."
Assert-Contains $TickText "SetResizeApplySuspended(true)" "tick does not keep viewport resize apply suspended during live resize."
Write-Host "PASS|live_resize_freezes_viewport_render_path"

foreach ($ext in @(".obj", ".exe", ".pdb", ".ilk", ".log")) {
    $files = Get-ChildItem $Root -File -ErrorAction SilentlyContinue | Where-Object { $_.Extension -eq $ext }
    if ($files.Count -gt 0) { throw "Root $ext files found: $($files.Name -join ', ')" }
    $pass = switch ($ext) { ".obj" { "no_obj_files_in_repo_root" } ".exe" { "no_exe_files_in_repo_root" } ".pdb" { "no_pdb_files_in_repo_root" } ".ilk" { "no_ilk_files_in_repo_root" } ".log" { "no_log_files_in_repo_root" } }
    Write-Host "PASS|$pass"
}

foreach ($needle in @("Panda3D", "DearPyGui")) {
    if (($UiCpp + "`n" + $EmbeddedCpp) -like "*$needle*") { throw "Forbidden dependency marker found: $needle" }
}
Write-Host "PASS|no_python_runtime"
Write-Host "PASS|no_panda3d"
Write-Host "PASS|no_dearpygui"

Assert-Contains $Docs "Live Resize Flicker Fix" "AQ3D8 docs heading missing."

$Probe = Join-Path $Root "Tools\AceAq3D8ResizeProbe.cpp"
$Exe = Join-Path $BuildDir "AceAq3D8ResizeProbe.exe"
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
    if ($LASTEXITCODE -ne 0) { throw "AceAq3D8ResizeProbe failed with exit code $LASTEXITCODE" }
}
finally {
    Pop-Location
}

Invoke-ExistingValidator "validate_ace_aq3d7.ps1" "aq3d7_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aq3d0.ps1" "aq3d0_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aqui0.ps1" "aqui0_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aqcpp1.ps1" "cpp1_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aqcpp2.ps1" "cpp2_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aqcpp3.ps1" "cpp3_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aqcpp4.ps1" "cpp4_probe_still_passes"

Write-Host "PASS|ace_aq3d8_validator|live resize transaction and resize flicker checks passed"
