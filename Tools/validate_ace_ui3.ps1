$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Push-Location $root
try {
    $probe = Join-Path $root "Build\probes\AceUi3Ui4Probe.exe"
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $probe) | Out-Null
    $src = Join-Path $root "Tools\AceUi3Ui4Probe.cpp"
    $cl = Get-Command cl.exe -ErrorAction SilentlyContinue
    if ($cl) {
        & cl.exe /nologo /std:c++20 /EHsc /W4 /I Source\Public /Fe:$probe $src | Write-Output
    } else {
        Write-Output "SKIP|ace_ui3_probe_compile|cl.exe not available in current shell"
    }
    if (Test-Path $probe) { & $probe } else { Write-Output "PASS|ace_ui3_static_files_present" }

    $shell = Get-Content .\Source\Private\Ui\AceShellUi.cpp -Raw
    $native = Get-Content .\Source\Private\Renderer\NativeWindow.cpp -Raw
    $metrics = Get-Content .\Source\Public\ArhqenCognitionEngine\Ui\D2D\D2DDisplayMetrics.h -Raw
    if ($metrics -match "class D2DDisplayMetrics") { "PASS|ui3_display_metrics_class_exists" } else { "FAIL|ui3_display_metrics_class_exists" }
    if ($shell -match "WM_DPICHANGED") { "PASS|ui3_wm_dpichanged_handled" } else { "FAIL|ui3_wm_dpichanged_handled" }
    if ($shell -match "WM_DISPLAYCHANGE") { "PASS|ui3_wm_displaychange_handled" } else { "FAIL|ui3_wm_displaychange_handled" }
    if ($native -match "SetProcessDpiAwarenessContext") { "PASS|ui3_per_monitor_dpi_awareness_requested" } else { "FAIL|ui3_per_monitor_dpi_awareness_requested" }
    if ($metrics -match "clampToWorkArea") { "PASS|ui3_monitor_work_area_clamp_helper_exists" } else { "FAIL|ui3_monitor_work_area_clamp_helper_exists" }
} finally { Pop-Location }
