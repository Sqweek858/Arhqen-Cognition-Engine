$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $Root "Build\ACE-PERF0"
New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null

$Required = @(
    "Source\Public\ArhqenCognitionEngine\Ui\AceEngineConsole.h",
    "Source\Public\ArhqenCognitionEngine\Ui\AceShellUi.h",
    "Source\Private\Ui\AceShellUi.cpp",
    "Source\Public\ArhqenCognitionEngine\Renderer\Scene\AceAquariumGpuViewportRenderer.h",
    "Source\Private\Renderer\Scene\AceAquariumGpuViewportRenderer.cpp",
    "Tools\AcePerf0StatsConsoleProbe.cpp",
    "Docs\ACE_PERF0_STATS_CONSOLE.md",
    "Docs\ACE_AQ3D15_VIEWPORT_LAYERING.md",
    "Docs\ACE_UI12_VIEWPORT_OVERLAY_POLISH.md",
    "Tools\AceUi12ViewportOverlayProbe.cpp",
    "Tools\validate_ace_ui12.ps1"
)

foreach ($Rel in $Required) {
    if (-not (Test-Path (Join-Path $Root $Rel))) {
        throw "Missing ACE-PERF0 file: $Rel"
    }
}
Write-Host "PASS|perf0_files_present"

$Shell = Get-Content (Join-Path $Root "Source\Private\Ui\AceShellUi.cpp") -Raw
$ShellH = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\Ui\AceShellUi.h") -Raw
$ConsoleH = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\Ui\AceEngineConsole.h") -Raw
$GpuH = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\Renderer\Scene\AceAquariumGpuViewportRenderer.h") -Raw
$GpuCpp = Get-Content (Join-Path $Root "Source\Private\Renderer\Scene\AceAquariumGpuViewportRenderer.cpp") -Raw
$Docs = Get-Content (Join-Path $Root "Docs\ACE_PERF0_STATS_CONSOLE.md") -Raw
$LayeringDocs = Get-Content (Join-Path $Root "Docs\ACE_AQ3D15_VIEWPORT_LAYERING.md") -Raw
$Dx12H = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\Renderer\RHI\AceDx12Rhi.h") -Raw
$Dx12Cpp = Get-Content (Join-Path $Root "Source\Private\Renderer\RHI\AceDx12Rhi.cpp") -Raw

function Assert-Contains($Name, $Haystack, $Needle) {
    if ($Haystack -notlike "*$Needle*") { throw "$Name missing marker: $Needle" }
    Write-Host "PASS|$Name"
}

