param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot ".."))
)

$ErrorActionPreference = "Stop"
$uiCpp = Join-Path $Root "Source\Private\Ui\AceShellUi.cpp"
$uiH = Join-Path $Root "Source\Public\ArhqenCognitionEngine\Ui\AceShellUi.h"
$doc = Join-Path $Root "Docs\ACE_AQ3D12.md"

function Pass($name) { "PASS|$name" }
function Fail($name, $why) { "FAIL|$name|$why"; $script:failed = $true }
function Has($text, $needle) { return $text.Contains($needle) }

$script:failed = $false
$ui = Get-Content $uiCpp -Raw
$h = Get-Content $uiH -Raw

if (Has $h "aquariumResizeShieldHwnd_") { Pass "resize_popup_shield_state_exists" } else { Fail "resize_popup_shield_state_exists" "Missing resize shield HWND state" }
if (Has $ui "ArhqenCognitionEngineAquariumResizeShieldPopup") { Pass "resize_popup_shield_window_class_exists" } else { Fail "resize_popup_shield_window_class_exists" "Missing resize shield popup class" }
if (Has $ui "WS_POPUP" -and Has $ui "WS_EX_NOACTIVATE") { Pass "resize_popup_is_owned_noactivate_popup" } else { Fail "resize_popup_is_owned_noactivate_popup" "Shield is not a no-activate popup" }
if (Has $ui "AquariumResizeShieldWindowProc" -and Has $ui "WM_PAINT" -and Has $ui "FillRect") { Pass "resize_popup_paints_own_stable_proxy" } else { Fail "resize_popup_paints_own_stable_proxy" "Shield does not paint its own proxy" }
if (Has $ui "case WM_NCLBUTTONDOWN:" -and Has $ui "beginWindowLiveResize();") { Pass "resize_popup_arms_before_modal_loop" } else { Fail "resize_popup_arms_before_modal_loop" "WM_NCLBUTTONDOWN does not arm live resize" }
if (Has $ui "updateAquariumResizeShieldWindow(aquariumEmbeddedViewportRect_)") { Pass "resize_popup_updates_during_live_resize" } else { Fail "resize_popup_updates_during_live_resize" "Shield is not updated during live resize" }
if (Has $ui "ClientToScreen(parent_, &topLeft)") { Pass "resize_popup_uses_screen_coordinates" } else { Fail "resize_popup_uses_screen_coordinates" "Shield does not convert viewport rect to screen coordinates" }
if (Has $ui "HideForLiveResize") { Pass "dx12_child_still_hidden_for_live_resize" } else { Fail "dx12_child_still_hidden_for_live_resize" "DX12 child is not hidden for live resize" }
if (Has $ui "hideAquariumResizeShieldWindow();") { Pass "resize_popup_hides_after_dx12_restore" } else { Fail "resize_popup_hides_after_dx12_restore" "Shield hide after restore missing" }
if (Has $h "resizeShieldPaintCount_") { Pass "resize_popup_counters_exist" } else { Fail "resize_popup_counters_exist" "Shield diagnostic counters missing" }
if (Test-Path $doc) { Pass "docs_aq3d12_exists" } else { Fail "docs_aq3d12_exists" "Docs/ACE_AQ3D12.md missing" }

$probe = Join-Path $Root "Tools\AceAq3D12ResizePopupShieldProbe.cpp"
if (Test-Path $probe) { Pass "aq3d12_probe_exists" } else { Fail "aq3d12_probe_exists" "Probe source missing" }

if ($failed) { exit 1 }
