param([string]$Root=(Resolve-Path (Join-Path $PSScriptRoot "..")).Path)
$ErrorActionPreference="Stop";$B=Join-Path $Root "Build\ACE-COMMANDS";$O=Join-Path $B "obj";New-Item -ItemType Directory -Force $O|Out-Null;$C=Get-Command cl.exe -ErrorAction SilentlyContinue;if(!$C){throw "cl.exe not found"}
& $C.Source /nologo /std:c++20 /W4 /WX /EHsc /permissive- /I"$(Join-Path $Root 'Source\Public')" /Fo"$O\" (Join-Path $Root "Tools\AceCommandRegistryProbe.cpp") (Join-Path $Root "Source\Private\Editor\Commands\AceCommandRegistry.cpp") /link /out:"$(Join-Path $B 'AceCommandRegistryProbe.exe')"
if($LASTEXITCODE){throw "Command probe compile failed"};& (Join-Path $B "AceCommandRegistryProbe.exe");if($LASTEXITCODE){throw "Command probe failed"};Write-Output "PASS|ace_commands_validation"
