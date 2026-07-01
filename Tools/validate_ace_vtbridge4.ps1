$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$probe = Join-Path $root 'Tools\AceViewportTextureBridge4Probe.cpp'
$out = Join-Path $root 'Build\Generated\AceViewportTextureBridge4Probe.exe'
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $out) | Out-Null
$cxx = Get-Command cl.exe -ErrorAction SilentlyContinue
if ($cxx) {
    Push-Location $root
    try { cl.exe /nologo /std:c++20 /EHsc $probe /Fe:$out | Out-Host }
    finally { Pop-Location }
} else {
    Push-Location $root
    try { g++ -std=c++20 $probe -o $out }
    finally { Pop-Location }
}
& $out
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
