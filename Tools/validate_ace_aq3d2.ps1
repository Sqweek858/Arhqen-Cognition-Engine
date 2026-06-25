$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $Root "Build\ACE-AQ3D2"
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
    "Source\Public\ArhqenCognitionEngine\AquariumUI\AceEnvironmentWorkspace.h",
    "Source\Private\AquariumUI\AceEnvironmentWorkspace.cpp",
    "Tools\AceAq3D2Probe.cpp",
    "Docs\ACE_AQ3D2.md"
)

foreach ($rel in $Required) {
    if (-not (Test-Path (Join-Path $Root $rel))) {
        throw "Missing required AQ3D2 file: $rel"
    }
}

$UiHeader = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\Ui\AceShellUi.h") -Raw
$UiCpp = Get-Content (Join-Path $Root "Source\Private\Ui\AceShellUi.cpp") -Raw
$EmbeddedHeader = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\AquariumRender\AceAquariumEmbeddedDx12Viewport.h") -Raw
$EmbeddedCpp = Get-Content (Join-Path $Root "Source\Private\AquariumRender\AceAquariumEmbeddedDx12Viewport.cpp") -Raw
$WorkspaceHeader = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\AquariumUI\AceEnvironmentWorkspace.h") -Raw
$WorkspaceCpp = Get-Content (Join-Path $Root "Source\Private\AquariumUI\AceEnvironmentWorkspace.cpp") -Raw
$AdapterCpp = Get-Content (Join-Path $Root "Source\Private\AquariumRender\AceAquariumSceneAdapter.cpp") -Raw
$CMake = Get-Content (Join-Path $Root "CMakeLists.txt") -Raw
$Vcx = Get-Content (Join-Path $Root "Source\ArhqenCognitionEngine.vcxproj") -Raw
$Filters = Get-Content (Join-Path $Root "Source\ArhqenCognitionEngine.vcxproj.filters") -Raw

if ($UiCpp -notlike "*Environment Workspace*" -or $UiCpp -notlike "*aquariumEmbeddedViewportVisible_ = true*" -or $UiCpp -notlike "*ACE-AQ3D2: full Environment workspace*") {
    throw "Environment does not open as a full workspace."
}
Write-Host "PASS|environment_opens_full_workspace"

if ($UiCpp -like "*Cognitive Environment Control Panel*" -or $UiCpp -like "*centered dashboard*" -or $UiCpp -like "*viewportCardH*") {
    throw "Environment still appears to use the old small modal/card 3D layout."
}
Write-Host "PASS|environment_no_longer_uses_small_modal_for_3d"

if ($EmbeddedHeader -notlike "*class AceAquariumEmbeddedDx12Viewport*" -or $EmbeddedHeader -notlike "*Dx12Renderer*" -or $EmbeddedCpp -notlike "*WS_CHILD*" -or $EmbeddedCpp -notlike "*CreateWindowExW*") {
    throw "Embedded DX12 viewport class missing child-surface implementation."
}
Write-Host "PASS|embedded_dx12_viewport_class_exists"

if ($EmbeddedCpp -like "*WS_OVERLAPPED*" -or $EmbeddedCpp -like "*WS_POPUP*" -or $UiCpp -like "*Arhqen Cognition Engine - DX12 3D Environment*" -or (Test-Path (Join-Path $Root "Source\Private\AquariumRender\AceAquariumDx12Viewport.cpp"))) {
    throw "Separate DX12 window path still exists."
}
Write-Host "PASS|dx12_viewport_is_embedded_not_separate_window"

if ($WorkspaceHeader -notlike "*AceEnvironmentWorkspace*" -or $WorkspaceCpp -notlike "*ViewportIsLargeEnough*" -or $WorkspaceCpp -notlike "*0.60f*" -or $WorkspaceCpp -notlike "*0.55f*") {
    throw "Workspace layout sizing/validation is missing."
}
Write-Host "PASS|viewport_rect_large_enough"

if ($WorkspaceCpp -notlike "*Compute(float clientWidth, float clientHeight*" -or $EmbeddedCpp -notlike "*MoveWindow(hwnd_, x_, y_, width_, height_, TRUE)*") {
    throw "Viewport rect does not update on resize/layout."
}
Write-Host "PASS|viewport_rect_updates_on_resize"

if ($EmbeddedHeader -notlike "*InitCount()*" -or $EmbeddedHeader -notlike "*ResizeCount()*" -or $EmbeddedHeader -notlike "*FrameCount()*" -or $EmbeddedCpp -notlike "*++initCount_*") {
    throw "Viewport resource counters/persistence markers missing."
}
Write-Host "PASS|viewport_resources_persistent"

if ($EmbeddedCpp -notlike "*sizeChanged*" -or $EmbeddedCpp -notlike "*++resizeCount_*" -or $EmbeddedCpp -notlike "*Resize resources only when*") {
    throw "Viewport resize is not guarded by actual size changes."
}
Write-Host "PASS|viewport_resize_only_on_size_change"

