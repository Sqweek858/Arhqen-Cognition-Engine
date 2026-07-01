$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $Root "Build\ACE-RHI5-RHI6"
New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
$ObjDir = Join-Path $BuildDir "obj"
New-Item -ItemType Directory -Force -Path $ObjDir | Out-Null
$FoArg = "/Fo$ObjDir\"

$Required = @(
    "Source\Public\ArhqenCognitionEngine\Renderer\Scene\AceAquariumGpuViewportRenderer.h",
    "Source\Private\Renderer\Scene\AceAquariumGpuViewportRenderer.cpp",
    "Tools\AceRhi6AquariumGpuViewportProbe.cpp",
    "Tools\validate_ace_rhi5_rhi6.ps1",
    "Docs\ACE_RHI5_RHI6.md"
)

foreach ($Rel in $Required) {
    if (-not (Test-Path (Join-Path $Root $Rel))) {
        throw "Missing ACE-RHI5/RHI6 file: $Rel"
    }
}
Write-Host "PASS|rhi5_rhi6_files_present"

$DxHeader = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\Renderer\RHI\AceDx12Rhi.h") -Raw
$Dx12 = Get-Content (Join-Path $Root "Source\Private\Renderer\RHI\AceDx12Rhi.cpp") -Raw
$GpuHeader = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\Renderer\Scene\AceAquariumGpuViewportRenderer.h") -Raw
$GpuCpp = Get-Content (Join-Path $Root "Source\Private\Renderer\Scene\AceAquariumGpuViewportRenderer.cpp") -Raw
$ShellHeader = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\Ui\AceShellUi.h") -Raw
$ShellCpp = Get-Content (Join-Path $Root "Source\Private\Ui\AceShellUi.cpp") -Raw
$CMake = Get-Content (Join-Path $Root "CMakeLists.txt") -Raw
$Vcx = Get-Content (Join-Path $Root "Source\ArhqenCognitionEngine.vcxproj") -Raw

if ($DxHeader -notlike "*readbackBgra8*" -or $DxHeader -notlike "*readbackFrames*" -or $Dx12 -notlike "*GetCopyableFootprints*" -or $Dx12 -notlike "*CopyTextureRegion*") {
    throw "RHI5 DX12 texture readback path missing."
}
Write-Host "PASS|rhi5_dx12_scene_texture_readback_present"

if ($GpuHeader -notlike "*class AceAquariumGpuViewportRenderer*" -or $GpuHeader -notlike "*AceAquariumGpuViewportSnapshot*" -or ($GpuCpp -notlike "*AceRhi6_AquariumGpuViewportPass*" -and $GpuCpp -notlike "*AceRhi7_AquariumDepthTested3DPass*")) {
    throw "RHI6 Aquarium GPU viewport renderer missing."
}
Write-Host "PASS|rhi6_aquarium_gpu_viewport_renderer_present"

if ($GpuCpp -notlike "*Usage::Vertex | Usage::CopyDst*" -or $GpuCpp -notlike "*list.draw*" -or $GpuCpp -notlike "*device_->readbackBgra8*") {
    throw "RHI6 GPU mesh draw/readback path missing."
}
Write-Host "PASS|rhi6_gpu_mesh_draw_readback_path_present"

if ($ShellHeader -notlike "*aquariumGpuViewportRenderer_*" -or $ShellCpp -notlike "*AceAquariumGpuViewportSnapshot*" -or $ShellCpp -notlike "*DrawBitmap*" -or $ShellCpp -notlike "*GPU Environment viewport active*") {
    throw "RHI5 main-HWND D2D composition bridge missing."
}
Write-Host "PASS|rhi5_main_hwnd_composition_bridge_present"

if ($CMake -notlike "*AceAquariumGpuViewportRenderer.cpp*" -or $Vcx -notlike "*AceAquariumGpuViewportRenderer.cpp*") {
    throw "Project files do not reference Aquarium GPU viewport renderer."
}
Write-Host "PASS|project_references_rhi6_gpu_viewport"

$Probe = Join-Path $Root "Tools\AceRhi6AquariumGpuViewportProbe.cpp"
$Sources = @(
    (Join-Path $Root "Source\Private\Renderer\RHI\AceRhi.cpp"),
    (Join-Path $Root "Source\Private\Renderer\RHI\AceDx12Rhi.cpp"),
    (Join-Path $Root "Source\Private\Renderer\Scene\AceAquariumGpuViewportRenderer.cpp")
)
$Exe = Join-Path $BuildDir "AceRhi6AquariumGpuViewportProbe.exe"

$cl = Get-Command cl.exe -ErrorAction SilentlyContinue
if ($cl) {
    Write-Host "INFO|compiler|cl.exe"
    & $cl.Source /std:c++20 /EHsc /W4 $FoArg /I (Join-Path $Root "Source\Public") /Fe:$Exe $Probe $Sources d3d12.lib dxgi.lib d3dcompiler.lib
    if ($LASTEXITCODE -ne 0) { throw "cl.exe failed with exit code $LASTEXITCODE" }

    Push-Location $Root
    try {
        & $Exe
        if ($LASTEXITCODE -ne 0) { throw "AceRhi6AquariumGpuViewportProbe failed with exit code $LASTEXITCODE" }
    }
    finally {
        Pop-Location
    }
} else {
    Write-Host "WARN|compiler|cl.exe not found; running static validation only in this environment"
}

Write-Host "PASS|ace_rhi5_rhi6_validation_complete"
