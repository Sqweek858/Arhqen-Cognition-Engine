param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot ".."))
)

$ErrorActionPreference = "Stop"
$uiCpp = Join-Path $Root "Source\Private\Ui\AceShellUi.cpp"
$uiH = Join-Path $Root "Source\Public\ArhqenCognitionEngine\Ui\AceShellUi.h"
$nativeCpp = Join-Path $Root "Source\Private\Renderer\NativeWindow.cpp"
$viewportCpp = Join-Path $Root "Source\Private\AquariumRender\AceAquariumEmbeddedDx12Viewport.cpp"
$viewportH = Join-Path $Root "Source\Public\ArhqenCognitionEngine\AquariumRender\AceAquariumEmbeddedDx12Viewport.h"

function Pass($name) { "PASS|$name" }
function Fail($name, $why) { "FAIL|$name|$why"; $script:failed = $true }
function ContainsText($path, $needle) { (Get-Content $path -Raw) -like "*$needle*" }

$script:failed = $false

$native = Get-Content $nativeCpp -Raw
$ui = Get-Content $uiCpp -Raw
$uiHeader = Get-Content $uiH -Raw
$viewport = Get-Content $viewportCpp -Raw
$viewportHeader = Get-Content $viewportH -Raw

if ($native -notmatch "WS_OVERLAPPEDWINDOW \| WS_CLIPCHILDREN") { Pass "top_level_window_no_clipchildren" } else { Fail "top_level_window_no_clipchildren" "WS_CLIPCHILDREN still present on top-level window style" }
if ($ui -match "D2D1_PRESENT_OPTIONS_RETAIN_CONTENTS") { Pass "d2d_hwnd_target_retain_contents" } else { Fail "d2d_hwnd_target_retain_contents" "D2D hwnd target does not request retained contents" }
if ($ui -match "case WM_SIZING:") { Pass "wm_sizing_arms_live_resize_transaction" } else { Fail "wm_sizing_arms_live_resize_transaction" "WM_SIZING not handled" }
if ($viewportHeader -match "HideForLiveResize" -and $viewport -match "HideForLiveResize") { Pass "embedded_viewport_has_strong_live_resize_hide" } else { Fail "embedded_viewport_has_strong_live_resize_hide" "HideForLiveResize API missing" }
if ($viewport -match "SWP_HIDEWINDOW" -and $viewport -match "-32768") { Pass "embedded_viewport_moved_offscreen_while_hidden" } else { Fail "embedded_viewport_moved_offscreen_while_hidden" "Live resize hide does not move child off-screen" }
if ($uiHeader -match "forceLiveResizeProxyRepaintNow" -and $ui -match "RDW_UPDATENOW") { Pass "live_resize_forces_synchronous_proxy_repaint" } else { Fail "live_resize_forces_synchronous_proxy_repaint" "No synchronous live-resize repaint" }
if ($uiHeader -match "restoreEmbeddedViewportAfterLiveResize" -and $ui -match "restoreEmbeddedViewportAfterLiveResize\(\)") { Pass "live_resize_restores_child_after_final_proxy_paint" } else { Fail "live_resize_restores_child_after_final_proxy_paint" "Final restore helper missing" }
if ($ui -match "RenderFrame\(aquariumController_" -and $ui -match "restoreEmbeddedViewportAfterLiveResize") { Pass "dx12_renders_once_after_final_resize" } else { Fail "dx12_renders_once_after_final_resize" "No post-resize render path found" }
if ($ui -match "windowLiveResizeActive_" -and $ui -match "renderAquariumResizeProxyViewport") { Pass "live_resize_proxy_path_still_present" } else { Fail "live_resize_proxy_path_still_present" "Proxy path missing" }

if ($failed) { exit 1 }