Assert-Contains "perf0_stat_coords_recognized" $Shell "id == L`"stat_coords`""
Assert-Contains "perf0_stat_rhi_recognized" $Shell "id == L`"stat_rhi`""
Assert-Contains "perf0_stat_fps_recognized" $Shell "id == L`"stat_fps`""
Assert-Contains "perf0_slash_commands_supported" $Shell "value.front() == L'/'"
Assert-Contains "perf0_unknown_command_warning_no_backend" $Shell "Not submitted to backend"
Assert-Contains "perf0_rename_marker_preserved" $Shell "const std::wstring prefix = L`"/rename `""
Assert-Contains "perf0_palette_stat_coords" $Shell "{L`"stat_coords`""
Assert-Contains "perf0_palette_stat_rhi" $Shell "{L`"stat_rhi`""
Assert-Contains "perf0_palette_stat_fps" $Shell "{L`"stat_fps`""
Assert-Contains "perf0_log_helper_exists" $ConsoleH "AceEngineAppendLog"
Assert-Contains "perf0_log_overlay_docked_to_viewport" $Shell "anchor = aquariumEmbeddedViewportRect_"
Assert-Contains "perf0_log_overlay_input_keyboard" $Shell "submitEngineLogOverlayInput"
Assert-Contains "perf0_log_overlay_scrollbar" $Shell "renderAquariumScrollbar(ctx, engineLogOverlayScroll_)"
Assert-Contains "perf0_log_overlay_unknown_no_backend" $Shell "source=engine_log_overlay"
Assert-Contains "perf0_ue_style_viewport_layering" $Shell "shouldUseDirectCompositionForAquariumViewport"
Assert-Contains "perf0_parent_composited_viewport" $Shell "parent-composited viewport"
Assert-Contains "perf0_cached_viewport_bitmap" $ShellH "aquariumSlateViewportBitmap_"
Assert-Contains "perf0_cached_bitmap_copy" $Shell "CopyFromMemory"
Assert-Contains "perf0_reset_dcomp_shell" $Shell "resetAquariumDirectCompositionIfActive"
Assert-Contains "perf0_reset_dcomp_renderer" $GpuH "resetCompositionHost"
Assert-Contains "perf0_reset_dcomp_rhi" $Dx12H "resetCompositionHost"
Assert-Contains "perf0_reset_dcomp_impl" $Dx12Cpp "impl_->resetComposition()"
Assert-Contains "perf0_layering_docs" $LayeringDocs "UE-style Viewport Layering"
if ($Shell -like "*DX12 child viewport clipped by engine log UI*" -or $Shell -like "*overlay.top - 8.0f*") { throw "Old child-HWND clipping workaround is still present" }
Write-Host "PASS|perf0_child_hwnd_clip_removed"
Assert-Contains "perf0_ui12_deferred_dcomp_reset" $Shell "defer DirectComposition teardown"
Assert-Contains "perf0_ui12_dcomp_after_paint_flag" $ShellH "aquariumResetDirectCompositionAfterPaint_"
Assert-Contains "perf0_ui12_parent_hold" $ShellH "aquariumParentCompositedHoldFrames_"
Assert-Contains "perf0_ui12_parent_hold_cpp" $Shell "requestParentCompositedViewportHold"
Assert-Contains "perf0_ui12_telemetry_layer" $Shell "renderAquariumViewportHudLayer"
Assert-Contains "perf0_ui12_telemetry_avoids_console" $Shell "computeAquariumTelemetryOverlayRect"
Assert-Contains "perf0_active_render_path_enum_exists" $ConsoleH "AceEngineRenderPath"
Assert-Contains "perf0_active_render_path_string_zero_copy" $ConsoleH "DX12_ZERO_COPY"
Assert-Contains "perf0_shell_render_path_state" $ShellH "aquariumActiveRenderPath_"
Assert-Contains "perf0_gpu_stats_accessor_header" $GpuH "gpuStats() const"
Assert-Contains "perf0_gpu_stats_accessor_cpp" $GpuCpp "device_->gpuStats()"
Assert-Contains "perf0_docs_exist" $Docs "ACE-PERF0"

$Fields = @(
    "zeroCopyFrames", "readbackFrames", "compositionFrames", "compositionResizes", "targetResizes",
    "framesRendered", "lastPrimitiveCount", "lastVertexCount", "lastExtent", "uploadBytesAllocated",
    "uploadAllocations", "nativeBuffers", "nativeTextures", "nativePipelines", "compiledShaders",
    "descriptorAllocations", "drawCallsExecuted", "submittedGpuCommandLists", "completedFenceValue",
    "readbackBytes", "offscreenSceneTargets", "wvpConstantsUploaded"
)
foreach ($Field in $Fields) {
    if ($Shell -notlike "*$Field*") { throw "RHI formatter missing field: $Field" }
}
Write-Host "PASS|perf0_rhi_stats_fields_in_formatter"

$Probe = Join-Path $Root "Tools\AcePerf0StatsConsoleProbe.cpp"
$Exe = Join-Path $BuildDir "AcePerf0StatsConsoleProbe.exe"
$cl = Get-Command cl.exe -ErrorAction SilentlyContinue
if ($cl) {
    Write-Host "INFO|compiler|cl.exe"
    & $cl.Source /std:c++20 /EHsc /W4 /Fo"$BuildDir\" /Fe:$Exe $Probe
    if ($LASTEXITCODE -ne 0) { throw "cl.exe failed with exit code $LASTEXITCODE" }
    Push-Location $Root
    try {
        & $Exe
        if ($LASTEXITCODE -ne 0) { throw "AcePerf0StatsConsoleProbe failed with exit code $LASTEXITCODE" }
    }
    finally { Pop-Location }
} else {
    Write-Host "WARN|compiler|cl.exe not found; running static validation only in this environment"
}

Write-Host "PASS|ace_perf0_validation_complete"
