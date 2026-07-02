param([string]$Root=(Resolve-Path (Join-Path $PSScriptRoot "..")).Path)
$ErrorActionPreference="Stop";$B=Join-Path $Root "Build\ACE-PANEL-RESIZE";$O=Join-Path $B "obj";New-Item -ItemType Directory -Force $O|Out-Null;$C=Get-Command cl.exe -ErrorAction SilentlyContinue;if(!$C){throw "cl.exe not found"}
$Shell=Get-Content (Join-Path $Root "Source\Private\Ui\AceShellUi.cpp") -Raw
if($Shell -like "*renderAquariumResizeHandle*"){throw "Visible corner resize handle renderer still exists"}
if($Shell -notlike "*PanelResizePolicy::hitTest*" -or $Shell -notlike "*PanelResizePolicy::cursor*"){throw "Shell is not integrated with reusable edge resize policy"}
Write-Output "PASS|shell_uses_invisible_edge_resize_policy"
$S=@((Join-Path $Root "Tools\AcePanelResizeProbe.cpp"),(Join-Path $Root "Source\Private\Ui\Core\AcePanelResizePolicy.cpp"),(Join-Path $Root "Source\Private\AquariumUI\AceEnvironment3DMode.cpp"),(Join-Path $Root "Source\Private\Ui\D2D\D2DLayoutPersistence.cpp"));& $C.Source /nologo /std:c++20 /W4 /WX /EHsc /permissive- /I"$(Join-Path $Root 'Source\Public')" /Fo"$O\" $S /link /out:"$(Join-Path $B 'AcePanelResizeProbe.exe')";if($LASTEXITCODE){throw "Panel resize probe compile failed"};& (Join-Path $B "AcePanelResizeProbe.exe");if($LASTEXITCODE){throw "Panel resize probe failed"};Write-Output "PASS|ace_panel_resize_validation"
