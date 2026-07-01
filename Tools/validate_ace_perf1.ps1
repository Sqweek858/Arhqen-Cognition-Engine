$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $Root "Build\ACE-PERF1"
New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null

$Required = @(
    "Source\Public\ArhqenCognitionEngine\Renderer\RHI\AceDx12Rhi.h",
    "Source\Private\Renderer\RHI\AceDx12Rhi.cpp",
    "Source\Private\Renderer\Scene\AceAquariumGpuViewportRenderer.cpp",
    "Source\Public\ArhqenCognitionEngine\Ui\AceShellUi.h",
    "Source\Private\Ui\AceShellUi.cpp",
    "Docs\ACE_PERF1_GPU_VIEWPORT_COMPOSITION.md",
    "Docs\ACE_PERF1R1_VERIFIED_HOT_PATH.md",
    "Tools\AcePerf1GpuViewportCompositionProbe.cpp"
)

foreach ($Rel in $Required) {
    if (-not (Test-Path (Join-Path $Root $Rel))) {
        throw "Missing ACE-PERF1 file: $Rel"
    }
}
Write-Host "PASS|perf1_files_present"

$Shell = Get-Content (Join-Path $Root "Source\Private\Ui\AceShellUi.cpp") -Raw
$ShellH = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\Ui\AceShellUi.h") -Raw
$Dx12H = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\Renderer\RHI\AceDx12Rhi.h") -Raw
$Dx12Cpp = Get-Content (Join-Path $Root "Source\Private\Renderer\RHI\AceDx12Rhi.cpp") -Raw
$GpuCpp = Get-Content (Join-Path $Root "Source\Private\Renderer\Scene\AceAquariumGpuViewportRenderer.cpp") -Raw
$Docs = Get-Content (Join-Path $Root "Docs\ACE_PERF1_GPU_VIEWPORT_COMPOSITION.md") -Raw

function Assert-Contains($Name, $Haystack, $Needle) {
    if ($Haystack -notlike "*$Needle*") { throw "$Name missing marker: $Needle" }
    Write-Host "PASS|$Name"
}

Assert-Contains "perf1_docs_title" $Docs "ACE-PERF1"
Assert-Contains "perf1_docs_ue_viewport" $Docs "FSlateDrawElement::MakeViewport"
Assert-Contains "perf1_mapped_upload_header" $Dx12H "mappedUploadUpdates"
Assert-Contains "perf1_readback_cache_header" $Dx12H "readbackBufferReuses"
Assert-Contains "perf1_readback_cache_cpp" $Dx12Cpp "Dx12ReadbackCache"
Assert-Contains "perf1_mapped_upload_cpp" $Dx12Cpp "nativeDst->mapped"
Assert-Contains "perf1_upload_heap_no_barrier" $Dx12Cpp "Upload heap resources are permanently GENERIC_READ"
Assert-Contains "perf1_dynamic_vertex_buffer" $GpuCpp "desc.memory = Memory::Upload"
Assert-Contains "perf1_fast_viewport_header" $ShellH "aquariumFastViewportPaintCount_"
Assert-Contains "perf1_fast_viewport_cpp" $Shell "renderFastAquariumViewportFrame"
Assert-Contains "perf1_fast_viewport_skip_chrome" $Shell "if (!fastAquariumViewportPaint)"
Assert-Contains "perf1_stat_ui_counter" $Shell "fast_viewport_paints"
Assert-Contains "perf1_no_child_hwnd_regression_marker" $Docs "No child-HWND regression"

Assert-Contains "perf1r1_combined_readback_header" $Dx12H "submitAndReadbackBgra8"
Assert-Contains "perf1r1_combined_readback_cpp" $Dx12Cpp "submitAndReadbackBgra8"
Assert-Contains "perf1r1_combined_readback_renderer" $GpuCpp "submitAndReadbackBgra8"
Assert-Contains "perf1r1_combined_path_label" $Shell "combined render+readback"
Assert-Contains "perf1r1_stat_rhi_visible" $Shell "combinedReadback="
Assert-Contains "perf1r1_stat_fps_visible" $Shell "combined_readback_frames="
Assert-Contains "perf1r1_fence_wait_visible" $Shell "fence_wait_ms="

if ($Shell -like "*stat_frame*") { throw "ACE-PERF1 must not add stat_frame; extend stat_fps/stat_rhi/stat_ui instead" }
Write-Host "PASS|perf1_no_stat_frame"

$Probe = Join-Path $Root "Tools\AcePerf1GpuViewportCompositionProbe.cpp"
$Exe = Join-Path $BuildDir "AcePerf1GpuViewportCompositionProbe.exe"
$cl = Get-Command cl.exe -ErrorAction SilentlyContinue
if ($cl) {
    Write-Host "INFO|compiler|cl.exe"
    & $cl.Source /std:c++20 /EHsc /W4 /Fo"$BuildDir\" /Fe:$Exe $Probe
    if ($LASTEXITCODE -ne 0) { throw "cl.exe failed with exit code $LASTEXITCODE" }
    Push-Location $Root
    try {
        & $Exe
        if ($LASTEXITCODE -ne 0) { throw "AcePerf1GpuViewportCompositionProbe failed with exit code $LASTEXITCODE" }
    }
    finally { Pop-Location }
} else {
    Write-Host "WARN|compiler|cl.exe not found; running static validation only in this environment"
}

Write-Host "PASS|ace_perf1_validation_complete"
