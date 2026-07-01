# Arhqen Cognition Engine

A C++ shell for future grounded cognitive architecture experiments.

ACE-CLEAN0 removes the old generic AI/cognitive backend logic. The current codebase keeps the Windows shell, custom cyberpunk UI framework, Direct2D/DirectWrite UI layer, native window infrastructure, optional DX12 renderer infrastructure, and the visual theme/layout systems.

The application is intentionally a clean shell in this patch. It includes a top bar shortcut named `Environment` that opens a placeholder panel for the future `3D Cognitive Environment`.

## Current scope

- Shell/UI/DX12 infrastructure preserved.
- Old AI backend logic deleted, not quarantined.
- Product branding changed to `Arhqen Cognition Engine`.
- `3D Cognitive Environment` placeholder added.
- No Aquarium bridge implemented yet.
- No 3D environment implemented yet.
- No new cognition implemented yet.

## Build

Use Visual Studio / MSBuild on Windows:

```powershell
msbuild .\ArhqenCognitionEngine.sln /p:Configuration=Release /p:Platform=x64
```

Or the existing debug build script:

```powershell
.\Scripts\build_debug.ps1
```

## Validation

```powershell
.\Tools\validate_ace_clean0.ps1
```


## ACE Aquarium Core C++ Port

- `ACE-AQCPP1` implements the first headless Aquarium runtime slice in C++.
- Included scope: homeostatic body, micro-world grid, hidden object effects, local observations, actions, step runtime, episode memory, and JSONL episode logging.
- The module is independent from UI/DX12 and has no Python runtime dependency.
- Run the validator with:

```powershell
.\Tools\validate_ace_aqcpp1.ps1
```


## ACE-AQCPP2 Prediction / Planning / Proto-Concepts / Symbols / Self Model

- `ACE-AQCPP2` adds the headless C++ M3-M8 Aquarium slice.
- Included scope: table world model, safe curiosity planner, counterfactual planner, causal proto-concepts, symbol binding v0, and functional self-model v0.
- These systems do not use debug truth or ObjectKind labels.
- No Python runtime dependency, no UI/DX12 integration, no 3D environment.

Run:

```powershell
.\Tools\validate_ace_aqcpp1.ps1
.\Tools\validate_ace_aqcpp2.ps1
```


## ACE-AQCPP3 Scenarios / Meaning / Metrics / Randomization / Sensor Noise

- `ACE-AQCPP3` adds deterministic scenarios, contextual meaning tests, experiment metrics, object property randomization, and sensor noise.
- These systems are C++ headless modules.
- No Python runtime dependency.
- No M14/M15 delayed effects or dynamic world.
- No UI/DX12/3D integration.

Run:

```powershell
.\Tools\validate_ace_aqcpp1.ps1
.\Tools\validate_ace_aqcpp2.ps1
.\Tools\validate_ace_aqcpp3.ps1
```


## ACE-AQCPP4 Full Headless Aquarium M0-M15 Parity

- `ACE-AQCPP4` completes the headless C++ port of Aquarium M0-M15.
- Added delayed consequences, dynamic world events, and full parity validation.
- No Python runtime dependency.
- No UI/DX12 Aquarium integration yet.
- No 3D environment yet.

C++ Aquarium Core Status:

- M0-M15 ported headless.
- No Python runtime dependency.
- No UI/DX12 integration yet.
- Next recommended milestone: `ACE-AQUI0 = Shell UI Bridge / Inspector / Headless Run Panel`.

Run:

```powershell
.\Tools\validate_ace_aqcpp1.ps1
.\Tools\validate_ace_aqcpp2.ps1
.\Tools\validate_ace_aqcpp3.ps1
.\Tools\validate_ace_aqcpp4.ps1
```


## ACE-AQUI0 Shell UI Aquarium Control Panel

- `ACE-AQUI0` adds the first shell UI control panel for the headless C++ Aquarium runtime.
- The existing `Environment` topbar entry now opens the Cognitive Environment Control Panel.
- The panel controls the C++ M0-M15 Aquarium core through `AceAquariumRuntimeController`.
- No Python runtime dependency.
- No 3D environment yet.
- No DX12 Aquarium renderer yet.

Next recommended milestone:

- `ACE-AQUI1 = Better Inspector + Episode Timeline + Scenario Run Dashboard`

## ACE-AQUI0R1 Validator Fix

Validator scripts now compile the complete current Aquarium core so older milestone probes link correctly after AQCPP4/AQUI0.

## ACE-AQUI0R2 Icon / RC Fix

Fixed missing `Source\Resources\ArhqenCognitionEngine.ico` referenced by `ArhqenCognitionEngine.rc`.


## ACE-UI1 Frosted Scroll Panels

- Adds frosted-glass polish, stronger border glow, subtle modal vignette, and scrollable Aquarium panel sections.
- Logs / Episodes, Debug Truth, Counterfactual, and Metrics now have scroll state and scrollbar rendering.
- No 3D viewport, no Python bridge, no image generation.

