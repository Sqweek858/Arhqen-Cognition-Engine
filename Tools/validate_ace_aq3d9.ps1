$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $Root "Build\ACE-AQ3D9"
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
    "Tools\AceAq3D9ResizeProxyProbe.cpp",
    "Tools\validate_ace_aq3d9.ps1",
    "Docs\ACE_AQ3D9.md"
)
foreach ($rel in $Required) {
    if (-not (Test-Path (Join-Path $Root $rel))) { throw "Missing required AQ3D9 file: $rel" }
}

$UiHeader = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\Ui\AceShellUi.h") -Raw
$UiCpp = Get-Content (Join-Path $Root "Source\Private\Ui\AceShellUi.cpp") -Raw
$EmbeddedCpp = Get-Content (Join-Path $Root "Source\Private\AquariumRender\AceAquariumEmbeddedDx12Viewport.cpp") -Raw
$Docs = Get-Content (Join-Path $Root "Docs\ACE_AQ3D9.md") -Raw

foreach ($needle in @("aquariumViewportHiddenForLiveResize_", "aquariumViewportWasVisibleBeforeLiveResize_")) {
    Assert-Contains $UiHeader $needle "Missing live resize viewport proxy state: $needle"
}
Write-Host "PASS|live_resize_proxy_state_exists"

$BeginStart = $UiCpp.IndexOf("void AceShellUi::beginWindowLiveResize")
if ($BeginStart -lt 0) { throw "beginWindowLiveResize missing." }
$BeginText = $UiCpp.Substring($BeginStart, [Math]::Min(1800, $UiCpp.Length - $BeginStart))
Assert-Contains $BeginText "aquariumEmbeddedDx12Viewport_.Hide()" "Live resize does not hide the child DX12 viewport."
Assert-Contains $BeginText "aquariumViewportHiddenForLiveResize_ = true" "Live resize does not mark the viewport hidden proxy state."
Assert-Contains $BeginText "++viewportHideForLiveResizeCount_" "Hide-for-live-resize counter missing."
Write-Host "PASS|live_resize_hides_child_viewport"

$SyncStart = $UiCpp.IndexOf("void AceShellUi::syncAquariumEmbeddedViewportWindow")
if ($SyncStart -lt 0) { throw "syncAquariumEmbeddedViewportWindow missing." }
$SyncText = $UiCpp.Substring($SyncStart, [Math]::Min(2800, $UiCpp.Length - $SyncStart))
Assert-Contains $SyncText "if (windowLiveResizeActive_)" "Child HWND sync is not guarded during live resize."
Assert-Contains $SyncText "aquariumEmbeddedViewportSyncNeeded_ = true" "Live resize sync guard does not defer sync."
Assert-Contains $SyncText "aquariumEmbeddedDx12Viewport_.Show" "Child viewport is not restored through final Show/sync."
Assert-Contains $SyncText "viewportShowAfterLiveResizeCount_" "Show-after-live-resize counter missing."
Assert-Contains $SyncText "aquariumViewportHiddenForLiveResize_ = false" "Child viewport hidden state is not cleared after final sync."
Write-Host "PASS|live_resize_restores_child_viewport_after_exit"
Write-Host "PASS|live_resize_does_not_sync_child_hwnd"

Assert-Contains $UiHeader "renderAquariumResizeProxyViewport" "D2D resize proxy viewport declaration missing."
Assert-Contains $UiCpp "void AceShellUi::renderAquariumResizeProxyViewport" "D2D resize proxy viewport implementation missing."
Assert-Contains $UiCpp "Resizing viewport" "Resize proxy label missing."
Write-Host "PASS|live_resize_has_d2d_proxy_viewport"

Assert-Contains $UiHeader "liveResizeProxyPaintCount_" "Resize proxy paint counter field missing."
Assert-Contains $UiCpp "++liveResizeProxyPaintCount_" "Resize proxy paint counter increment missing."
Write-Host "PASS|live_resize_proxy_paint_counter_exists"

