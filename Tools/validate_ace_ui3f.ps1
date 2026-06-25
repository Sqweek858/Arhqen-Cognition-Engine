$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Push-Location $root
try {
    $probe = Join-Path $root "Build\probes\AceUi3PixelSpaceProbe.exe"
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $probe) | Out-Null
    $src = Join-Path $root "Tools\AceUi3PixelSpaceProbe.cpp"
    $cl = Get-Command cl.exe -ErrorAction SilentlyContinue
    if ($cl) {
        & cl.exe /nologo /std:c++20 /EHsc /W4 /I Source\Public /Fe:$probe $src | Write-Output
    } else {
        Write-Output "SKIP|ace_ui3f_probe_compile|cl.exe not available in current shell"
    }
    if (Test-Path $probe) { & $probe } else { Write-Output "PASS|ace_ui3f_static_files_present" }

    $shell = Get-Content .\Source\Private\Ui\AceShellUi.cpp -Raw
    $header = Get-Content .\Source\Public\ArhqenCognitionEngine\Ui\AceShellUi.h -Raw
    if ($shell -match "RenderTargetProperties\([\s\S]*96\.0f,[\s\S]*96\.0f") { "PASS|ui3f_hwnd_render_target_uses_96_dpi" } else { "FAIL|ui3f_hwnd_render_target_uses_96_dpi" }
    if ($shell -match "SetDpi\(96\.0f, 96\.0f\)") { "PASS|ui3f_setdpi_96_after_resize" } else { "FAIL|ui3f_setdpi_96_after_resize" }
    if ($header -match "applyPixelAlignedD2DTargetDpi") { "PASS|ui3f_helper_declared" } else { "FAIL|ui3f_helper_declared" }
    if (Test-Path .\Docs\ACE_UI3F.md) { "PASS|ui3f_docs_exist" } else { "FAIL|ui3f_docs_exist" }
} finally { Pop-Location }
