param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
)

$ErrorActionPreference = "Stop"

$Probe = Join-Path $Root "Tools\AcePerf3ViewportReadbackCacheProbe.cpp"
$Console = Join-Path $Root "Source\Public\ArhqenCognitionEngine\Ui\AceEngineConsole.h"
$ShellHeader = Join-Path $Root "Source\Public\ArhqenCognitionEngine\Ui\AceShellUi.h"
$Shell = Join-Path $Root "Source\Private\Ui\AceShellUi.cpp"
$Docs = Join-Path $Root "Docs\ACE_PERF3_VIEWPORT_READBACK_CACHE.md"

foreach ($Path in @($Probe, $Console, $ShellHeader, $Shell, $Docs)) {
    if (!(Test-Path $Path)) { throw "Missing required PERF3 file: $Path" }
}

$ConsoleText = Get-Content $Console -Raw
$HeaderText = Get-Content $ShellHeader -Raw
$ShellText = Get-Content $Shell -Raw
$DocsText = Get-Content $Docs -Raw

function Assert-Contains([string]$Name, [string]$Text, [string]$Needle) {
    if ($Text -notlike "*$Needle*") { throw "FAIL|$Name|missing $Needle" }
    Write-Output "PASS|$Name"
}

Assert-Contains "perf3_cached_render_path_enum" $ConsoleText "Dx12CachedReadback"
Assert-Contains "perf3_cached_render_path_string" $ConsoleText "DX12_CACHED_READBACK"
Assert-Contains "perf3_cache_key_state" $HeaderText "AquariumViewportCacheKey"
Assert-Contains "perf3_cache_key_valid_state" $HeaderText "aquariumViewportCacheKeyValid_"
Assert-Contains "perf3_cached_draw_path" $ShellText "drawCachedAquariumSlateViewportElement"
Assert-Contains "perf3_cache_sets_path" $ShellText "AceEngineRenderPath::Dx12CachedReadback"
Assert-Contains "perf3_cache_skips_rhi" $ShellText "SetLastRhiRenderMs(0.0)"
Assert-Contains "perf3_cache_updates_after_fresh_frame" $ShellText "aquariumViewportCacheKey_ = key"
Assert-Contains "perf3_cache_stats_hits" $ShellText "viewport_cache_hits"
Assert-Contains "perf3_cache_stats_misses" $ShellText "viewport_cache_misses"
Assert-Contains "perf3_docs" $DocsText "ACE-PERF3"
Assert-Contains "perf3_docs_path" $DocsText "DX12_CACHED_READBACK"

Write-Output "PASS|ace_perf3_viewport_readback_cache_validation"
