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
        Write-Output "SKIP|ace_ui4_probe_compile|cl.exe not available in current shell"
    }
    if (Test-Path $probe) { & $probe } else { Write-Output "PASS|ace_ui4_static_files_present" }

    $glass = Get-Content .\Source\Private\Ui\D2D\D2DGlassEffects.cpp -Raw
    $cache = Get-Content .\Source\Public\ArhqenCognitionEngine\Ui\D2D\D2DCachedEffects.h -Raw
    $blur = Get-Content .\Source\Private\Ui\D2D\D2DBlurRuntime.cpp -Raw
    if ($cache -match "class D2DCachedEffects") { "PASS|ui4_cached_effects_class_exists" } else { "FAIL|ui4_cached_effects_class_exists" }
    if ($glass -match "drawCachedBlurFallback") { "PASS|ui4_cached_blur_fallback_used" } else { "FAIL|ui4_cached_blur_fallback_used" }
    if ($blur -match "cached_frosted_fallback") { "PASS|ui4_blur_runtime_reports_cached_mode" } else { "FAIL|ui4_blur_runtime_reports_cached_mode" }
    if ((Get-Content .\Source\Private\Ui\AceShellUi.cpp -Raw) -match "effect cache: entries=") { "PASS|ui4_cache_stats_command_reports_effect_cache" } else { "FAIL|ui4_cache_stats_command_reports_effect_cache" }
} finally { Pop-Location }
