$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $Root "Build\ACE-AQ3D1"
New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
$ObjDir = Join-Path $BuildDir "obj"
New-Item -ItemType Directory -Force -Path $ObjDir | Out-Null
$FoArg = "/Fo$ObjDir\"

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
    "Source\Public\ArhqenCognitionEngine\AquariumRender\AceAquariumEmbeddedDx12Viewport.h",
    "Source\Private\AquariumRender\AceAquariumEmbeddedDx12Viewport.cpp",
    "Tools\AceAq3D1Probe.cpp",
    "Docs\ACE_AQ3D1.md"
)

foreach ($rel in $Required) {
    if (-not (Test-Path (Join-Path $Root $rel))) {
        throw "Missing required AQ3D1 file: $rel"
    }
}

$UiHeader = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\Ui\AceShellUi.h") -Raw
$UiCpp = Get-Content (Join-Path $Root "Source\Private\Ui\AceShellUi.cpp") -Raw
$EmbeddedHeader = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\AquariumRender\AceAquariumEmbeddedDx12Viewport.h") -Raw
$EmbeddedCpp = Get-Content (Join-Path $Root "Source\Private\AquariumRender\AceAquariumEmbeddedDx12Viewport.cpp") -Raw
$AdapterCpp = Get-Content (Join-Path $Root "Source\Private\AquariumRender\AceAquariumSceneAdapter.cpp") -Raw
$CMake = Get-Content (Join-Path $Root "CMakeLists.txt") -Raw
$Vcx = Get-Content (Join-Path $Root "Source\ArhqenCognitionEngine.vcxproj") -Raw
$Filters = Get-Content (Join-Path $Root "Source\ArhqenCognitionEngine.vcxproj.filters") -Raw

if ($EmbeddedHeader -notlike "*class AceAquariumEmbeddedDx12Viewport*" -or $EmbeddedHeader -notlike "*Dx12Renderer*" -or $EmbeddedCpp -notlike "*WS_CHILD*" -or $EmbeddedCpp -notlike "*CreateWindowExW*") {
    throw "Embedded DX12 viewport class missing child-surface implementation."
}
Write-Host "PASS|embedded_dx12_viewport_class_exists"

if ($UiCpp -like "*Arhqen Cognition Engine - DX12 3D Environment*" -or $EmbeddedCpp -like "*WS_OVERLAPPED*" -or $EmbeddedCpp -like "*WS_POPUP*" -or (Test-Path (Join-Path $Root "Source\Private\AquariumRender\AceAquariumDx12Viewport.cpp"))) {
    throw "Separate DX12 window path is still present as main path."
}
Write-Host "PASS|separate_dx12_window_not_main_path"

if ($UiCpp -notlike "*DX12 3D Environment*" -or $UiCpp -notlike "*aquariumEmbeddedViewportRect_*" -or $UiCpp -notlike "*renderAquariumViewportCard*") {
    throw "Environment panel embedded viewport region missing."
}
Write-Host "PASS|environment_panel_has_embedded_viewport_region"

if ($UiCpp -notlike "*Open 3D Environment*" -or $UiCpp -notlike "*aquariumEmbeddedViewportVisible_ = true*" -or $UiCpp -notlike "*aquariumContentScroll_.offset = 0.0f*") {
    throw "Open 3D Environment does not show/focus embedded viewport."
}
Write-Host "PASS|open_3d_environment_shows_embedded_viewport"

if ($UiCpp -notlike "*std::round(left)*" -or $UiCpp -notlike "*std::round(right - left)*" -or $EmbeddedCpp -notlike "*MoveWindow(hwnd_, x_, y_, width_, height_, TRUE)*") {
    throw "Viewport rect does not update from panel bounds."
}
Write-Host "PASS|viewport_rect_updates_on_panel_resize"

if ($UiCpp -like "*D2D/isometric fallback | agent-facing visual hints only*" -or $UiCpp -like "*Minimal 3D/isometric Aquarium viewport is active*") {
    throw "D2D/isometric fallback is still presented as 3D."
}
Write-Host "PASS|d2d_isometric_not_default_3d_path"

foreach ($ref in @("AceAquariumEmbeddedDx12Viewport", "AceAquariumSceneAdapter", "AceAquariumViewport", "AceAquariumRenderPrimitive")) {
    if ($CMake -notlike "*$ref*") { throw "CMakeLists.txt missing $ref" }
    if ($Vcx -notlike "*$ref*") { throw ".vcxproj missing $ref" }
    if ($Filters -notlike "*$ref*") { throw ".vcxproj.filters missing $ref" }
}
Write-Host "PASS|project_references_exist"

if ($UiCpp -notlike "*DEBUG TRUTH - NOT AGENT INPUT*" -or $AdapterCpp -notlike "*DEBUG TRUTH - NOT AGENT INPUT*" -or $EmbeddedCpp -notlike "*debugTruthEnabled ? color*") {
    throw "Debug truth warning/state missing from UI/adapter/embedded viewport."
}

$ForbiddenSources = $UiCpp + "`n" + $AdapterCpp + "`n" + $EmbeddedCpp
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

$RootTrash = Get-ChildItem $Root -File -ErrorAction SilentlyContinue | Where-Object { $_.Extension -in @(".obj", ".exe", ".pdb", ".ilk", ".log") }
if ($RootTrash.Count -gt 0) {
    throw "Build artifacts found in repo root: $($RootTrash.Name -join ', ')"
}
Write-Host "PASS|no_obj_files_in_repo_root"

$CoreFiles = Get-ChildItem (Join-Path $Root "Source\Private\Aquarium") -Filter "AceAq*.cpp" | Sort-Object Name | ForEach-Object { $_.FullName }
$UiFiles = Get-ChildItem (Join-Path $Root "Source\Private\AquariumUI") -Filter "AceAquarium*.cpp" | Sort-Object Name | ForEach-Object { $_.FullName }
# Headless probe intentionally excludes the Win32/DX12 embedded child surface. Static checks and full build cover that class.
$RenderFiles = Get-ChildItem (Join-Path $Root "Source\Private\AquariumRender") -Filter "AceAquarium*.cpp" |
    Where-Object { $_.Name -ne "AceAquariumEmbeddedDx12Viewport.cpp" } |
    Where-Object { $_.Name -ne "AceAquariumDx12Viewport.cpp" } |
    Sort-Object Name |
    ForEach-Object { $_.FullName }

$Probe = Join-Path $Root "Tools\AceAq3D1Probe.cpp"
$Exe = Join-Path $BuildDir "AceAq3D1Probe.exe"

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
    if ($LASTEXITCODE -ne 0) { throw "AceAq3D1Probe failed with exit code $LASTEXITCODE" }
}
finally {
    Pop-Location
}

Invoke-ExistingValidator "validate_ace_aqcpp1.ps1" "cpp1_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aqcpp2.ps1" "cpp2_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aqcpp3.ps1" "cpp3_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aqcpp4.ps1" "cpp4_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aqui0.ps1" "aqui0_probe_still_passes"

Write-Host "PASS|ace_aq3d1_validation_complete"
