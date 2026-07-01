$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $Root "Build\ACE-AQ3D11"
New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
$ObjDir = Join-Path $BuildDir "obj"
New-Item -ItemType Directory -Force -Path $ObjDir | Out-Null
$FoArg = "/Fo$ObjDir\"


$ShellHeader = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\Ui\AceShellUi.h") -Raw
$ShellCpp = Get-Content (Join-Path $Root "Source\Private\Ui\AceShellUi.cpp") -Raw


if ($ShellHeader -notlike "*aquariumResizeQuarantineActive_*" -or $ShellHeader -notlike "*aquariumResizeQuarantineDelaySeconds_*") {
    throw "AQ3D11R3 resize quarantine fields missing."
}
Write-Host "PASS|resize_quarantine_fields_present"

if ($ShellCpp -notlike "*no 2D/3D ping-pong during resize*" -or $ShellCpp -notlike "*Real DX12 3D resize freeze active*" -or $ShellCpp -notlike "*aquariumResizeQuarantineDelaySeconds_ -=*") {
    throw "AQ3D11R5 resize freeze branch missing."
}
Write-Host "PASS|resize_freeze_tick_branch_present"

if ($ShellCpp -notlike "*side-panel resize freezes the child DX12 viewport*" -or $ShellCpp -notlike "*beginAquariumPanelResize*" -or $ShellCpp -like "*HideForLiveResize*") {
    throw "AQ3D11R5 panel resize freeze missing or old hide/show path remains."
}
Write-Host "PASS|panel_resize_uses_freeze_not_proxy"

if ($ShellCpp -notlike "*post-resize quarantine delay*" -or $ShellCpp -notlike "*Do not Show/Move/resize*") {
    throw "AQ3D11R5 sync/restore guards missing."
}
Write-Host "PASS|child_hwnd_sync_guarded_during_resize_freeze"


if ($ShellHeader -notlike "*bool aquariumUseSingleHwndCompositeViewport_ = false*") {
    throw "AQ3D11R1 requires real DX12 3D path by default; composite viewport flag must be false."
}
Write-Host "PASS|real_dx12_3d_path_default_enabled"

if ($ShellCpp -like "*Single-HWND composite viewport*" -or $ShellCpp -like "*Single-HWND composited viewport active*") {
    throw "Old Single-HWND composite viewport label is still user-visible."
}
Write-Host "PASS|legacy_single_hwnd_composite_not_user_visible"

if ($ShellCpp -notlike "*Real DX12 3D active | RMB look | WASD move | Q/E vertical*") {
    throw "Real 3D status text missing."
}
Write-Host "PASS|real_3d_status_text_present"

if ($ShellCpp -like "*Resizing viewport proxy active*" -or $ShellCpp -like "*Resizing real DX12 viewport - D2D proxy active*" -or $ShellCpp -like "*renderAquariumResizeProxyViewport(ctx, rect)*" -or $ShellCpp -like "*HideForLiveResize*") {
    throw "AQ3D11R5 must not swap to D2D proxy or hide/show child HWND during resize."
}
Write-Host "PASS|resize_does_not_swap_to_2d_proxy"

if ($ShellCpp -notlike "*bool AceShellUi::shouldFreezeNativeLiveResize() const*" -or $ShellCpp -notlike "*return false;*") {
    throw "Native resize freeze must be disabled for AQ3D11R2."
}
Write-Host "PASS|native_resize_freeze_disabled_for_real3d"


$CameraHeader = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\AquariumRender\AceAquariumCamera.h") -Raw
$CameraCpp = Get-Content (Join-Path $Root "Source\Private\AquariumRender\AceAquariumCamera.cpp") -Raw
$ViewportHeader = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\AquariumRender\AceAquariumEmbeddedDx12Viewport.h") -Raw
$ViewportCpp = Get-Content (Join-Path $Root "Source\Private\AquariumRender\AceAquariumEmbeddedDx12Viewport.cpp") -Raw
$RendererHeader = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\Renderer\Dx12Renderer.h") -Raw
$RendererCpp = Get-Content (Join-Path $Root "Source\Private\Renderer\Dx12Renderer.cpp") -Raw
$SceneHeader = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\Renderer\Scene3DDrawList.h") -Raw
$CMake = Get-Content (Join-Path $Root "CMakeLists.txt") -Raw
$Vcx = Get-Content (Join-Path $Root "Source\ArhqenCognitionEngine.vcxproj") -Raw
$Filters = Get-Content (Join-Path $Root "Source\ArhqenCognitionEngine.vcxproj.filters") -Raw

$Required = @(
    "Source\Public\ArhqenCognitionEngine\Renderer\Scene3DDrawList.h",
    "Source\Private\Renderer\Scene3DDrawList.cpp",
    "Tools\AceAq3D11RealCameraProbe.cpp",
    "Docs\ACE_AQ3D11.md"
)

foreach ($rel in $Required) {
    if (-not (Test-Path (Join-Path $Root $rel))) {
        throw "Missing AQ3D11 file: $rel"
    }
}

