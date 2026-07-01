param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
)

$ErrorActionPreference = "Stop"

$Probe = Join-Path $Root "Tools\AcePerf2R2StatConsoleDetailProbe.cpp"
$Shell = Join-Path $Root "Source\Private\Ui\AceShellUi.cpp"
$Console = Join-Path $Root "Source\Public\ArhqenCognitionEngine\Ui\AceEngineConsole.h"
$Docs = Join-Path $Root "Docs\ACE_PERF2R2_STAT_CONSOLE_DETAIL_RESTORE.md"

foreach ($Path in @($Probe, $Shell, $Console, $Docs)) {
    if (!(Test-Path $Path)) { throw "Missing required PERF2R2 file: $Path" }
}

$ShellText = Get-Content $Shell -Raw
$ConsoleText = Get-Content $Console -Raw
$DocsText = Get-Content $Docs -Raw

function Assert-Contains([string]$Name, [string]$Text, [string]$Needle) {
    if ($Text -notlike "*$Needle*") { throw "FAIL|$Name|missing $Needle" }
    Write-Output "PASS|$Name"
}

Assert-Contains "perf2r2_stat_rhi_multiline" $ShellText "[STAT_RHI]\n"
Assert-Contains "perf2r2_stat_fps_multiline" $ShellText "[STAT_FPS]\n"
Assert-Contains "perf2r2_readback_visible" $ShellText "readback_active:"
Assert-Contains "perf2r2_d2d_quality_visible" $ShellText "ui_layer: D2D_RETAINED_OVERLAY"
Assert-Contains "perf2r2_detail_log_helper" $ConsoleText "AceEngineAppendLogLines"
Assert-Contains "perf2r2_detail_log_call" $ShellText 'AceEngineAppendLogLines(logTag + "_DETAIL"'
Assert-Contains "perf2r2_wrapping" $ShellText "aceWrapEngineConsoleLine"
Assert-Contains "perf2r2_docs" $DocsText "ACE-PERF2R2"

if ($ShellText -like "*stat_frame*") { throw "FAIL|perf2r2_no_new_stat_frame|stat_frame should not exist" }
Write-Output "PASS|perf2r2_no_new_stat_frame"
Write-Output "PASS|ace_perf2r2_stat_console_detail_validation"
