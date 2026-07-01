$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $Root "Build\ACE-RHI1-RHI2"
New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
$ObjDir = Join-Path $BuildDir "obj"
New-Item -ItemType Directory -Force -Path $ObjDir | Out-Null
$FoArg = "/Fo$ObjDir\"

$Required = @(
    "Source\Public\ArhqenCognitionEngine\Renderer\RHI\AceDx12Rhi.h",
    "Source\Private\Renderer\RHI\AceDx12Rhi.cpp",
    "Tools\AceRhi2Dx12GpuProbe.cpp",
    "Docs\ACE_RHI1_RHI2.md"
)

foreach ($Rel in $Required) {
    if (-not (Test-Path (Join-Path $Root $Rel))) {
        throw "Missing ACE-RHI1/RHI2 file: $Rel"
    }
}
Write-Host "PASS|rhi1_rhi2_files_present"

$Header = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\Renderer\RHI\AceDx12Rhi.h") -Raw
$Dx12 = Get-Content (Join-Path $Root "Source\Private\Renderer\RHI\AceDx12Rhi.cpp") -Raw
$Renderer = Get-Content (Join-Path $Root "Source\Private\Renderer\Core\AceRenderer.cpp") -Raw
$CMake = Get-Content (Join-Path $Root "CMakeLists.txt") -Raw
$Vcx = Get-Content (Join-Path $Root "Source\ArhqenCognitionEngine.vcxproj") -Raw

if ($Header -notlike "*class Dx12Device final : public IDevice*" -or $Header -notlike "*upload(Buffer dst*" -or $Header -notlike "*gpuSmokeTest*") {
    throw "DX12 RHI public shell is incomplete."
}
Write-Host "PASS|dx12_public_backend_shell_present"

if ($Dx12 -notlike "*D3D12CreateDevice*" -or $Dx12 -notlike "*CreateCommandQueue*" -or $Dx12 -notlike "*CreateCommandAllocator*" -or $Dx12 -notlike "*CreateCommandList*") {
    throw "DX12 device/queue/command shell missing."
}
Write-Host "PASS|dx12_device_queue_command_shell_present"

if ($Dx12 -notlike "*Dx12DescriptorHeap*" -or $Dx12 -notlike "*D3D12_DESCRIPTOR_HEAP_TYPE_RTV*" -or $Dx12 -notlike "*D3D12_DESCRIPTOR_HEAP_TYPE_DSV*" -or $Dx12 -notlike "*D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV*") {
    throw "DX12 descriptor heap layer missing."
}
Write-Host "PASS|dx12_descriptor_heaps_present"

if ($Dx12 -notlike "*Dx12UploadArena*" -or $Dx12 -notlike "*D3D12_HEAP_TYPE_UPLOAD*" -or $Dx12 -notlike "*CopyBufferRegion*") {
    throw "DX12 upload arena/copy path missing."
}
Write-Host "PASS|dx12_upload_buffer_path_present"

if ($Dx12 -notlike "*ClearRenderTargetView*" -or $Dx12 -notlike "*ClearDepthStencilView*" -or $Dx12 -notlike "*ResourceBarrier*") {
    throw "DX12 render target clear/barrier path missing."
}
Write-Host "PASS|dx12_gpu_clear_and_barrier_path_present"

if ($Renderer -notlike "*CreateDx12Device()*" -or $Renderer -notlike "*Backend::Dx12*" -or $Renderer -notlike "*g.execute(*") {
    throw "Renderer facade is not wired to DX12 backend execution."
}
Write-Host "PASS|renderer_facade_wires_dx12_backend"

if ($CMake -notlike "*AceDx12Rhi.cpp*" -or $Vcx -notlike "*AceDx12Rhi.cpp*" -or $Vcx -notlike "*d3d12.lib*") {
    throw "Project files do not reference DX12 RHI source/libs."
}
Write-Host "PASS|project_references_dx12_rhi"

$Probe = Join-Path $Root "Tools\AceRhi2Dx12GpuProbe.cpp"
$Sources = @(
    (Join-Path $Root "Source\Private\Renderer\RHI\AceRhi.cpp"),
    (Join-Path $Root "Source\Private\Renderer\RHI\AceDx12Rhi.cpp")
)
$Exe = Join-Path $BuildDir "AceRhi2Dx12GpuProbe.exe"

$cl = Get-Command cl.exe -ErrorAction SilentlyContinue
if ($cl) {
    Write-Host "INFO|compiler|cl.exe"
    & $cl.Source /std:c++20 /EHsc /W4 $FoArg /I (Join-Path $Root "Source\Public") /Fe:$Exe $Probe $Sources d3d12.lib dxgi.lib
    if ($LASTEXITCODE -ne 0) { throw "cl.exe failed with exit code $LASTEXITCODE" }

    Push-Location $Root
    try {
        & $Exe
        if ($LASTEXITCODE -ne 0) { throw "AceRhi2Dx12GpuProbe failed with exit code $LASTEXITCODE" }
    }
    finally {
        Pop-Location
    }
} else {
    Write-Host "WARN|compiler|cl.exe not found; running static validation only in this environment"
}

Write-Host "PASS|ace_rhi1_rhi2_validation_complete"