## ACE-UI1R1 Stronger Frost Real Scroll

- Strengthens Environment modal frosted-glass/vignette/border glow.
- Restores static Aquarium cards so they do not show decorative scrollbars.
- Keeps scrollbars only on long sections.

## ACE-UI1R2 Actual Layout Repair

- Environment is now a workspace-sized panel, not a small modal.
- Chat/home/input layer is hidden while Environment is open.
- Aquarium control button rects are computed from the visible panel.
- Fake vignette bars removed.
- Logs stay inside the Environment panel.

## ACE-UI1R3 Centered Dashboard

- Environment panel is centered in the app viewport.
- Top controls are split into a clean two-row command strip.
- Dashboard cards start below controls, preventing text/button overlap.
- Metrics is static again; scrollbars remain only on long sections.

## ACE-UI1R4 Header Spacing Grouped Controls

- Moves Environment subtitle away from the header divider.
- Groups Aquarium controls into Scenario, Planner, Runtime, and Manual Actions cards.
- Removes old overlapping scenario/planner text row.
- Starts dashboard cards below grouped controls.

## ACE-UI1R5 Contained Logs

- Keeps Logs / Episodes strictly inside the Environment panel.
- Shrinks upper dashboard cards when vertical space is tight.
- Keeps Logs / Episodes scrollable with bounded visible rect.

## ACE-UI1R6 Logs Focus Mode

- Adds a Focus Logs toggle to hide dashboard cards and give Logs / Episodes the full remaining Environment panel area.
- Removes Metrics from scroll hit-testing because Metrics is static.

## ACE-UI1R7 Content Clip Scrollbar

- Adds a real Environment content viewport with clipping.
- Adds one parent scrollbar for all content below controls.
- Keeps header/buttons fixed while dashboard/log content scrolls.
- Removes child scrollbars from Logs/Debug/Counterfactual hit-testing.

## ACE-UI1R8 Log End Padding Fix

- Fixes scrolled content bottom so Logs / Episodes bottom edge can be reached and seen.
- Adds end padding and a small bottom inset for Logs.

## ACE-UI1R9 Subtle Modal Blur + Hover + Softer Vignette

- bigger, lighter vignette with smoother falloff
- aquarium control buttons now react on hover
- Environment / Settings tabs add a more visible backdrop blur veil when open

## ACE-AQ3D0 Minimal Aquarium Viewport

ACE-AQ3D0 adds the first visual Aquarium viewport synchronized with the headless C++ runtime. The current path is a D2D/isometric fallback inside the Environment panel, with grid, agent, direction arrow, front-cell highlight, object primitives, and Debug Truth separation.

## ACE-AQ3D0R Real DX12 Environment Entry

ACE-AQ3D0R adds a real DX12 3D Environment entry from the Aquarium Control Panel. The previous D2D/isometric AQ3D0 card is no longer the default 3D path; the Control Panel now opens a separate DX12 viewport window synchronized with the existing C++ Aquarium runtime.

## ACE-AQ3D1 Embedded DX12 Environment Viewport

ACE-AQ3D1 embeds the DX12 Aquarium viewport directly inside the main Environment panel. The AQ3D0R separate-window path is no longer the main path; `Open 3D Environment` now shows/focuses an embedded DX12 child surface synchronized with the existing Aquarium C++ runtime.

## ACE-AQ3D2 Full Environment Workspace

ACE-AQ3D2 upgrades the Aquarium view into a full Environment Workspace with a stable embedded DX12 viewport. The viewport is large, embedded in the main shell, and guarded against per-frame resize/reinitialization flicker.

## ACE-AQ3D3 Full-Screen 3D Environment Mode

ACE-AQ3D3 adds a full-screen 3D Environment Mode launched from the Environment Control Panel.
The flow is `Environment -> Enter 3D Environment -> Back / Exit 3D`, using the same main window and the embedded DX12 viewport as the dominant surface.

## ACE-AQ3D4 3D Environment UI Layout Rework

ACE-AQ3D4 reworks the 3D Environment UI layout with a thin topbar, resizable details/log panels, and a cleaner Environment Control Panel.

## ACE-AQ3D5 Layout Determinism + Flicker Fix + Panel Clipping

ACE-AQ3D5 stabilizes the 3D Environment UI layout, clipping, panel resizing, logs scrolling, and viewport flicker behavior.

ACE-AQ3D7 fixes flicker by using dirty rect invalidation, stable child HWND sync outside paint, and queued DX12 viewport resize handling.

ACE-AQ3D8 stabilizes live window resize by deferring embedded DX12 child HWND sync and renderer resize/recreate until the final resize transaction completes.

ACE-AQ3D9 hides the child DX12 viewport during live resize and paints a stable D2D proxy until the final resize sync restores the viewport.

## ACE-AQ3D12 Owned Popup Resize Shield

ACE-AQ3D12 covers the live-resize viewport region with a no-activate owned popup shield while the embedded DX12 child HWND is hidden, then restores DX12 after the final resize.
## ACE-AQ3D13 Frozen Native Resize Commit