Assert-Contains $UiHeader "d2dResizeDuringLiveResizeCount_" "D2D resize-during-live-resize counter missing."
Assert-Contains $UiCpp "layoutForLiveResize" "Live resize lightweight layout helper missing."
Assert-Contains $UiCpp "++d2dResizeDuringLiveResizeCount_" "D2D target/layout does not update during live resize."
Write-Host "PASS|live_resize_resizes_d2d_target"

$TickStart = $UiCpp.IndexOf("void AceShellUi::tick")
if ($TickStart -lt 0) { throw "AceShellUi::tick missing." }
$TickText = $UiCpp.Substring($TickStart, [Math]::Min(2800, $UiCpp.Length - $TickStart))
Assert-Contains $TickText "if (windowLiveResizeActive_)" "tick does not guard live resize."
Assert-Contains $TickText "SetResizeApplySuspended(true)" "tick does not keep viewport resize apply suspended during live resize."
Assert-Contains $TickText "else" "tick does not separate live resize from normal RenderFrame path."
Write-Host "PASS|live_resize_does_not_render_dx12_frame"

$ApplyStart = $EmbeddedCpp.IndexOf("void AceAquariumEmbeddedDx12Viewport::ApplyPendingResizeIfNeeded")
if ($ApplyStart -lt 0) { throw "ApplyPendingResizeIfNeeded missing." }
$ApplyText = $EmbeddedCpp.Substring($ApplyStart, [Math]::Min(1200, $EmbeddedCpp.Length - $ApplyStart))
Assert-Contains $ApplyText "if (resizeApplySuspended_)" "Embedded resize apply is not suspended."
Assert-Contains $ApplyText "return" "Embedded resize suspension does not return."
Write-Host "PASS|live_resize_does_not_recreate_renderer"

Assert-Contains $UiCpp "WM_ENTERSIZEMOVE" "WM_ENTERSIZEMOVE missing."
Write-Host "PASS|wm_entersizemove_still_handled"
Assert-Contains $UiCpp "WM_EXITSIZEMOVE" "WM_EXITSIZEMOVE missing."
Write-Host "PASS|wm_exitsizemove_still_handled"

Assert-Contains $Docs "D2D proxy" "AQ3D9 docs do not mention D2D proxy."
Assert-Contains $Docs "child DX12 HWND" "AQ3D9 docs do not mention child DX12 HWND."
Assert-Contains $Docs "not a new renderer" "AQ3D9 docs do not document non-goal renderer boundary."
Write-Host "PASS|aq3d9_docs_exist"

foreach ($ext in @(".obj", ".exe", ".pdb", ".ilk", ".log")) {
    $files = Get-ChildItem $Root -File -ErrorAction SilentlyContinue | Where-Object { $_.Extension -eq $ext }
    if ($files.Count -gt 0) { throw "Root $ext files found: $($files.Name -join ', ')" }
}
Write-Host "PASS|no_root_build_artifacts"

$Probe = Join-Path $Root "Tools\AceAq3D9ResizeProxyProbe.cpp"
$Exe = Join-Path $BuildDir "AceAq3D9ResizeProxyProbe.exe"
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
    if ($LASTEXITCODE -ne 0) { throw "AceAq3D9ResizeProxyProbe failed with exit code $LASTEXITCODE" }
}
finally {
    Pop-Location
}

Invoke-ExistingValidator "validate_ace_aq3d8.ps1" "aq3d8_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aq3d7.ps1" "aq3d7_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aq3d0.ps1" "aq3d0_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aqui0.ps1" "aqui0_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aqcpp1.ps1" "cpp1_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aqcpp2.ps1" "cpp2_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aqcpp3.ps1" "cpp3_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aqcpp4.ps1" "cpp4_probe_still_passes"

Write-Host "PASS|ace_aq3d9_validator|resize-safe viewport proxy checks passed"
