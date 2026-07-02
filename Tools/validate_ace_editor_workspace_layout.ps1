param([string]$Root=(Resolve-Path (Join-Path $PSScriptRoot "..")).Path)
$ErrorActionPreference = "Stop"
$Build = Join-Path $Root "Build\ACE-EDITOR-WORKSPACE-PROBE"
$Objects = Join-Path $Build "obj"
New-Item -ItemType Directory -Force $Objects | Out-Null
$Compiler = Get-Command cl.exe -ErrorAction SilentlyContinue
if (-not $Compiler) { throw "cl.exe not found" }
$Sources = @(
    (Join-Path $Root "Tools\AceEditorWorkspaceLayoutProbe.cpp"),
    (Join-Path $Root "Source\Private\Editor\Workspace\AceEditorWorkspaceLayout.cpp"),
    (Join-Path $Root "Source\Private\Core\Identity\AceGuid.cpp"),
    (Join-Path $Root "Source\Private\Core\Serialization\AceArchive.cpp"),
    (Join-Path $Root "Source\Private\Core\Serialization\AceAtomicFile.cpp")
)
& $Compiler.Source /nologo /std:c++20 /W4 /WX /EHsc /permissive- /DUNICODE /D_UNICODE /DNOMINMAX /DWIN32_LEAN_AND_MEAN /I"$(Join-Path $Root 'Source\Public')" /Fo"$Objects\" $Sources /link /out:"$(Join-Path $Build 'AceEditorWorkspaceLayoutProbe.exe')"
if ($LASTEXITCODE) { throw "Editor workspace probe compile failed: $LASTEXITCODE" }
& (Join-Path $Build "AceEditorWorkspaceLayoutProbe.exe")
if ($LASTEXITCODE) { throw "Editor workspace probe failed: $LASTEXITCODE" }
$Project = Get-Content (Join-Path $Root "Source\ArhqenCognitionEngine.vcxproj") -Raw
$CMake = Get-Content (Join-Path $Root "CMakeLists.txt") -Raw
if ($Project -notlike "*AceEditorWorkspaceLayout.cpp*" -or $CMake -notlike "*AceEditorWorkspaceLayout.cpp*") {
    throw "Editor workspace layout is missing from a build system"
}
Write-Output "PASS|ace_editor_workspace_layout_validation"
