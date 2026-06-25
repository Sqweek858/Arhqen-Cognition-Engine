$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $Root "Build\ACE-AQCPP4"
New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null

$Probe = Join-Path $Root "Tools\AceAqCpp4Probe.cpp"
if (-not (Test-Path $Probe)) {
    throw "Missing probe: Tools\AceAqCpp4Probe.cpp"
}

# AQUI0R1 validator hardening:
# Older probes still test their original milestone behavior, but the shared
# AceAqEnvironment now depends on later core files such as delayed effects
# and dynamic world. Therefore all validators compile the complete current
# Aquarium core instead of stale historical subsets.
$CoreFiles = Get-ChildItem (Join-Path $Root "Source\Private\Aquarium") -Filter "AceAq*.cpp" |
    Sort-Object Name |
    ForEach-Object { $_.FullName }

if (-not $CoreFiles -or $CoreFiles.Count -eq 0) {
    throw "No Aquarium core source files found."
}

$Exe = Join-Path $BuildDir "AceAqCpp4Probe.exe"

$cl = Get-Command cl.exe -ErrorAction SilentlyContinue
if ($cl) {
    Write-Host "INFO|compiler|cl.exe"
    & $cl.Source /std:c++20 /EHsc /W4 /I (Join-Path $Root "Source\Public") /Fe:$Exe $Probe $CoreFiles
    if ($LASTEXITCODE -ne 0) {
        throw "cl.exe failed with exit code $LASTEXITCODE"
    }
} else {
    $gpp = Get-Command g++.exe -ErrorAction SilentlyContinue
    if (-not $gpp) {
        $gpp = Get-Command g++ -ErrorAction SilentlyContinue
    }
    if (-not $gpp) {
        throw "No C++ compiler found. Run from Developer PowerShell with cl.exe or install g++."
    }

    Write-Host "INFO|compiler|g++"
    & $gpp.Source -std=c++20 -Wall -Wextra -pedantic -I (Join-Path $Root "Source\Public") -o $Exe $Probe $CoreFiles
    if ($LASTEXITCODE -ne 0) {
        throw "g++ failed with exit code $LASTEXITCODE"
    }
}

Push-Location $Root
try {
    & $Exe
    if ($LASTEXITCODE -ne 0) {
        throw "AceAqCpp4Probe.cpp failed with exit code $LASTEXITCODE"
    }
}
finally {
    Pop-Location
}

Write-Host "PASS|ace-aqcpp4_validator|AceAqCpp4Probe.cpp passed against current complete Aquarium core"
