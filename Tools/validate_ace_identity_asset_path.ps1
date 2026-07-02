param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
)

$ErrorActionPreference = "Stop"
$BuildDir = Join-Path $Root "Build\ACE-IDENTITY-ASSET-PATH"
$ObjDir = Join-Path $BuildDir "obj"
New-Item -ItemType Directory -Force -Path $ObjDir | Out-Null

$Compiler = Get-Command cl.exe -ErrorAction SilentlyContinue
if (!$Compiler) { throw "cl.exe was not found. Run from a Visual Studio Developer PowerShell." }

$Include = Join-Path $Root "Source\Public"
$Sources = @(
    (Join-Path $Root "Tools\AceIdentityAssetPathProbe.cpp"),
    (Join-Path $Root "Source\Private\Core\Identity\AceGuid.cpp"),
    (Join-Path $Root "Source\Private\Core\Assets\AceAssetPath.cpp")
)
$Executable = Join-Path $BuildDir "AceIdentityAssetPathProbe.exe"

& $Compiler.Source /nologo /std:c++20 /W4 /WX /EHsc /permissive- /DUNICODE /D_UNICODE `
    /I"$Include" /Fo"$ObjDir\" $Sources /link /out:"$Executable"
if ($LASTEXITCODE -ne 0) { throw "Identity/asset-path probe compilation failed: $LASTEXITCODE" }

& $Executable
if ($LASTEXITCODE -ne 0) { throw "Identity/asset-path probe failed: $LASTEXITCODE" }
Write-Output "PASS|ace_identity_asset_path_validation"
