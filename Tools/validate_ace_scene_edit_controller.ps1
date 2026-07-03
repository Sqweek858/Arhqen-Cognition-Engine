param([string]$Root=(Resolve-Path (Join-Path $PSScriptRoot "..")).Path)
$ErrorActionPreference = "Stop"
$Build = Join-Path $Root "Build\ACE-SCENE-EDIT"
$Objects = Join-Path $Build "obj"
New-Item -ItemType Directory -Force $Objects | Out-Null
$Compiler = Get-Command cl.exe -ErrorAction SilentlyContinue
if (-not $Compiler) { throw "cl.exe not found" }
$Sources = @(
    (Join-Path $Root "Tools\AceSceneEditControllerProbe.cpp"),
    (Join-Path $Root "Source\Private\Editor\Scene\AceSceneEditController.cpp"),
    (Join-Path $Root "Source\Private\Editor\Transactions\AceTransaction.cpp"),
    (Join-Path $Root "Source\Private\Core\Scene\AceSceneWorld.cpp"),
    (Join-Path $Root "Source\Private\Core\Identity\AceGuid.cpp"),
    (Join-Path $Root "Source\Private\Core\Serialization\AceArchive.cpp"),
    (Join-Path $Root "Source\Private\Core\Serialization\AceAtomicFile.cpp")
)
& $Compiler.Source /nologo /std:c++20 /W4 /WX /EHsc /permissive- /DUNICODE /D_UNICODE /DNOMINMAX /DWIN32_LEAN_AND_MEAN /I"$(Join-Path $Root 'Source\Public')" /Fo"$Objects\" $Sources /link /out:"$(Join-Path $Build 'AceSceneEditControllerProbe.exe')"
if ($LASTEXITCODE) { throw "Scene Edit probe compile failed: $LASTEXITCODE" }
Push-Location $Root
try { & (Join-Path $Build "AceSceneEditControllerProbe.exe") } finally { Pop-Location }
if ($LASTEXITCODE) { throw "Scene Edit probe failed: $LASTEXITCODE" }
Write-Output "PASS|ace_scene_edit_controller_validation"
