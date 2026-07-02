param([string]$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path)
$ErrorActionPreference = "Stop"
$BuildDir = Join-Path $Root "Build\ACE-ARCHIVE"; $ObjDir = Join-Path $BuildDir "obj"
New-Item -ItemType Directory -Force -Path $ObjDir | Out-Null
$Compiler = Get-Command cl.exe -ErrorAction SilentlyContinue
if (!$Compiler) { throw "cl.exe was not found. Run from a Visual Studio Developer PowerShell." }
$Sources = @((Join-Path $Root "Tools\AceArchiveProbe.cpp"),(Join-Path $Root "Source\Private\Core\Identity\AceGuid.cpp"),(Join-Path $Root "Source\Private\Core\Serialization\AceArchive.cpp"),(Join-Path $Root "Source\Private\Core\Serialization\AceAtomicFile.cpp"))
& $Compiler.Source /nologo /std:c++20 /W4 /WX /EHsc /permissive- /DUNICODE /D_UNICODE /DNOMINMAX /DWIN32_LEAN_AND_MEAN /I"$(Join-Path $Root 'Source\Public')" /Fo"$ObjDir\" $Sources /link /out:"$(Join-Path $BuildDir 'AceArchiveProbe.exe')"
if ($LASTEXITCODE -ne 0) { throw "Archive probe compilation failed: $LASTEXITCODE" }
& (Join-Path $BuildDir "AceArchiveProbe.exe")
if ($LASTEXITCODE -ne 0) { throw "Archive probe failed: $LASTEXITCODE" }
Write-Output "PASS|ace_archive_validation"
