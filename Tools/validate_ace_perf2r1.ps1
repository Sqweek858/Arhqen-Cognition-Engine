$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $Root "Build\ACE-PERF2R1"
New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null

$Required = @(
    "Source\Private\Ui\AceShellUi.cpp",
    "Source\Public\ArhqenCognitionEngine\Ui\AceShellUi.h",
    "Source\Public\ArhqenCognitionEngine\Ui\AceEngineConsole.h",
    "Source\Public\ArhqenCognitionEngine\Renderer\Scene\AceAquariumGpuViewportRenderer.h",
    "Source\Private\Renderer\Scene\AceAquariumGpuViewportRenderer.cpp",
    "Docs\ACE_PERF2R1_D2D_LAYER_QUALITY_GUARD.md",
    "Tools\AcePerf2R1D2DLayerQualityGuardProbe.cpp"
)

foreach ($Rel in $Required) {
    if (-not (Test-Path (Join-Path $Root $Rel))) {
        throw "Missing ACE-PERF2R1 file: $Rel"
    }
}
Write-Host "PASS|perf2r1_files_present"

$Shell = Get-Content (Join-Path $Root "Source\Private\Ui\AceShellUi.cpp") -Raw
$Docs = Get-Content (Join-Path $Root "Docs\ACE_PERF2R1_D2D_LAYER_QUALITY_GUARD.md") -Raw

function Assert-Contains($Name, $Haystack, $Needle) {
    if ($Haystack -notlike "*$Needle*") { throw "$Name missing marker: $Needle" }
    Write-Host "PASS|$Name"
}

function Assert-NotContains($Name, $Haystack, $Needle) {
    if ($Haystack -like "*$Needle*") { throw "$Name unexpectedly contains marker: $Needle" }
    Write-Host "PASS|$Name"
}

Assert-Contains "perf2r1_docs_title" $Docs "ACE-PERF2R1"
Assert-Contains "perf2r1_d2d_quality_marker" $Shell "ui_layer=D2D_RETAINED_OVERLAY"
Assert-Contains "perf2r1_gpu_text_disabled_marker" $Shell "gpu_text_overlay=false"
Assert-Contains "perf2r1_visual_quality_gate" $Shell "ACE-PERF2R1: visual quality guard"
Assert-Contains "perf2r1_d2d_log_console" $Shell "engineLogOverlayInput_.render(ctx)"
Assert-Contains "perf2r1_d2d_hud_layer" $Shell "renderAquariumViewportHudLayer"
Assert-NotContains "perf2r1_no_default_gpu_overlay_param" $Shell "useDirectComposition ? &gpuOverlay : nullptr"
Assert-NotContains "perf2r1_no_stat_frame" $Shell "stat_frame"

$Probe = Join-Path $Root "Tools\AcePerf2R1D2DLayerQualityGuardProbe.cpp"
$Exe = Join-Path $BuildDir "AcePerf2R1D2DLayerQualityGuardProbe.exe"
$cl = Get-Command cl.exe -ErrorAction SilentlyContinue
if ($cl) {
    Write-Host "INFO|compiler|cl.exe"
    & $cl.Source /std:c++20 /EHsc /W4 /Fo"$BuildDir\" /Fe:$Exe $Probe
    if ($LASTEXITCODE -ne 0) { throw "cl.exe failed with exit code $LASTEXITCODE" }
    Push-Location $Root
    try {
        & $Exe
        if ($LASTEXITCODE -ne 0) { throw "AcePerf2R1D2DLayerQualityGuardProbe failed with exit code $LASTEXITCODE" }
    }
    finally { Pop-Location }
} else {
    Write-Host "WARN|compiler|cl.exe not found; running static validation only in this environment"
}

Write-Host "PASS|ace_perf2r1_validation_complete"
