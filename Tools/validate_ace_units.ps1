param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
)

$ErrorActionPreference = "Stop"

$BuildDir = Join-Path $Root "Build\ACE-UNITS"
$ObjDir = Join-Path $BuildDir "obj"
New-Item -ItemType Directory -Force -Path $ObjDir | Out-Null

$Compiler = Get-Command cl.exe -ErrorAction SilentlyContinue
if (!$Compiler) {
    throw "cl.exe was not found. Run from a Visual Studio Developer PowerShell."
}

$Include = Join-Path $Root "Source\Public"
$Probe = Join-Path $Root "Tools\AceUnitsProbe.cpp"
$Implementation = Join-Path $Root "Source\Private\Core\Units\AceUnits.cpp"
$Executable = Join-Path $BuildDir "AceUnitsProbe.exe"

& $Compiler.Source /nologo /std:c++20 /W4 /WX /EHsc /permissive- /I"$Include" `
    /Fo"$ObjDir\" $Probe $Implementation /link /out:"$Executable"
if ($LASTEXITCODE -ne 0) {
    throw "ACE units probe compilation failed with exit code $LASTEXITCODE."
}

& $Executable
if ($LASTEXITCODE -ne 0) {
    throw "ACE units probe failed with exit code $LASTEXITCODE."
}

Write-Output "PASS|ace_units_validation"
