$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $Root "Build\ACE-AQ3D0"
New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
$ObjDir = Join-Path $BuildDir "obj"
New-Item -ItemType Directory -Force -Path $ObjDir | Out-Null
$FoArg = "/Fo$ObjDir\\"

function Invoke-ExistingValidator {
    param(
        [string]$ScriptName,
        [string]$PassName
    )

    $ScriptPath = Join-Path $Root "Tools\$ScriptName"
    if (-not (Test-Path $ScriptPath)) {
        throw "Missing validator: $ScriptName"
    }

    & $ScriptPath
    if ($LASTEXITCODE -ne 0) {
        throw "$ScriptName failed with exit code $LASTEXITCODE"
    }

    Write-Host "PASS|$PassName"
}

$Required = @(
    "Source\Public\ArhqenCognitionEngine\AquariumRender\AceAquariumRenderPrimitive.h",
    "Source\Public\ArhqenCognitionEngine\AquariumRender\AceAquariumSceneAdapter.h",
    "Source\Public\ArhqenCognitionEngine\AquariumRender\AceAquariumViewport.h",
    "Source\Public\ArhqenCognitionEngine\AquariumRender\AceAquariumCamera.h",
    "Source\Private\AquariumRender\AceAquariumRenderPrimitive.cpp",
    "Source\Private\AquariumRender\AceAquariumSceneAdapter.cpp",
    "Source\Private\AquariumRender\AceAquariumViewport.cpp",
    "Source\Private\AquariumRender\AceAquariumCamera.cpp",
    "Tools\AceAq3D0Probe.cpp",
    "Docs\ACE_AQ3D0.md"
)

foreach ($rel in $Required) {
    if (-not (Test-Path (Join-Path $Root $rel))) {
        throw "Missing required AQ3D0 file: $rel"
    }
}

$CMake = Get-Content (Join-Path $Root "CMakeLists.txt") -Raw
$Vcx = Get-Content (Join-Path $Root "Source\ArhqenCognitionEngine.vcxproj") -Raw
$Filters = Get-Content (Join-Path $Root "Source\ArhqenCognitionEngine.vcxproj.filters") -Raw
$UiCpp = Get-Content (Join-Path $Root "Source\Private\Ui\AceShellUi.cpp") -Raw
$AdapterCpp = Get-Content (Join-Path $Root "Source\Private\AquariumRender\AceAquariumSceneAdapter.cpp") -Raw

$Refs = @(
    "AceAquariumRenderPrimitive",
    "AceAquariumSceneAdapter",
    "AceAquariumViewport",
    "AceAquariumCamera"
)

foreach ($ref in $Refs) {
    if ($CMake -notlike "*$ref*") { throw "CMakeLists.txt missing $ref" }
    if ($Vcx -notlike "*$ref*") { throw ".vcxproj missing $ref" }
    if ($Filters -notlike "*$ref*") { throw ".vcxproj.filters missing $ref" }
}
Write-Host "PASS|project_references_exist"

if ($UiCpp -notlike "*3D Viewport*" -or $UiCpp -notlike "*renderAquariumViewportCard*" -or $UiCpp -notlike "*D2D/isometric fallback*") {
    throw "Environment panel does not contain AQ3D0 viewport entry/render path."
}
Write-Host "PASS|environment_panel_has_viewport_entry"

if ($UiCpp -notlike "*DEBUG TRUTH - NOT AGENT INPUT*" -or $AdapterCpp -notlike "*DEBUG TRUTH - NOT AGENT INPUT*") {
    throw "Debug truth warning missing."
}
Write-Host "PASS|debug_truth_warning_present"

$ForbiddenRuntimeMarkers = @{
    "no_python_runtime" = "Python"
    "no_panda3d" = "Panda3D"
    "no_dearpygui" = "DearPyGui"
    "no_llm" = "LLM"
    "no_tokenizer" = "tokenizer"
}

foreach ($name in $ForbiddenRuntimeMarkers.Keys) {
    $needle = $ForbiddenRuntimeMarkers[$name]
    if ($UiCpp -like "*$needle*" -or $AdapterCpp -like "*$needle*") {
        throw "Forbidden runtime marker found for $name: $needle"
    }
    Write-Host "PASS|$name"
}

$CoreFiles = Get-ChildItem (Join-Path $Root "Source\Private\Aquarium") -Filter "AceAq*.cpp" | Sort-Object Name | ForEach-Object { $_.FullName }
$UiFiles = Get-ChildItem (Join-Path $Root "Source\Private\AquariumUI") -Filter "AceAquarium*.cpp" | Sort-Object Name | ForEach-Object { $_.FullName }
$RenderFiles = Get-ChildItem (Join-Path $Root "Source\Private\AquariumRender") -Filter "AceAquarium*.cpp" | Where-Object { $_.Name -ne "AceAquariumDx12Viewport.cpp" } | Sort-Object Name | ForEach-Object { $_.FullName }
# DX12 viewport excluded from headless AQ3D0 probe; AQ3D0R validates it statically/build-time.
$Probe = Join-Path $Root "Tools\AceAq3D0Probe.cpp"
$Exe = Join-Path $BuildDir "AceAq3D0Probe.exe"

$cl = Get-Command cl.exe -ErrorAction SilentlyContinue
if ($cl) {
    Write-Host "INFO|compiler|cl.exe"
    & $cl.Source /std:c++20 /EHsc /W4 $FoArg /I (Join-Path $Root "Source\Public") /Fe:$Exe $Probe $CoreFiles $UiFiles $RenderFiles
    if ($LASTEXITCODE -ne 0) { throw "cl.exe failed with exit code $LASTEXITCODE" }
} else {
    $gpp = Get-Command g++.exe -ErrorAction SilentlyContinue
    if (-not $gpp) { $gpp = Get-Command g++ -ErrorAction SilentlyContinue }
    if (-not $gpp) { throw "No C++ compiler found. Run from Developer PowerShell or install g++." }

    Write-Host "INFO|compiler|g++"
    & $gpp.Source -std=c++20 -Wall -Wextra -pedantic -I (Join-Path $Root "Source\Public") -o $Exe $Probe $CoreFiles $UiFiles $RenderFiles
    if ($LASTEXITCODE -ne 0) { throw "g++ failed with exit code $LASTEXITCODE" }
}

Push-Location $Root
try {
    & $Exe
    if ($LASTEXITCODE -ne 0) { throw "AceAq3D0Probe failed with exit code $LASTEXITCODE" }
}
finally {
    Pop-Location
}

Invoke-ExistingValidator "validate_ace_aqcpp1.ps1" "cpp1_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aqcpp2.ps1" "cpp2_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aqcpp3.ps1" "cpp3_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aqcpp4.ps1" "cpp4_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aqui0.ps1" "aqui0_probe_still_passes"

Write-Host "PASS|ace_aq3d0_validation_complete"
