$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $Root "Build\ACE-RHI3-RHI4"
New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
$ObjDir = Join-Path $BuildDir "obj"
New-Item -ItemType Directory -Force -Path $ObjDir | Out-Null
$FoArg = "/Fo$ObjDir\"

$Required = @(
    "Source\Public\ArhqenCognitionEngine\Renderer\RHI\AceDx12Rhi.h",
    "Source\Private\Renderer\RHI\AceDx12Rhi.cpp",
    "Tools\AceRhi4Dx12OffscreenSceneProbe.cpp",
    "Docs\ACE_RHI3_RHI4.md"
)

foreach ($Rel in $Required) {
    if (-not (Test-Path (Join-Path $Root $Rel))) {
        throw "Missing ACE-RHI3/RHI4 file: $Rel"
    }
}
Write-Host "PASS|rhi3_rhi4_files_present"

$Header = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\Renderer\RHI\AceDx12Rhi.h") -Raw
$Dx12 = Get-Content (Join-Path $Root "Source\Private\Renderer\RHI\AceDx12Rhi.cpp") -Raw
$Probe = Get-Content (Join-Path $Root "Tools\AceRhi4Dx12OffscreenSceneProbe.cpp") -Raw

if ($Header -notlike "*compiledShaders*" -or $Header -notlike "*nativePipelines*" -or $Header -notlike "*drawCallsExecuted*" -or $Header -notlike "*createOffscreenSceneTargets*") {
    throw "RHI3/RHI4 public DX12 stats/offscreen API missing."
}
Write-Host "PASS|dx12_rhi3_rhi4_public_api_present"

if ($Dx12 -notlike "*D3DCompile*" -or $Dx12 -notlike "*VSMain*" -or $Dx12 -notlike "*PSMain*") {
    throw "RHI3 shader compile path missing."
}
Write-Host "PASS|rhi3_shader_compile_path_present"

if ($Dx12 -notlike "*D3D12SerializeRootSignature*" -or $Dx12 -notlike "*CreateRootSignature*") {
    throw "RHI3 root signature path missing."
}
Write-Host "PASS|rhi3_root_signature_path_present"

if ($Dx12 -notlike "*CreateGraphicsPipelineState*" -or $Dx12 -notlike "*D3D12_INPUT_ELEMENT_DESC*" -or $Dx12 -notlike "*IASetPrimitiveTopology*") {
    throw "RHI3 PSO/input layout path missing."
}
Write-Host "PASS|rhi3_pso_input_layout_path_present"

if ($Dx12 -notlike "*DrawInstanced*" -or $Dx12 -notlike "*DrawIndexedInstanced*" -or $Dx12 -notlike "*SetPipelineState*") {
    throw "RHI3 actual draw execution path missing."
}
Write-Host "PASS|rhi3_actual_draw_execution_present"

if ($Dx12 -notlike "*AceRhi4_OffscreenSceneColor*" -or $Dx12 -notlike "*AceRhi4_OffscreenSceneDepth*" -or $Dx12 -notlike "*offscreenSceneTargets*") {
    throw "RHI4 offscreen scene target path missing."
}
Write-Host "PASS|rhi4_offscreen_scene_targets_present"

if ($Probe -notlike "*rhi3_dx12_draw_instanced_executes*" -or $Probe -notlike "*rhi4_scene_renderer_draws_to_offscreen_targets*" -or $Probe -notlike "*dynamic_cast<Dx12Device*>*") {
    throw "RHI3/RHI4 GPU probe is incomplete."
}
Write-Host "PASS|rhi3_rhi4_gpu_probe_present"

$ProbePath = Join-Path $Root "Tools\AceRhi4Dx12OffscreenSceneProbe.cpp"
$Sources = @(
    (Join-Path $Root "Source\Private\Renderer\RHI\AceRhi.cpp"),
    (Join-Path $Root "Source\Private\Renderer\RHI\AceDx12Rhi.cpp"),
    (Join-Path $Root "Source\Private\Renderer\Scene\AceRenderScene.cpp"),
    (Join-Path $Root "Source\Private\Renderer\Core\AceRenderer.cpp")
)
$Exe = Join-Path $BuildDir "AceRhi4Dx12OffscreenSceneProbe.exe"

$cl = Get-Command cl.exe -ErrorAction SilentlyContinue
if ($cl) {
    Write-Host "INFO|compiler|cl.exe"
    & $cl.Source /std:c++20 /EHsc /W4 $FoArg /I (Join-Path $Root "Source\Public") /Fe:$Exe $ProbePath $Sources d3d12.lib dxgi.lib d3dcompiler.lib
    if ($LASTEXITCODE -ne 0) { throw "cl.exe failed with exit code $LASTEXITCODE" }

    Push-Location $Root
    try {
        & $Exe
        if ($LASTEXITCODE -ne 0) { throw "AceRhi4Dx12OffscreenSceneProbe failed with exit code $LASTEXITCODE" }
    }
    finally {
        Pop-Location
    }
} else {
    Write-Host "WARN|compiler|cl.exe not found; running static validation only in this environment"
}

Write-Host "PASS|ace_rhi3_rhi4_validation_complete"