$RenderFrameStart = $EmbeddedCpp.IndexOf("bool AceAquariumEmbeddedDx12Viewport::RenderFrame")
$RenderFrameText = $EmbeddedCpp.Substring($RenderFrameStart)
$RenderFrameText = $RenderFrameText.Substring(0, [Math]::Min(1200, $RenderFrameText.Length))
if ($RenderFrameText -like "*RecreateRenderer*" -or $EmbeddedCpp -notlike "*Do not MoveWindow every frame*") {
    throw "Viewport resources may be recreated every frame."
}
Write-Host "PASS|viewport_not_recreated_every_frame"

$AllSource = $UiCpp + "`n" + $EmbeddedCpp + "`n" + $WorkspaceCpp
if ($AllSource -like "*Embedded Embedded*") {
    throw "Bad duplicated Embedded text still exists."
}
Write-Host "PASS|no_embedded_embedded_error_text"

if ($AllSource -like "*separate DX12 surface*") {
    throw "Wrong separate DX12 surface text still exists."
}
Write-Host "PASS|no_separate_dx12_surface_text"

if ($UiCpp -like "*D2D/isometric fallback | agent-facing visual hints only*" -or $UiCpp -like "*Minimal 3D/isometric Aquarium viewport is active*") {
    throw "D2D/isometric fallback is still presented as 3D."
}
Write-Host "PASS|d2d_isometric_not_default_3d_path"

if ($UiCpp -notlike "*DEBUG TRUTH - NOT AGENT INPUT*" -or $AdapterCpp -notlike "*DEBUG TRUTH - NOT AGENT INPUT*" -or $EmbeddedCpp -notlike "*debugTruthEnabled ? color*") {
    throw "Debug truth warning/state missing from UI/adapter/embedded viewport."
}

foreach ($ref in @("AceAquariumEmbeddedDx12Viewport", "AceEnvironmentWorkspace", "AceAquariumSceneAdapter", "AceAquariumViewport", "AceAquariumRenderPrimitive")) {
    if ($CMake -notlike "*$ref*") { throw "CMakeLists.txt missing $ref" }
    if ($Vcx -notlike "*$ref*") { throw ".vcxproj missing $ref" }
    if ($Filters -notlike "*$ref*") { throw ".vcxproj.filters missing $ref" }
}
Write-Host "PASS|project_references_exist"

$ForbiddenSources = $UiCpp + "`n" + $AdapterCpp + "`n" + $EmbeddedCpp + "`n" + $WorkspaceCpp
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

$RootObj = Get-ChildItem $Root -File -ErrorAction SilentlyContinue | Where-Object { $_.Extension -eq ".obj" }
$RootExe = Get-ChildItem $Root -File -ErrorAction SilentlyContinue | Where-Object { $_.Extension -eq ".exe" }
$RootPdb = Get-ChildItem $Root -File -ErrorAction SilentlyContinue | Where-Object { $_.Extension -eq ".pdb" }

if ($RootObj.Count -gt 0) { throw "Root .obj files found: $($RootObj.Name -join ', ')" }
Write-Host "PASS|no_obj_files_in_repo_root"

if ($RootExe.Count -gt 0) { throw "Root .exe files found: $($RootExe.Name -join ', ')" }
Write-Host "PASS|no_exe_files_in_repo_root"

if ($RootPdb.Count -gt 0) { throw "Root .pdb files found: $($RootPdb.Name -join ', ')" }
Write-Host "PASS|no_pdb_files_in_repo_root"

$CoreFiles = Get-ChildItem (Join-Path $Root "Source\Private\Aquarium") -Filter "AceAq*.cpp" | Sort-Object Name | ForEach-Object { $_.FullName }
$UiFiles = Get-ChildItem (Join-Path $Root "Source\Private\AquariumUI") -Filter "Ace*.cpp" | Sort-Object Name | ForEach-Object { $_.FullName }
# Headless probe intentionally excludes the Win32/DX12 embedded child surface. Static checks and full build cover that class.
$RenderFiles = Get-ChildItem (Join-Path $Root "Source\Private\AquariumRender") -Filter "AceAquarium*.cpp" |
    Where-Object { $_.Name -ne "AceAquariumEmbeddedDx12Viewport.cpp" } |
    Where-Object { $_.Name -ne "AceAquariumDx12Viewport.cpp" } |
    Sort-Object Name |
    ForEach-Object { $_.FullName }

$Probe = Join-Path $Root "Tools\AceAq3D2Probe.cpp"
$Exe = Join-Path $BuildDir "AceAq3D2Probe.exe"

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
    if ($LASTEXITCODE -ne 0) { throw "AceAq3D2Probe failed with exit code $LASTEXITCODE" }
}
finally {
    Pop-Location
}

Invoke-ExistingValidator "validate_ace_aqcpp1.ps1" "cpp1_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aqcpp2.ps1" "cpp2_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aqcpp3.ps1" "cpp3_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aqcpp4.ps1" "cpp4_probe_still_passes"
Invoke-ExistingValidator "validate_ace_aqui0.ps1" "aqui0_probe_still_passes"

Write-Host "PASS|ace_aq3d2_validation_complete"
