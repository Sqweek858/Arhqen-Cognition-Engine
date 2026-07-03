param([string]$Root=(Resolve-Path (Join-Path $PSScriptRoot "..")).Path)
$ErrorActionPreference = "Stop"
$Build = Join-Path $Root "Build\ACE-ASSET-REGISTRY"
$Objects = Join-Path $Build "obj"
New-Item -ItemType Directory -Force $Objects | Out-Null
$Compiler = Get-Command cl.exe -ErrorAction SilentlyContinue
if (-not $Compiler) { throw "cl.exe not found" }
$Sources = @(
    (Join-Path $Root "Tools\AceAssetRegistryProbe.cpp"),
    (Join-Path $Root "Source\Private\Core\Assets\AceAssetRegistry.cpp"),
    (Join-Path $Root "Source\Private\Core\Assets\AceAssetDirectoryWatcher.cpp"),
    (Join-Path $Root "Source\Private\Core\Assets\AceAssetPath.cpp"),
    (Join-Path $Root "Source\Private\Core\Identity\AceGuid.cpp"),
    (Join-Path $Root "Source\Private\Core\Serialization\AceArchive.cpp"),
    (Join-Path $Root "Source\Private\Core\Serialization\AceAtomicFile.cpp")
)
& $Compiler.Source /nologo /std:c++20 /W4 /WX /EHsc /permissive- /DUNICODE /D_UNICODE /DNOMINMAX /DWIN32_LEAN_AND_MEAN /I"$(Join-Path $Root 'Source\Public')" /Fo"$Objects\" $Sources /link /out:"$(Join-Path $Build 'AceAssetRegistryProbe.exe')"
if ($LASTEXITCODE) { throw "Asset Registry probe compile failed: $LASTEXITCODE" }
Push-Location $Root
try { & (Join-Path $Build "AceAssetRegistryProbe.exe") } finally { Pop-Location }
if ($LASTEXITCODE) { throw "Asset Registry probe failed: $LASTEXITCODE" }
Write-Output "PASS|ace_asset_registry_validation"
