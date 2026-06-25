$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $Root "Build\ACE-AQ3D0R"
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
    "Source\Public\ArhqenCognitionEngine\AquariumRender\AceAquariumDx12Viewport.h",
    "Source\Private\AquariumRender\AceAquariumDx12Viewport.cpp",
    "Tools\AceAq3D0RProbe.cpp",
    "Docs\ACE_AQ3D0R.md"
)

foreach ($rel in $Required) {
    if (-not (Test-Path (Join-Path $Root $rel))) {
        throw "Missing required AQ3D0R file: $rel"
    }
}

$UiHeader = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\Ui\AceShellUi.h") -Raw
$UiCpp = Get-Content (Join-Path $Root "Source\Private\Ui\AceShellUi.cpp") -Raw
$Dx12Header = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\AquariumRender\AceAquariumDx12Viewport.h") -Raw
$Dx12Cpp = Get-Content (Join-Path $Root "Source\Private\AquariumRender\AceAquariumDx12Viewport.cpp") -Raw
$AdapterCpp = Get-Content (Join-Path $Root "Source\Private\AquariumRender\AceAquariumSceneAdapter.cpp") -Raw
$CMake = Get-Content (Join-Path $Root "CMakeLists.txt") -Raw
$Vcx = Get-Content (Join-Path $Root "Source\ArhqenCognitionEngine.vcxproj") -Raw
$Filters = Get-Content (Join-Path $Root "Source\ArhqenCognitionEngine.vcxproj.filters") -Raw

if ($UiCpp -notlike "*Open DX12 Viewport*" -and $UiCpp -notlike "*Open 3D Environment*") {
    throw "Open 3D/DX12 viewport button missing."
}
Write-Host "PASS|control_panel_has_open_3d_environment_button"

if ($UiCpp -like "*Minimal 3D/isometric Aquarium viewport is active*" -or $UiCpp -like "*D2D/isometric fallback | agent-facing visual hints only*") {
    throw "D2D/isometric fallback is still presented as default 3D path."
}
Write-Host "PASS|d2d_isometric_not_default_3d_path"

if ($Dx12Header -notlike "*class AceAquariumDx12Viewport*" -or $Dx12Header -notlike "*Dx12Renderer*" -or $Dx12Cpp -notlike "*CreateWindowExW*") {
    throw "DX12 viewport class/open surface implementation missing."
}
Write-Host "PASS|dx12_viewport_class_exists"

if ($UiCpp -notlike "*aquariumDx12Viewport_.Open*" -or $UiCpp -notlike "*aquariumDx12Viewport_.RenderFrame*") {
    throw "DX12 viewport open/render command missing from UI."
}
Write-Host "PASS|dx12_viewport_open_command_exists"

foreach ($ref in @("AceAquariumDx12Viewport", "AceAquariumSceneAdapter", "AceAquariumViewport", "AceAquariumRenderPrimitive")) {
    if ($CMake -notlike "*$ref*") { throw "CMakeLists.txt missing $ref" }
    if ($Vcx -notlike "*$ref*") { throw ".vcxproj missing $ref" }
    if ($Filters -notlike "*$ref*") { throw ".vcxproj.filters missing $ref" }
}
Write-Host "PASS|project_references_exist"

if ($UiCpp -notlike "*DEBUG TRUTH - NOT AGENT INPUT*" -or $AdapterCpp -notlike "*DEBUG TRUTH - NOT AGENT INPUT*" -or $Dx12Cpp -notlike "*DEBUG TRUTH - NOT AGENT INPUT*") {
    throw "Debug truth warning missing from one of UI/adapter/DX12 viewport."
}

$ForbiddenSources = $UiCpp + "`n" + $AdapterCpp + "`n" + $Dx12Cpp
$Forbidden = @{
    "no_python_runtime" = "Python"
    "no_panda3d" = "Panda3D"
    "no_dearpygui" = "DearPyGui"
    "no_llm" = "LLM"
    "no_tokenizer" = "tokenizer"
}

foreach ($name in $Forbidden.Keys) {
    $needle = $Forbidden[$name]
    if ($ForbiddenSources -like "*$needle*") {
        throw "Forbidden runtime marker found for $name`: $needle"
    }
    Write-Host "PASS|$name"
}

$CoreFiles = Get-ChildItem (Join-Path $Root "Source\Private\Aquarium") -Filter "AceAq*.cpp" | Sort-Object Name | ForEach-Object { $_.FullName }
$UiFiles = Get-ChildItem (Join-Path $Root "Source\Private\AquariumUI") -Filter "AceAquarium*.cpp" | Sort-Object Name | ForEach-Object { $_.FullName }
# Headless probe intentionally excludes AceAquariumDx12Viewport.cpp. The build validates the real DX12 class.
$RenderFiles = Get-ChildItem (Join-Path $Root "Source\Private\AquariumRender") -Filter "AceAquarium*.cpp" |
    Where-Object { $_.Name -ne "AceAquariumDx12Viewport.cpp" } |
    Sort-Object Name |
    ForEach-Object { $_.FullName }

$Probe = Join-Path $Root "Tools\AceAq3D0RProbe.cpp"
$Exe = Join-Path $BuildDir "AceAq3D0RProbe.exe"

$cl = Get-Command cl.exe -ErrorAction SilentlyContinue
if ($cl) {
    Write-Host "INFO|compiler|cl.exe"
    & $cl.Source /std:c++20 /EHsc /W4 $FoArg /I (Join-Path $Root "Source\Public") /Fe:$Exe $Probe $CoreFiles $UiFiles $RenderFiles
    if ($LASTEXITCODE -ne 0) { throw "cl.exe failed with exit code $LASTEXITCODE" }
} else {
    $gpp = Get-Command g++.exe -ErrorAction SilentlyContinue
    if (-not $gpp) { $gpp = Get-Command g++ -ErrorAction SilentlyContinue }
    if (-not $gpp) { throw "No C++ compiler found. Run from Developer PowerShell with cl.exe or install g++." }

    Write-Host "INFO|compiler|g++"
    & $gpp.Source -std=c++20 -Wall -Wextra -pedantic -I (Join-Path $Root "Source\Public") -o $Exe $Probe $CoreFiles $UiFiles $RenderFiles
    if ($LASTEXITCODE -ne 0) { throw "g++ failed with exit code $LASTEXITCODE" }
}

Push-Location $Root
try {
    & $Exe
    if ($LASTEXITCODE -ne 0) { throw "AceAq3D0RProbe failed with exit code $LASTEXITCODE" }
}
finally {
    Pop-Location
}

Invoke-ExistingValidator "validate_ace_aqcpp1.ps1" "cpp1_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aqcpp2.ps1" "cpp2_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aqcpp3.ps1" "cpp3_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aqcpp4.ps1" "cpp4_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aqui0.ps1" "aqui0_probe_still_passes"

Write-Host "PASS|ace_aq3d0r_validation_complete"
