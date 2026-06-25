$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $Root "Build\ACE-AQUI0"
New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null

$Required = @(
    "Source\Public\ArhqenCognitionEngine\AquariumUI\AceAquariumRuntimeController.h",
    "Source\Public\ArhqenCognitionEngine\AquariumUI\AceAquariumUiSnapshot.h",
    "Source\Public\ArhqenCognitionEngine\AquariumUI\AceAquariumPanel.h",
    "Source\Public\ArhqenCognitionEngine\AquariumUI\AceAquariumLogModel.h",
    "Source\Private\AquariumUI\AceAquariumRuntimeController.cpp",
    "Source\Private\AquariumUI\AceAquariumUiSnapshot.cpp",
    "Source\Private\AquariumUI\AceAquariumPanel.cpp",
    "Source\Private\AquariumUI\AceAquariumLogModel.cpp",
    "Tools\AceAqui0Probe.cpp",
    "Docs\ACE_AQUI0.md"
)

foreach ($rel in $Required) {
    if (-not (Test-Path (Join-Path $Root $rel))) {
        throw "Missing required file: $rel"
    }
}

$SourceText = Get-ChildItem (Join-Path $Root "Source\Public\ArhqenCognitionEngine\Aquarium"), (Join-Path $Root "Source\Private\Aquarium"), (Join-Path $Root "Source\Public\ArhqenCognitionEngine\AquariumUI"), (Join-Path $Root "Source\Private\AquariumUI") -Recurse -File |
    Where-Object { $_.Extension -in @(".h", ".cpp") } |
    ForEach-Object { Get-Content $_.FullName -Raw }

$Joined = $SourceText -join "`n"
$Forbidden = @("Panda3D", "DearPyGui", "Gymnasium", "LLM", "tokenizer")
foreach ($needle in $Forbidden) {
    if ($Joined -like "*$needle*") {
        throw "Forbidden runtime reference found: $needle"
    }
    Write-Host "PASS|no_$($needle.ToLower())|$needle"
}

if ($Joined -like "*Python*" -or $Joined -like "*python*") {
    throw "Python runtime reference found in Aquarium/AquariumUI source."
}
Write-Host "PASS|no_python_runtime"

$RendererText = Get-ChildItem (Join-Path $Root "Source\Private\Renderer"), (Join-Path $Root "Source\Public\ArhqenCognitionEngine\Renderer") -Recurse -File -ErrorAction SilentlyContinue |
    Where-Object { $_.Extension -in @(".h", ".cpp") } |
    ForEach-Object { Get-Content $_.FullName -Raw }

if (($RendererText -join "`n") -like "*Aquarium*") {
    throw "DX12/Renderer Aquarium integration found prematurely."
}
Write-Host "PASS|no_dx12_aquarium_renderer_yet"

$Vcx = Join-Path $Root "Source\ArhqenCognitionEngine.vcxproj"
$ProjectText = Get-Content $Vcx -Raw
foreach ($needle in @(
    "AquariumUI\AceAquariumRuntimeController.cpp",
    "AquariumUI\AceAquariumUiSnapshot.cpp",
    "AquariumUI\AceAquariumPanel.cpp",
    "AquariumUI\AceAquariumLogModel.cpp"
)) {
    if ($ProjectText -notlike "*$needle*") {
        throw "Project file missing reference: $needle"
    }
}
Write-Host "PASS|project_references_exist"

# AQUI0R1 validator hardening: compile the current complete Aquarium core.
$CoreFiles = Get-ChildItem (Join-Path $Root "Source\Private\Aquarium") -Filter "AceAq*.cpp" | Sort-Object Name | ForEach-Object { $_.FullName }
$UiFiles = Get-ChildItem (Join-Path $Root "Source\Private\AquariumUI") -Filter "AceAquarium*.cpp" | ForEach-Object { $_.FullName }
$AllRuntimeFiles = $CoreFiles + $UiFiles

function Invoke-Probe($ProbeName, $ExeName, [bool]$WithUi) {
    $Exe = Join-Path $BuildDir $ExeName
    $Files = $CoreFiles
    if ($WithUi) {
        $Files = $AllRuntimeFiles
    }

    $cl = Get-Command cl.exe -ErrorAction SilentlyContinue
    if ($cl) {
        Write-Host "INFO|compiler|cl.exe"
        & $cl.Source /std:c++20 /EHsc /W4 /I (Join-Path $Root "Source\Public") /Fe:$Exe (Join-Path $Root "Tools\$ProbeName") $Files
        if ($LASTEXITCODE -ne 0) { throw "cl.exe failed for $ProbeName with exit code $LASTEXITCODE" }
    } else {
        $gpp = Get-Command g++.exe -ErrorAction SilentlyContinue
        if (-not $gpp) { $gpp = Get-Command g++ -ErrorAction SilentlyContinue }
        if (-not $gpp) { throw "No C++ compiler found. Run from Developer PowerShell with cl.exe or install g++." }

        Write-Host "INFO|compiler|g++"
        & $gpp.Source -std=c++20 -Wall -Wextra -pedantic -I (Join-Path $Root "Source\Public") -o $Exe (Join-Path $Root "Tools\$ProbeName") $Files
        if ($LASTEXITCODE -ne 0) { throw "g++ failed for $ProbeName with exit code $LASTEXITCODE" }
    }

    Push-Location $Root
    try {
        & $Exe
        if ($LASTEXITCODE -ne 0) { throw "$ProbeName failed with exit code $LASTEXITCODE" }
    }
    finally {
        Pop-Location
    }
}

Invoke-Probe "AceAqCpp1Probe.cpp" "AceAqCpp1Probe.exe" $false
Write-Host "PASS|cpp1_probe_still_passes"
Invoke-Probe "AceAqCpp2Probe.cpp" "AceAqCpp2Probe.exe" $false
Write-Host "PASS|cpp2_probe_still_passes"
Invoke-Probe "AceAqCpp3Probe.cpp" "AceAqCpp3Probe.exe" $false
Write-Host "PASS|cpp3_probe_still_passes"
Invoke-Probe "AceAqCpp4Probe.cpp" "AceAqCpp4Probe.exe" $false
Write-Host "PASS|cpp4_probe_still_passes"
Invoke-Probe "AceAqui0Probe.cpp" "AceAqui0Probe.exe" $true
Write-Host "PASS|ace_aqui0_validator|runtime controller and snapshot checks passed"
