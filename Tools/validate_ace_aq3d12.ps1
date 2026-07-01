$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $Root "Build\ACE-AQ3D12"
New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
$ObjDir = Join-Path $BuildDir "obj"
New-Item -ItemType Directory -Force -Path $ObjDir | Out-Null
$FoArg = "/Fo$ObjDir\"

$ShellHeader = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\Ui\AceShellUi.h") -Raw
$ShellCpp = Get-Content (Join-Path $Root "Source\Private\Ui\AceShellUi.cpp") -Raw
$CameraCpp = Get-Content (Join-Path $Root "Source\Private\AquariumRender\AceAquariumCamera.cpp") -Raw

if ($ShellHeader -notlike "*aquariumSingleHwndCamera_*" -or $ShellHeader -notlike "*bool aquariumUseSingleHwndCompositeViewport_ = true*") {
    throw "AQ3D12 single-HWND camera/default path missing."
}
Write-Host "PASS|single_hwnd_3d_default_enabled"

if ($ShellCpp -notlike "*ACE-AQ3D12: main path is single-HWND composition*" -or $ShellCpp -notlike "*renderAquariumSlateCompositeViewport(ctx, rect, debugTruthEnabled)*") {
    throw "AQ3D12 render surface does not route to single-HWND composition."
}
Write-Host "PASS|surface_routes_to_single_hwnd_composition"

if ($ShellCpp -notlike "*No child HWND, no separate flip-model swapchain*" -or $ShellCpp -notlike "*aquariumEmbeddedDx12Viewport_.Hide()*") {
    throw "AQ3D12 does not suppress child HWND in main path."
}
Write-Host "PASS|child_hwnd_suppressed_in_main_path"

if ($ShellCpp -notlike "*GetAsyncKeyState('W')*" -or $ShellCpp -notlike "*aquariumSingleHwndCamera_.UpdateFromInput*" -or $ShellCpp -notlike "*ApplyMouseDelta*") {
    throw "AQ3D12 single-HWND camera input path missing."
}
Write-Host "PASS|single_hwnd_camera_input_present"


if ($ShellCpp -notlike "*ACE-AQ3D12R1: bounded scene grid*" -or $ShellCpp -like "*for (int z = -2; z <= 16*") {
    throw "AQ3D12R1 bounded scene grid missing or old fixed grid remains."
}
Write-Host "PASS|bounded_scene_grid_present"

if ($ShellCpp -notlike "*ignore old adapter grid-line primitives*" -or $ShellCpp -notlike "*former isometric GridLine primitives*") {
    throw "AQ3D12R1 must ignore old isometric grid-line primitives in the single-HWND compositor."
}
Write-Host "PASS|old_isometric_gridlines_ignored"


if ($ShellCpp -notlike "*aceProjectSingleHwnd3D*" -or $ShellCpp -notlike "*ViewProjectionMatrix*" -or $ShellCpp -notlike "*Single-HWND 3D | RMB look | WASD move | Q/E vertical*") {
    throw "AQ3D12 projected single-HWND 3D compositor missing."
}
Write-Host "PASS|single_hwnd_projected_3d_compositor_present"

if ($ShellCpp -like "*Legacy composite viewport disabled by AQ3D11R1*") {
    throw "Old legacy-disabled label still present."
}
Write-Host "PASS|old_legacy_label_removed"

if ($ShellCpp -like "*renderAquariumResizeProxyViewport(ctx, rect)*") {
    throw "AQ3D12 main path must not call resize proxy."
}
Write-Host "PASS|no_resize_proxy_call_in_main_path"

if ($CameraCpp -notlike "*yaw_ -= deltaX*" -or $CameraCpp -notlike "*clampPitch*") {
    throw "Camera mouse direction/pitch clamp missing."
}
Write-Host "PASS|camera_mouse_direction_and_pitch_clamp"

$Probe = Join-Path $Root "Tools\AceAq3D12SingleHwndProbe.cpp"
$CameraSrc = Join-Path $Root "Source\Private\AquariumRender\AceAquariumCamera.cpp"
$Exe = Join-Path $BuildDir "AceAq3D12SingleHwndProbe.exe"

$cl = Get-Command cl.exe -ErrorAction SilentlyContinue
if ($cl) {
    Write-Host "INFO|compiler|cl.exe"
    & $cl.Source /std:c++20 /EHsc /W4 $FoArg /I (Join-Path $Root "Source\Public") /Fe:$Exe $Probe $CameraSrc
    if ($LASTEXITCODE -ne 0) { throw "cl.exe failed with exit code $LASTEXITCODE" }
} else {
    $gpp = Get-Command g++.exe -ErrorAction SilentlyContinue
    if (-not $gpp) { $gpp = Get-Command g++ -ErrorAction SilentlyContinue }
    if (-not $gpp) { throw "No C++ compiler found. Run from Developer PowerShell with cl.exe or install g++." }

    Write-Host "INFO|compiler|g++"
    & $gpp.Source -std=c++20 -Wall -Wextra -pedantic -I (Join-Path $Root "Source\Public") -o $Exe $Probe $CameraSrc
    if ($LASTEXITCODE -ne 0) { throw "g++ failed with exit code $LASTEXITCODE" }
}

Push-Location $Root
try {
    & $Exe
    if ($LASTEXITCODE -ne 0) { throw "AceAq3D12SingleHwndProbe failed with exit code $LASTEXITCODE" }
}
finally {
    Pop-Location
}

Write-Host "PASS|ace_aq3d12_validation_complete"
