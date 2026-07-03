param([string]$Root=(Resolve-Path (Join-Path $PSScriptRoot "..")).Path)
$ErrorActionPreference = "Stop"
$Build = Join-Path $Root "Build\ACE-CAMERA-SPEED"
$Objects = Join-Path $Build "obj"
New-Item -ItemType Directory -Force $Objects | Out-Null
$Compiler = Get-Command cl.exe -ErrorAction SilentlyContinue
if (-not $Compiler) { throw "cl.exe not found" }
$Sources = @(
    (Join-Path $Root "Tools\AceCameraSpeedModelProbe.cpp"),
    (Join-Path $Root "Source\Private\Editor\Viewport\AceCameraSpeedModel.cpp"),
    (Join-Path $Root "Source\Private\AquariumRender\AceAquariumCamera.cpp")
)
& $Compiler.Source /nologo /std:c++20 /W4 /WX /EHsc /permissive- /I"$(Join-Path $Root 'Source\Public')" /Fo"$Objects\" $Sources /link /out:"$(Join-Path $Build 'AceCameraSpeedModelProbe.exe')"
if ($LASTEXITCODE) { throw "Camera speed model probe compile failed: $LASTEXITCODE" }
& (Join-Path $Build "AceCameraSpeedModelProbe.exe")
if ($LASTEXITCODE) { throw "Camera speed model probe failed: $LASTEXITCODE" }
Write-Output "PASS|ace_camera_speed_validation"
