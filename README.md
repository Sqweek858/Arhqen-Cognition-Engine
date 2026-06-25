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
