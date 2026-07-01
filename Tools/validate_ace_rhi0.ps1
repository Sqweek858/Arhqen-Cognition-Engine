$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $Root "Build\ACE-RHI0"
New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null

$Required = @(
  "Source\Public\ArhqenCognitionEngine\Renderer\RHI\AceRhi.h",
  "Source\Private\Renderer\RHI\AceRhi.cpp",
  "Source\Public\ArhqenCognitionEngine\Renderer\Scene\AceRenderScene.h",
  "Source\Private\Renderer\Scene\AceRenderScene.cpp",
  "Source\Public\ArhqenCognitionEngine\Renderer\Core\AceRenderer.h",
  "Source\Private\Renderer\Core\AceRenderer.cpp",
  "Tools\AceRhi0RendererCoreProbe.cpp",
  "Docs\ACE_RHI0.md"
)

foreach ($Rel in $Required) {
  if (-not (Test-Path (Join-Path $Root $Rel))) { throw "Missing ACE-RHI0 file: $Rel" }
}
Write-Host "PASS|rhi0_files_present"

$Rhi = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\Renderer\RHI\AceRhi.h") -Raw
$Scene = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\Renderer\Scene\AceRenderScene.h") -Raw
$Renderer = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\Renderer\Core\AceRenderer.h") -Raw

if ($Rhi -notlike "*class Registry*" -or $Rhi -notlike "*class CommandList*" -or $Rhi -notlike "*class RenderGraph*" -or $Rhi -notlike "*class IDevice*" -or $Rhi -notlike "*class NullDevice*") {
  throw "RHI foundation layer incomplete."
}
Write-Host "PASS|rhi_registry_command_graph_device_present"

if ($Scene -notlike "*class RenderScene*" -or $Scene -notlike "*class SceneRenderer*" -or $Scene -notlike "*MeshBatch*") {
  throw "Scene renderer layer incomplete."
}
Write-Host "PASS|scene_renderer_layer_present"

if ($Renderer -notlike "*class Renderer*" -or $Renderer -notlike "*render*" -or $Renderer -notlike "*FrameStats*") {
  throw "Renderer facade incomplete."
}
Write-Host "PASS|renderer_facade_present"

$Probe = Join-Path $Root "Tools\AceRhi0RendererCoreProbe.cpp"
$Sources = @(
  (Join-Path $Root "Source\Private\Renderer\RHI\AceRhi.cpp"),
  (Join-Path $Root "Source\Private\Renderer\Scene\AceRenderScene.cpp"),
  (Join-Path $Root "Source\Private\Renderer\Core\AceRenderer.cpp")
)
$Exe = Join-Path $BuildDir "AceRhi0RendererCoreProbe.exe"

$cl = Get-Command cl.exe -ErrorAction SilentlyContinue
if ($cl) {
  Write-Host "INFO|compiler|cl.exe"
  & $cl.Source /std:c++20 /EHsc /W4 /I (Join-Path $Root "Source\Public") /Fe:$Exe $Probe $Sources
  if ($LASTEXITCODE -ne 0) { throw "cl.exe failed with exit code $LASTEXITCODE" }
} else {
  $gpp = Get-Command g++.exe -ErrorAction SilentlyContinue
  if (-not $gpp) { $gpp = Get-Command g++ -ErrorAction SilentlyContinue }
  if (-not $gpp) { throw "No C++ compiler found. Run from Developer PowerShell with cl.exe or install g++." }

  Write-Host "INFO|compiler|g++"
  & $gpp.Source -std=c++20 -Wall -Wextra -pedantic -I (Join-Path $Root "Source\Public") -o $Exe $Probe $Sources
  if ($LASTEXITCODE -ne 0) { throw "g++ failed with exit code $LASTEXITCODE" }
}

Push-Location $Root
try {
  & $Exe
  if ($LASTEXITCODE -ne 0) { throw "AceRhi0RendererCoreProbe failed with exit code $LASTEXITCODE" }
}
finally {
  Pop-Location
}

Write-Host "PASS|ace_rhi0_validation_complete"
