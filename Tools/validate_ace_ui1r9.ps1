$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$Header = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\Ui\AceShellUi.h") -Raw
$Cpp = Get-Content (Join-Path $Root "Source\Private\Ui\AceShellUi.cpp") -Raw
$Bg = Get-Content (Join-Path $Root "Source\Private\Ui\D2D\D2DCyberBackgroundField.cpp") -Raw

$Checks = @(
    @{Name="mouse_state"; Needle="float mouseX_ = 0.0f;"; Hay=$Header},
    @{Name="hover_helper_decl"; Needle="bool isAquariumButtonHovered(UiRect rect) const;"; Hay=$Header},
    @{Name="hover_helper_impl"; Needle="bool AceShellUi::isAquariumButtonHovered(UiRect rect) const"; Hay=$Cpp},
    @{Name="aquarium_button_hover"; Needle="const bool hovered = isAquariumButtonHovered(rect);"; Hay=$Cpp},
    @{Name="settings_modal_blur"; Needle="settingsModalRect_.inset(-18.0f)"; Hay=$Cpp},
    @{Name="environment_modal_blur"; Needle="environmentModalRect_.inset(-26.0f)"; Hay=$Cpp},
    @{Name="vignette_patch_marker"; Needle="ACE-UI1R9: bigger and softer vignette."; Hay=$Bg},
    @{Name="vignette_extra_band"; Needle="{392.0f, 0.035f}"; Hay=$Bg}
)
foreach ($check in $Checks) {
    if ($check.Hay -notlike "*${($check.Needle)}*") { throw "Missing $($check.Name): $($check.Needle)" }
    Write-Host "PASS|$($check.Name)|$($check.Needle)"
}
Write-Host "PASS|ace_ui1r9_static_validation|blur hover vignette markers present"
