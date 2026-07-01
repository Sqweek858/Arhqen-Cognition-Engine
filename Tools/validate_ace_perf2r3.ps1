param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
)

$ErrorActionPreference = "Stop"

$Probe = Join-Path $Root "Tools\AcePerf2R3LogTextSelectionProbe.cpp"
$ShellHeader = Join-Path $Root "Source\Public\ArhqenCognitionEngine\Ui\AceShellUi.h"
$Shell = Join-Path $Root "Source\Private\Ui\AceShellUi.cpp"
$Docs = Join-Path $Root "Docs\ACE_PERF2R3_LOG_TEXT_SELECTION.md"

foreach ($Path in @($Probe, $ShellHeader, $Shell, $Docs)) {
    if (!(Test-Path $Path)) { throw "Missing required PERF2R3 file: $Path" }
}

$HeaderText = Get-Content $ShellHeader -Raw
$ShellText = Get-Content $Shell -Raw
$DocsText = Get-Content $Docs -Raw

function Assert-Contains([string]$Name, [string]$Text, [string]$Needle) {
    if ($Text -notlike "*$Needle*") { throw "FAIL|$Name|missing $Needle" }
    Write-Output "PASS|$Name"
}

Assert-Contains "perf2r3_selection_state_fields" $HeaderText "engineLogSelectionAnchor_"
Assert-Contains "perf2r3_hit_test" $ShellText "AceShellUi::hitTestEngineLogText"
Assert-Contains "perf2r3_multiline_copy" $ShellText 'out << L"\r\n"'
Assert-Contains "perf2r3_clipboard_copy" $ShellText "D2DClipboard::writeText"
Assert-Contains "perf2r3_ctrl_c" $ShellText "engineLogHasTextSelection() && copyEngineLogTextSelectionToClipboard()"
Assert-Contains "perf2r3_ctrl_a" $ShellText "selectAllEngineLogText()"
Assert-Contains "perf2r3_drag_selection" $ShellText "engineLogTextSelecting_ = true"
Assert-Contains "perf2r3_single_selection_tint_marker" $ShellText "PERF2R3.2"
Assert-Contains "perf2r3_single_selection_tint_opacity" $ShellText "SetOpacity(0.36f)"
if ($ShellText -like "*rowBand*") { throw "FAIL|perf2r3_no_row_band_duplicate|unexpected rowBand selection layer" }
if ($ShellText -like "*SetOpacity(0.16f)*" -or $ShellText -like "*SetOpacity(0.48f)*") { throw "FAIL|perf2r3_no_double_blue_selection|old double selection opacity still present" }
Write-Output "PASS|perf2r3_no_double_blue_selection"
Assert-Contains "perf2r3_ibeam_cursor" $ShellText "engineLogOverlayLogViewportRect_.contains(x, y)"
Assert-Contains "perf2r3_no_char_leak" $ShellText "return engineLogTextFocused_;"
Assert-Contains "perf2r3_docs" $DocsText "ACE-PERF2R3"
Assert-Contains "perf2r3_visual_polish_docs" $DocsText "PERF2R3.2"

if ($ShellText -like "*submitAndPresentBgra8ToComposition*") { throw "FAIL|perf2r3_no_rhi_path_change|unexpected GPU composition marker in AceShellUi.cpp" }
Write-Output "PASS|perf2r3_no_rhi_path_change"
Write-Output "PASS|ace_perf2r3_log_text_selection_validation"
