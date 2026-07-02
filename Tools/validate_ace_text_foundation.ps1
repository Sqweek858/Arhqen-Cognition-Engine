param([string]$Root=(Resolve-Path (Join-Path $PSScriptRoot "..")).Path)
$ErrorActionPreference = "Stop"
$Build = Join-Path $Root "Build\ACE-TEXT-FOUNDATION"
$Objects = Join-Path $Build "obj"
New-Item -ItemType Directory -Force $Objects | Out-Null
$Compiler = Get-Command cl.exe -ErrorAction SilentlyContinue
if (-not $Compiler) { throw "cl.exe not found" }
$Sources = @(
    (Join-Path $Root "Tools\AceTextFoundationProbe.cpp"),
    (Join-Path $Root "Source\Private\Ui\D2D\DWriteFontEngine.cpp"),
    (Join-Path $Root "Source\Private\Ui\D2D\DWriteTextCache.cpp"),
    (Join-Path $Root "Source\Private\Ui\Core\AceUiStyleSet.cpp")
)
& $Compiler.Source /nologo /std:c++20 /W4 /WX /EHsc /permissive- /DUNICODE /D_UNICODE /DNOMINMAX /DWIN32_LEAN_AND_MEAN /I"$(Join-Path $Root 'Source\Public')" /Fo"$Objects\" $Sources dwrite.lib /link /out:"$(Join-Path $Build 'AceTextFoundationProbe.exe')"
if ($LASTEXITCODE) { throw "Text foundation probe compile failed: $LASTEXITCODE" }
& (Join-Path $Build "AceTextFoundationProbe.exe")
if ($LASTEXITCODE) { throw "Text foundation probe failed: $LASTEXITCODE" }
$Foundation = Get-Content (Join-Path $Root "Source\Private\Ui\D2D\D2DTextLayoutFoundation.cpp") -Raw
if ($Foundation -like "*toDraw = EllipsizeToFit*") { throw "Draw path still manually slices Unicode for ellipsis" }
if ($Foundation -notlike "*SnapTextCoordinate(rect.left)*") { throw "Text draw origin is not pixel snapped" }
Write-Output "PASS|ace_text_foundation_validation"
