$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $Root "Build\ACE-UI12"
New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null

$Required = @(
    "Source\Public\ArhqenCognitionEngine\Ui\AceShellUi.h",
    "Source\Private\Ui\AceShellUi.cpp",
    "Docs\ACE_UI12_VIEWPORT_OVERLAY_POLISH.md",
    "Docs\ACE_AQ3D15_VIEWPORT_LAYERING.md",
    "Tools\AceUi12ViewportOverlayProbe.cpp"
)

foreach ($Rel in $Required) {
    if (-not (Test-Path (Join-Path $Root $Rel))) {
        throw "Missing ACE-UI12 file: $Rel"
    }
}
Write-Host "PASS|ui12_files_present"

$Shell = Get-Content (Join-Path $Root "Source\Private\Ui\AceShellUi.cpp") -Raw
$ShellH = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\Ui\AceShellUi.h") -Raw
$Docs = Get-Content (Join-Path $Root "Docs\ACE_UI12_VIEWPORT_OVERLAY_POLISH.md") -Raw

function Assert-Contains($Name, $Haystack, $Needle) {
    if ($Haystack -notlike "*$Needle*") { throw "$Name missing marker: $Needle" }
    Write-Host "PASS|$Name"
}

Assert-Contains "ui12_docs_title" $Docs "Stable Viewport Overlay Layering"
Assert-Contains "ui12_overlay_state" $Shell "isViewportLocalOverlayActive"
Assert-Contains "ui12_parent_hold_header" $ShellH "aquariumParentCompositedHoldFrames_"
Assert-Contains "ui12_parent_hold_cpp" $Shell "requestParentCompositedViewportHold"
Assert-Contains "ui12_deferred_dcomp_header" $ShellH "aquariumResetDirectCompositionAfterPaint_"
Assert-Contains "ui12_deferred_dcomp_cpp" $Shell "defer DirectComposition teardown"
Assert-Contains "ui12_hud_layer" $Shell "renderAquariumViewportHudLayer"
Assert-Contains "ui12_telemetry_rect" $Shell "computeAquariumTelemetryOverlayRect"
Assert-Contains "ui12_telemetry_avoid" $Shell "avoidRect.top - 12.0f"
Assert-Contains "ui12_no_toggle_toast_marker" $Shell "no toast on console toggle"
Assert-Contains "ui12_perf2_gpu_overlay_marker" $Shell "ACE-PERF2"
Assert-Contains "ui12_perf2_overlay_builder" $Shell "buildAquariumGpuViewportOverlay"
Assert-Contains "ui12_perf2_gpu_overlay_docs" $Docs "GPU-composited viewport overlays"
Assert-Contains "ui12_r1_no_dcomp_rmb_docs" $Docs "RMB/WASD"

if ($Shell -like "*Docked engine log console opened. Press `` to close.*") { throw "Backtick toggle still emits the old toast body" }
if ($Shell -like "*DX12 child viewport clipped by engine log UI*" -or $Shell -like "*overlay.top - 8.0f*") { throw "Old child-HWND clipping workaround was reintroduced" }
Write-Host "PASS|ui12_no_old_child_clip"

$Probe = Join-Path $Root "Tools\AceUi12ViewportOverlayProbe.cpp"
$Exe = Join-Path $BuildDir "AceUi12ViewportOverlayProbe.exe"
$cl = Get-Command cl.exe -ErrorAction SilentlyContinue
if ($cl) {
    Write-Host "INFO|compiler|cl.exe"
    & $cl.Source /std:c++20 /EHsc /W4 /Fo"$BuildDir\" /Fe:$Exe $Probe
    if ($LASTEXITCODE -ne 0) { throw "cl.exe failed with exit code $LASTEXITCODE" }
    Push-Location $Root
    try {
        & $Exe
        if ($LASTEXITCODE -ne 0) { throw "AceUi12ViewportOverlayProbe failed with exit code $LASTEXITCODE" }
    }
    finally { Pop-Location }
} else {
    Write-Host "WARN|compiler|cl.exe not found; running static validation only in this environment"
}

Write-Host "PASS|ace_ui12_validation_complete"
