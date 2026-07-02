param([string]$Root=(Resolve-Path (Join-Path $PSScriptRoot "..")).Path)
$ErrorActionPreference="Stop"; $Build=Join-Path $Root "Build\ACE-TRANSACTIONS"; $Obj=Join-Path $Build "obj"; New-Item -ItemType Directory -Force $Obj|Out-Null
$Compiler=Get-Command cl.exe -ErrorAction SilentlyContinue; if(!$Compiler){throw "cl.exe not found"}
& $Compiler.Source /nologo /std:c++20 /W4 /WX /EHsc /permissive- /I"$(Join-Path $Root 'Source\Public')" /Fo"$Obj\" (Join-Path $Root "Tools\AceTransactionProbe.cpp") (Join-Path $Root "Source\Private\Editor\Transactions\AceTransaction.cpp") /link /out:"$(Join-Path $Build 'AceTransactionProbe.exe')"
if($LASTEXITCODE -ne 0){throw "Transaction probe compile failed: $LASTEXITCODE"}; & (Join-Path $Build "AceTransactionProbe.exe"); if($LASTEXITCODE -ne 0){throw "Transaction probe failed: $LASTEXITCODE"}
Write-Output "PASS|ace_transaction_validation"