ACE-AQ3D13 freezes native border live-resize in 3D Environment mode and commits the final window RECT once on release, avoiding repeated child DX12 swapchain/D2D resize churn during the modal drag.


## ACE-AQ3D14 Slate-style Offscreen Viewport Composite

ACE-AQ3D14 moves the 3D Environment main path away from the legacy child-HWND DX12 viewport and into a single-HWND, Slate-style composited viewport path. This avoids the D2D parent plus child DX12 swapchain live-resize flicker path.

## ACE-UI3 DPI / Multi-Monitor Geometry Pass

ACE-UI3 adds a Slate-inspired display metrics layer for per-monitor DPI, monitor work-area clamping, client/screen rect conversion, and `WM_DPICHANGED` / `WM_DISPLAYCHANGE` handling.

## ACE-UI4 Cached Blur / Acrylic Effects Pass

ACE-UI4 adds cached frosted/acrylic fallback effect geometry, exposes effect-cache diagnostics through `cache_stats`, and keeps the current D2D renderer visually richer without importing Slate or adding a new renderer dependency.

## ACE-UI3F D2D Pixel-Space DPI Correction

ACE-UI3F pins the D2D HwndRenderTarget to explicit 96-DPI pixel-space so the existing Arhqen layout does not get auto-scaled on high-DPI monitors.
## ACE-UI3G Logical UI Scale + Viewport Fit Polish

ACE-UI3G keeps the D2D pixel-space DPI fix while making the 3D Environment UI more readable through controlled logical scaling, slightly larger text, wider panels, and a better viewport scene fit.

## ACE-UI5..UI11 + ACE-AQUI1 UI Foundation Pass

ACE-UI5 adds text clipping/ellipsis helpers, ACE-UI6 adds a retained layout tree, ACE-UI7 adds a draw command buffer, ACE-UI8 adds invalidation dirty regions, ACE-UI9 adds a UI debug overlay, ACE-AQUI1 adds Aquarium telemetry widgets, and ACE-UI11 adds the first named style-set layer.

## ACE-AQ3D11 Real 3D Viewport + Simple Camera

ACE-AQ3D11 adds a minimal real 3D DX12 path for the embedded Environment viewport: camera-relative WASD movement, Q/E world vertical movement, right-mouse look, view/projection matrices, Scene3DDrawList, DX12 3D shaders, and depth buffer support.

## ACE-AQ3D11R2 Resize Proxy + Mouse Direction Fix

ACE-AQ3D11R2 fixes mouse-look X direction and uses the stable D2D composite only as a temporary live-resize proxy while the real embedded DX12 3D viewport is hidden and restored after resize.

## ACE-AQ3D12 Single-HWND 3D Composition

ACE-AQ3D12 makes the Environment 3D main path single-HWND to avoid child HWND / DWM resize flicker. It keeps AceAquariumRealCamera input and uses a CPU projected Direct2D compositor as the flicker-safe bridge before a future DX12 offscreen texture path.

## ACE-RHI0 Renderer Core Foundation

ACE-RHI0 adds a backend-neutral renderer core: RHI types, resource registry, command lists, render graph, scene renderer, renderer facade and Null backend validation. This is the first step toward engine-level rendering without child-HWND flicker or D2D fake-3D.

## ACE-RHI1 + ACE-RHI2 DX12 Backend Shell

ACE-RHI1/RHI2 adds a real DX12 backend shell with hardware adapter selection, D3D12 device/queue/allocator/command list/fence, RTV/DSV/CBV-SRV-UAV descriptor heaps, native buffer/texture backing, upload arena, GPU buffer copy and render-target clear smoke validation.

## ACE-RHI3 + ACE-RHI4 DX12 Draw + Offscreen Scene

ACE-RHI3/RHI4 adds the first real DX12 draw path: embedded BasicColor HLSL, D3DCompile, root signature, graphics PSO, input layout, DrawInstanced/DrawIndexedInstanced, offscreen SceneColor/SceneDepth targets and a GPU probe that renders a MeshBatch through the renderer facade.

## ACE-RHI5 + ACE-RHI6 GPU Environment Viewport

ACE-RHI5/RHI6 connects the DX12 offscreen scene path to the main Environment viewport: DX12 renders Aquarium grid/blocks/agent mesh geometry into SceneColor/SceneDepth, reads back BGRA8, and the main HWND composes it as a D2D bitmap without reviving the child HWND flicker path. This is a stable bridge before zero-copy DirectComposition/shared-texture composition.

## ACE-RHI7 + ACE-RHI8 GPU 3D + Zero-Copy Composition

ACE-RHI7/RHI8 adds camera WVP root constants, a WVP HLSL shader path, depth-tested real 3D Aquarium mesh geometry, and a DirectComposition composition-swapchain path that presents DX12 SceneColor into the main HWND without CPU readback on the fast path. Readback remains only as fallback.
