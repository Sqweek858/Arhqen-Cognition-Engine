$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$Probe = Join-Path $Root "Tools\AceViewportTextureBridgeProbe.cpp"
$Out = Join-Path $Root "Build\Generated\AceViewportTextureBridgeProbe.exe"
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Out) | Out-Null

$Compiler = "cl.exe"
$cl = Get-Command $Compiler -ErrorAction SilentlyContinue
if ($null -eq $cl) {
    $gpp = Get-Command "g++.exe" -ErrorAction SilentlyContinue
    if ($null -eq $gpp) { throw "No C++ compiler found for AceViewportTextureBridgeProbe." }
    & $gpp.Source -std=c++17 -I$Root $Probe -o $Out
} else {
    & $cl.Source /nologo /std:c++17 /EHsc /I$Root $Probe /Fe:$Out | Write-Host
}

& $Out
if ($LASTEXITCODE -ne 0) { throw "ACE-VTBRIDGE0 probe failed." }
