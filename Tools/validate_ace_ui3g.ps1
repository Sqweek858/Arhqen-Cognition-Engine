$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
Set-Location $Root

function Pass($name) { "PASS|$name" }
function Fail($name, $why) { "FAIL|$name|$why"; exit 1 }
function Contains($path, $needle, $name) {
    if ((Get-Content $path -Raw) -notlike "*$needle*") { Fail $name "missing '$needle' in $path" }
    Pass $name
}

Contains "Source/Public/ArhqenCognitionEngine/AquariumUI/AceEnvironment3DMode.h" "LogicalScale = 1.16f" "ui3g_logical_scale_constant_exists"
Contains "Source/Public/ArhqenCognitionEngine/AquariumUI/AceEnvironment3DMode.h" "TopbarHeight = 30.0f" "ui3g_topbar_larger"
Contains "Source/Public/ArhqenCognitionEngine/AquariumUI/AceEnvironment3DMode.h" "ButtonHeight = 30.0f" "ui3g_buttons_larger"
Contains "Source/Public/ArhqenCognitionEngine/AquariumUI/AceEnvironment3DMode.h" "detailsWidth = 430.0f" "ui3g_left_panel_default_larger"
Contains "Source/Public/ArhqenCognitionEngine/AquariumUI/AceEnvironment3DMode.h" "logsWidth = 370.0f" "ui3g_logs_panel_default_larger"
Contains "Source/Private/Ui/AceShellUi.cpp" "Segoe UI\", 15.0f" "ui3g_small_font_nudged_up"
Contains "Source/Private/Ui/AceShellUi.cpp" "Segoe UI\", 19.0f" "ui3g_body_button_fonts_nudged_up"
Contains "Source/Private/Ui/AceShellUi.cpp" "renderTarget_->SetDpi(96.0f, 96.0f)" "ui3f_pixel_space_dpi_still_active"
Contains "Source/Private/Ui/AceShellUi.cpp" "aquariumUseSingleHwndCompositeViewport_" "aq3d14_single_hwnd_path_still_present"
Contains "Source/Private/Ui/AceShellUi.cpp" "* 2.86f" "ui3g_viewport_fit_zoom_polished"
Contains "Docs/ACE_UI3G.md" "ACE-UI3G" "ui3g_docs_exist"

$rootArtifacts = Get-ChildItem -File -Path . | Where-Object { $_.Extension -in '.obj','.exe','.pdb','.ilk','.log' }
if ($rootArtifacts.Count -gt 0) { Fail "no_build_artifacts_in_repo_root" ($rootArtifacts.Name -join ',') }
Pass "no_build_artifacts_in_repo_root"
