$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$probe = Join-Path $root "Tools\AceViewportTextureBridge2Probe.cpp"
$out = Join-Path $root "Build\Generated\ace_vtbridge2_probe.exe"
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $out) | Out-Null

$compiler = Get-Command cl.exe -ErrorAction SilentlyContinue
if ($compiler) {
    & cl.exe /nologo /std:c++20 /EHsc /I"$root\Source\Public" /Fe:$out $probe | Write-Host
} else {
    & g++ -std=c++20 -I"$root/Source/Public" -o $out $probe
}

& $out
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
