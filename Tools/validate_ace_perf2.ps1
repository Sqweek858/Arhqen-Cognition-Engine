$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $Root "Build\ACE-PERF2"
New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null

$Required = @(
    "Source\Public\ArhqenCognitionEngine\Renderer\RHI\AceDx12Rhi.h",
    "Source\Private\Renderer\RHI\AceDx12Rhi.cpp",
    "Source\Public\ArhqenCognitionEngine\Renderer\Scene\AceAquariumGpuViewportRenderer.h",
    "Source\Private\Renderer\Scene\AceAquariumGpuViewportRenderer.cpp",
    "Source\Public\ArhqenCognitionEngine\Ui\AceEngineConsole.h",
    "Source\Public\ArhqenCognitionEngine\Ui\AceShellUi.h",
    "Source\Private\Ui\AceShellUi.cpp",
    "Docs\ACE_PERF2_GPU_COMPOSITED_VIEWPORT.md",
    "Tools\AcePerf2GpuCompositedViewportProbe.cpp"
)

foreach ($Rel in $Required) {
    if (-not (Test-Path (Join-Path $Root $Rel))) {
        throw "Missing ACE-PERF2 file: $Rel"
    }
}
Write-Host "PASS|perf2_files_present"

$Shell = Get-Content (Join-Path $Root "Source\Private\Ui\AceShellUi.cpp") -Raw
$ShellH = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\Ui\AceShellUi.h") -Raw
$Dx12H = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\Renderer\RHI\AceDx12Rhi.h") -Raw
$Dx12Cpp = Get-Content (Join-Path $Root "Source\Private\Renderer\RHI\AceDx12Rhi.cpp") -Raw
$GpuH = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\Renderer\Scene\AceAquariumGpuViewportRenderer.h") -Raw
$GpuCpp = Get-Content (Join-Path $Root "Source\Private\Renderer\Scene\AceAquariumGpuViewportRenderer.cpp") -Raw
$ConsoleH = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\Ui\AceEngineConsole.h") -Raw
$Docs = Get-Content (Join-Path $Root "Docs\ACE_PERF2_GPU_COMPOSITED_VIEWPORT.md") -Raw

function Assert-Contains($Name, $Haystack, $Needle) {
    if ($Haystack -notlike "*$Needle*") { throw "$Name missing marker: $Needle" }
    Write-Host "PASS|$Name"
}

function Assert-NotContains($Name, $Haystack, $Needle) {
    if ($Haystack -like "*$Needle*") { throw "$Name unexpectedly contains marker: $Needle" }
    Write-Host "PASS|$Name"
}

Assert-Contains "perf2_docs_title" $Docs "ACE-PERF2"
Assert-Contains "perf2_docs_ue_reference" $Docs "FSlateDrawElement::MakeViewport"
Assert-Contains "perf2_path_enum" $ConsoleH "DX12_GPU_COMPOSITED"
Assert-Contains "perf2_overlay_struct" $GpuH "AceAquariumGpuViewportOverlay"
Assert-Contains "perf2_overlay_builder" $Shell "buildAquariumGpuViewportOverlay"
Assert-Contains "perf2_overlay_gpu_pass" $GpuCpp "AcePerf2_ViewportGpuOverlayPass"
Assert-Contains "perf2_overlay_glyphs" $GpuCpp "aceGpuGlyph"
Assert-Contains "perf2_combined_submit_present_header" $Dx12H "submitAndPresentBgra8ToComposition"
Assert-Contains "perf2_combined_submit_present_cpp" $Dx12Cpp "DirectComposition swapchain copy into one GPU command list"
Assert-Contains "perf2_renderer_uses_no_readback_path" $GpuCpp "submitAndPresentBgra8ToComposition"
Assert-Contains "perf2_no_stale_readback_fallback" $GpuCpp "do not read stale pixels"
Assert-Contains "perf2_gpu_composition_stats" $Dx12H "combinedGpuCompositionFrames"
Assert-Contains "perf2_stat_rhi_gpu_composition" $Shell "combinedGpuCompositionFrames="
Assert-Contains "perf2_stat_fps_readback_flag" $Shell "readback="
Assert-Contains "perf2r1_visual_quality_guard_or_dcomp_policy" $Shell "ACE-PERF2R1: visual quality guard"
Assert-Contains "perf2r1_d2d_overlay_marker" $Shell "ui_layer=D2D_RETAINED_OVERLAY"
Assert-NotContains "perf2_no_default_gpu_overlay_param" $Shell "useDirectComposition ? &gpuOverlay : nullptr"
Assert-NotContains "perf2_no_stat_frame" $Shell "stat_frame"

$Probe = Join-Path $Root "Tools\AcePerf2GpuCompositedViewportProbe.cpp"
$Exe = Join-Path $BuildDir "AcePerf2GpuCompositedViewportProbe.exe"
$cl = Get-Command cl.exe -ErrorAction SilentlyContinue
if ($cl) {
    Write-Host "INFO|compiler|cl.exe"
    & $cl.Source /std:c++20 /EHsc /W4 /Fo"$BuildDir\" /Fe:$Exe $Probe
    if ($LASTEXITCODE -ne 0) { throw "cl.exe failed with exit code $LASTEXITCODE" }
    Push-Location $Root
    try {
        & $Exe
        if ($LASTEXITCODE -ne 0) { throw "AcePerf2GpuCompositedViewportProbe failed with exit code $LASTEXITCODE" }
    }
    finally { Pop-Location }
} else {
    Write-Host "WARN|compiler|cl.exe not found; running static validation only in this environment"
}

Write-Host "PASS|ace_perf2_validation_complete"
