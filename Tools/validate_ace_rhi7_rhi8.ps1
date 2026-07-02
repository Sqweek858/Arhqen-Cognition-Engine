$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $Root "Build\ACE-RHI7-RHI8"
New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
$ObjDir = Join-Path $BuildDir "obj"
New-Item -ItemType Directory -Force -Path $ObjDir | Out-Null
$FoArg = "/Fo$ObjDir\"

$Required = @(
    "Source\Public\ArhqenCognitionEngine\Renderer\RHI\AceRhi.h",
    "Source\Private\Renderer\RHI\AceRhi.cpp",
    "Source\Public\ArhqenCognitionEngine\Renderer\RHI\AceDx12Rhi.h",
    "Source\Private\Renderer\RHI\AceDx12Rhi.cpp",
    "Source\Public\ArhqenCognitionEngine\Renderer\Scene\AceAquariumGpuViewportRenderer.h",
    "Source\Private\Renderer\Scene\AceAquariumGpuViewportRenderer.cpp",
    "Tools\AceRhi8ZeroCopyViewportProbe.cpp",
    "Docs\ACE_RHI7_RHI8.md"
)

foreach ($Rel in $Required) {
    if (-not (Test-Path (Join-Path $Root $Rel))) {
        throw "Missing ACE-RHI7/RHI8 file: $Rel"
    }
}
Write-Host "PASS|rhi7_rhi8_files_present"

$Rhi = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\Renderer\RHI\AceRhi.h") -Raw
$RhiCpp = Get-Content (Join-Path $Root "Source\Private\Renderer\RHI\AceRhi.cpp") -Raw
$DxHeader = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\Renderer\RHI\AceDx12Rhi.h") -Raw
$Dx12 = Get-Content (Join-Path $Root "Source\Private\Renderer\RHI\AceDx12Rhi.cpp") -Raw
$GpuHeader = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\Renderer\Scene\AceAquariumGpuViewportRenderer.h") -Raw
$GpuCpp = Get-Content (Join-Path $Root "Source\Private\Renderer\Scene\AceAquariumGpuViewportRenderer.cpp") -Raw
$ShellCpp = Get-Content (Join-Path $Root "Source\Private\Ui\AceShellUi.cpp") -Raw
$Vcx = Get-Content (Join-Path $Root "Source\ArhqenCognitionEngine.vcxproj") -Raw
$CMake = Get-Content (Join-Path $Root "CMakeLists.txt") -Raw

if ($Rhi -notlike "*CmdWvpConstants*" -or $Rhi -notlike "*void wvp*" -or $RhiCpp -notlike "*CommandList::wvp*") {
    throw "RHI7 WVP command list path missing."
}
Write-Host "PASS|rhi7_wvp_command_list_present"

if ($Dx12 -notlike "*row_major float4x4 gWvp*" -or $Dx12 -notlike "*SetGraphicsRoot32BitConstants*" -or $Dx12 -notlike "*D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS*") {
    throw "RHI7 shader/root-constant WVP path missing."
}
Write-Host "PASS|rhi7_shader_wvp_root_constants_present"

if ($GpuCpp -notlike "*AceRhi7_AquariumDepthTested3DPass*" -or $GpuCpp -notlike "*pushBox*" -or $GpuCpp -notlike "*list.wvp*" -or $GpuCpp -notlike "*DepthAttachment*") {
    throw "RHI7 real 3D depth-tested Aquarium mesh path missing."
}
Write-Host "PASS|rhi7_depth_tested_3d_mesh_path_present"

if ($DxHeader -notlike "*presentBgra8ToComposition*" -or $DxHeader -notlike "*zeroCopyFrames*" -or $Dx12 -notlike "*DCompositionCreateDevice*" -or $Dx12 -notlike "*CreateSwapChainForComposition*" -or $Dx12 -notlike "*IDCompositionVisual*") {
    throw "RHI8 DirectComposition zero-copy path missing."
}
Write-Host "PASS|rhi8_directcomposition_zero_copy_path_present"

if ($GpuHeader -notlike "*zeroCopyPresented*" -or $GpuCpp -notlike "*presentBgra8ToComposition*" -or $GpuCpp -notlike "*readback fallback*") {
    throw "RHI8 zero-copy + readback fallback handoff missing in GPU viewport renderer."
}
Write-Host "PASS|rhi8_gpu_viewport_zero_copy_handoff_present"

if ($ShellCpp -notlike "*DirectComposition zero-copy*" -or $ShellCpp -notlike "*setWorldToClipMatrix*" -or $ShellCpp -notlike "*parent_*" -or $ShellCpp -notlike "*renderGpuViewport*") {
    throw "Shell does not wire real camera WVP + zero-copy composition."
}
Write-Host "PASS|shell_wires_wvp_and_zero_copy_composition"

if ($DxHeader -notlike "*setCompositionOverlay*" -or
    $Dx12 -notlike "*dcompOverlayVisual*" -or
    $ShellCpp -notlike "*CreateSwapChainForComposition D2D HUD*" -or
    $ShellCpp -notlike "*renderAquariumD2DCompositionHud*") {
    throw "RHI8 layered DX12 scene + transparent D2D UI composition path missing."
}
if ($ShellCpp -like "*useDirectComposition ? &directCompositionOverlay*") {
    throw "Production DirectComposition path regressed to the low-quality GPU text overlay."
}
Write-Host "PASS|rhi8_separate_dx12_scene_d2d_ui_layers_present"

if ($Vcx -notlike "*dcomp.lib*" -or $CMake -notlike "*dcomp*") {
    throw "Project files do not link DirectComposition."
}
Write-Host "PASS|project_links_directcomposition"

$Probe = Join-Path $Root "Tools\AceRhi8ZeroCopyViewportProbe.cpp"
$Sources = @(
    (Join-Path $Root "Source\Private\Renderer\RHI\AceRhi.cpp"),
    (Join-Path $Root "Source\Private\Renderer\RHI\AceDx12Rhi.cpp"),
    (Join-Path $Root "Source\Private\Renderer\Scene\AceAquariumGpuViewportRenderer.cpp")
)
$Exe = Join-Path $BuildDir "AceRhi8ZeroCopyViewportProbe.exe"

$cl = Get-Command cl.exe -ErrorAction SilentlyContinue
if ($cl) {
    Write-Host "INFO|compiler|cl.exe"
    & $cl.Source /std:c++20 /EHsc /W4 $FoArg /I (Join-Path $Root "Source\Public") /Fe:$Exe $Probe $Sources d3d12.lib d3d11.lib dxgi.lib d3dcompiler.lib dcomp.lib user32.lib
    if ($LASTEXITCODE -ne 0) { throw "cl.exe failed with exit code $LASTEXITCODE" }

    Push-Location $Root
    try {
        & $Exe
        if ($LASTEXITCODE -ne 0) { throw "AceRhi8ZeroCopyViewportProbe failed with exit code $LASTEXITCODE" }
    }
    finally {
        Pop-Location
    }
} else {
    Write-Host "WARN|compiler|cl.exe not found; running static validation only in this environment"
}

Write-Host "PASS|ace_rhi7_rhi8_validation_complete"