if ($CameraHeader -notlike "*class AceAquariumRealCamera*" -or $CameraHeader -notlike "*AceAqCameraInput*" -or $CameraCpp -notlike "*Forward()*" -or $CameraCpp -notlike "*ViewProjectionMatrix*") {
    throw "Real camera class/matrices are missing."
}
Write-Host "PASS|real_camera_class_exists"

if ($CameraCpp -notlike "*input.moveForward*" -or $CameraCpp -notlike "*movement = Add(movement, forward)*" -or $CameraCpp -notlike "*input.moveBackward*") {
    throw "W/S do not use camera forward."
}
Write-Host "PASS|ws_uses_camera_forward"

if ($CameraCpp -notlike "*input.moveRight*" -or $CameraCpp -notlike "*input.moveLeft*" -or $CameraCpp -notlike "*Right()*") {
    throw "A/D do not use camera right."
}
Write-Host "PASS|ad_uses_camera_right"

if ($CameraCpp -notlike "*input.moveUp*" -or $CameraCpp -notlike "*input.moveDown*" -or $CameraCpp -notlike "*WorldUp()*") {
    throw "Q/E world vertical rules missing."
}
Write-Host "PASS|qe_uses_world_vertical"

if ($CameraCpp -notlike "*if (len > 1.0f)*" -or $CameraCpp -notlike "*moveSpeed_ * dt*") {
    throw "Movement normalization or dt-based movement missing."
}
Write-Host "PASS|movement_normalized_and_dt_based"

if ($CameraCpp -notlike "*clampPitch*" -or $CameraCpp -notlike "*ApplyMouseDelta*") {
    throw "Pitch clamp or mouse look missing."
}
Write-Host "PASS|pitch_clamp_and_mouse_look"

if ($CameraCpp -notlike "*yaw_ -= deltaX * mouseSensitivity_*" -or $CameraCpp -like "*yaw_ += deltaX * mouseSensitivity_*") {
    throw "Mouse X yaw direction is not corrected for AQ3D11R2."
}
Write-Host "PASS|mouse_x_direction_corrected"

if ($RendererHeader -notlike "*renderFrame3D*" -or $RendererHeader -notlike "*Scene3DDrawList*" -or $RendererCpp -notlike "*createScene3DPipeline*" -or $RendererCpp -notlike "*ClearDepthStencilView*") {
    throw "DX12 real 3D render path/depth is missing."
}
Write-Host "PASS|dx12_real_3d_path_exists"

if ($ViewportCpp -notlike "*BuildScene3D*" -or $ViewportCpp -notlike "*renderer_.renderFrame3D*" -or $ViewportCpp -like "*BuildDrawList*") {
    throw "Embedded viewport still uses old 2D draw-list path."
}
Write-Host "PASS|embedded_viewport_uses_3d_scene_path"

if ($ViewportCpp -notlike "*GetAsyncKeyState('W')*" -or $ViewportCpp -notlike "*WM_RBUTTONDOWN*" -or $ViewportCpp -notlike "*ApplyMouseDelta*") {
    throw "WASD / mouse look input path missing."
}
Write-Host "PASS|viewport_input_controls_present"

foreach ($ref in @("Scene3DDrawList", "AceAquariumCamera", "AceAquariumEmbeddedDx12Viewport")) {
    if ($CMake -notlike "*$ref*") { throw "CMakeLists.txt missing $ref" }
    if ($Vcx -notlike "*$ref*") { throw ".vcxproj missing $ref" }
    if ($Filters -notlike "*$ref*") { throw ".vcxproj.filters missing $ref" }
}
Write-Host "PASS|project_references_exist"

$ForbiddenSources = $CameraCpp + "`n" + $ViewportCpp + "`n" + $RendererCpp + "`n" + $SceneHeader
$Forbidden = @{
    "no_python_runtime" = "Python"
    "no_panda3d" = "Panda3D"
    "no_dearpygui" = "DearPyGui"
    "no_llm" = "LLM"
    "no_tokenizer" = "tokenizer"
    "no_isometric_fake_marker" = "isometric fake"
}

foreach ($name in $Forbidden.Keys) {
    $needle = $Forbidden[$name]
    if ($ForbiddenSources -like "*$needle*") {
        throw "Forbidden marker found for $name`: $needle"
    }
    Write-Host "PASS|$name"
}

$Probe = Join-Path $Root "Tools\AceAq3D11RealCameraProbe.cpp"
$CameraSrc = Join-Path $Root "Source\Private\AquariumRender\AceAquariumCamera.cpp"
$Exe = Join-Path $BuildDir "AceAq3D11RealCameraProbe.exe"

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
    if ($LASTEXITCODE -ne 0) { throw "AceAq3D11RealCameraProbe failed with exit code $LASTEXITCODE" }
}
finally {
    Pop-Location
}

if (Test-Path (Join-Path $Root "Tools\validate_ace_aq3d10.ps1")) {
    & (Join-Path $Root "Tools\validate_ace_aq3d10.ps1")
    if ($LASTEXITCODE -ne 0) { throw "validate_ace_aq3d10.ps1 failed with exit code $LASTEXITCODE" }
    Write-Host "PASS|aq3d10_resize_hardening_still_present"
}

Write-Host "PASS|ace_aq3d11_validation_complete"
