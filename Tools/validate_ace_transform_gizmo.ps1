param([string]$Root=(Resolve-Path (Join-Path $PSScriptRoot "..")).Path)
$ErrorActionPreference = "Stop"
$Build = Join-Path $Root "Build\ACE-TRANSFORM-GIZMO"; $Objects = Join-Path $Build "obj"
New-Item -ItemType Directory -Force $Objects | Out-Null
$Compiler = Get-Command cl.exe -ErrorAction SilentlyContinue
if (-not $Compiler) { throw "cl.exe not found" }
$Sources = @(
 (Join-Path $Root "Tools\AceTransformGizmoProbe.cpp"),
 (Join-Path $Root "Source\Private\Editor\Scene\AceTransformGizmo.cpp"),
 (Join-Path $Root "Source\Private\Editor\Scene\AceSceneEditController.cpp"),
 (Join-Path $Root "Source\Private\Editor\Transactions\AceTransaction.cpp"),
 (Join-Path $Root "Source\Private\Core\Scene\AceSceneWorld.cpp"),
 (Join-Path $Root "Source\Private\Core\Identity\AceGuid.cpp"),
 (Join-Path $Root "Source\Private\Core\Serialization\AceArchive.cpp"),
 (Join-Path $Root "Source\Private\Core\Serialization\AceAtomicFile.cpp")
)
& $Compiler.Source /nologo /std:c++20 /W4 /WX /EHsc /permissive- /DUNICODE /D_UNICODE /DNOMINMAX /DWIN32_LEAN_AND_MEAN /I"$(Join-Path $Root 'Source\Public')" /Fo"$Objects\" $Sources /link /out:"$(Join-Path $Build 'AceTransformGizmoProbe.exe')"
if ($LASTEXITCODE) { throw "Transform Gizmo compile failed: $LASTEXITCODE" }
Push-Location $Root; try { & (Join-Path $Build "AceTransformGizmoProbe.exe") } finally { Pop-Location }
if ($LASTEXITCODE) { throw "Transform Gizmo probe failed: $LASTEXITCODE" }
Write-Output "PASS|ace_transform_gizmo_validation"
