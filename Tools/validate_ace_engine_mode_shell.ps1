param([string]$Root=(Resolve-Path (Join-Path $PSScriptRoot "..")).Path)
$ErrorActionPreference = "Stop"
$Build = Join-Path $Root "Build\ACE-ENGINE-MODE-SHELL"
New-Item -ItemType Directory -Force $Build | Out-Null
$Compiler = Get-Command cl.exe -ErrorAction SilentlyContinue
if (-not $Compiler) { throw "cl.exe not found" }
$Probe = Join-Path $Root "Tools\AceEngineModeShellProbe.cpp"
$Executable = Join-Path $Build "AceEngineModeShellProbe.exe"
& $Compiler.Source /nologo /std:c++20 /W4 /WX /EHsc /permissive- /Fo"$Build\" /Fe:$Executable $Probe
if ($LASTEXITCODE) { throw "Engine mode shell probe compile failed: $LASTEXITCODE" }
Push-Location $Root
try { & $Executable } finally { Pop-Location }
if ($LASTEXITCODE) { throw "Engine mode shell probe failed: $LASTEXITCODE" }
Write-Output "PASS|ace_engine_mode_shell_validation"
