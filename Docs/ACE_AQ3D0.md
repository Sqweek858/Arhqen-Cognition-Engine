# ACE-AQ3D0 Minimal DX12 Aquarium Viewport

## Scope

`ACE-AQ3D0 = Minimal DX12 Aquarium Viewport`

This milestone adds the first visual Aquarium viewport synchronized with the existing headless C++ Aquarium runtime.

The current implementation uses a safe **D2D/isometric fallback viewport** inside the Environment panel. It does not add a full DX12 embedded 3D render target yet. The fallback path is intentional for AQ3D0 because the current shell UI is already D2D/HwndRenderTarget-based and the goal is the first synchronized visual viewport without destabilizing the renderer stack.

## Architecture

```text
Aquarium Core M0-M15
  ↓
AceAquariumRuntimeController
  ↓
AceAquariumUiSnapshot / runtime state
  ↓
AceAquariumSceneAdapter
  ↓
AceAquariumViewport
  ↓
Environment panel D2D/isometric viewport card
```

## Files

```text
Source/Public/ArhqenCognitionEngine/AquariumRender/AceAquariumRenderPrimitive.h
Source/Public/ArhqenCognitionEngine/AquariumRender/AceAquariumSceneAdapter.h
Source/Public/ArhqenCognitionEngine/AquariumRender/AceAquariumViewport.h
Source/Public/ArhqenCognitionEngine/AquariumRender/AceAquariumCamera.h

Source/Private/AquariumRender/AceAquariumRenderPrimitive.cpp
Source/Private/AquariumRender/AceAquariumSceneAdapter.cpp
Source/Private/AquariumRender/AceAquariumViewport.cpp
Source/Private/AquariumRender/AceAquariumCamera.cpp
```

## What the viewport shows

The Environment panel now contains a `3D Viewport` card.

Visible elements:

- grid/floor lines
- base floor tiles
- agent marker
- direction arrow
- front cell highlight
- object primitives derived from agent-facing sensor properties
- optional debug labels only when Debug Truth is enabled

## Rendering path

```text
D2D/isometric fallback
```

No real DX12 embedded viewport is implemented yet.

## Privacy / Debug Truth

When Debug Truth is OFF:

- viewport primitives do not expose ObjectKind labels
- no WATER / ACID / FOOD / WALL / STONE / ICE / etc. labels are emitted
- scene visuals are derived from generic visible/sensory properties:
  - solid hint
  - liquid-like hint
  - color
  - smell/temperature/effect hints

When Debug Truth is ON:

- debug labels may include ObjectKind names
- a separate warning primitive is emitted:

```text
DEBUG TRUTH - NOT AGENT INPUT
```

Debug truth is never used by planner/world-model decision logic.

## Validation

Run:

```powershell
.\Tools\validate_ace_aqcpp1.ps1
.\Tools\validate_ace_aqcpp2.ps1
.\Tools\validate_ace_aqcpp3.ps1
.\Tools\validate_ace_aqcpp4.ps1
.\Tools\validate_ace_aqui0.ps1
.\Tools\validate_ace_ui1r9.ps1
.\Tools\validate_ace_aq3d0.ps1
.\Scripts\build_debug.ps1
```

`validate_ace_aq3d0.ps1` compiles and runs:

```text
Tools/AceAq3D0Probe.cpp
```

## Non-goals

Not implemented in AQ3D0:

- full DX12 embedded viewport
- 3D editor
- model import
- terrain
- physics engine
- material graph
- shader pack
- Python bridge
- Panda3D / DearPyGui
- AQ-M16+
- LLM / chat / tokenizer

## Next recommended milestone

```text
ACE-AQ3D1 = Camera/Input/Viewport Polish + Object Visuals
```

Recommended focus:

- camera zoom/pan/orbit
- better isometric object shapes
- selected cell inspector
- stronger front-cell highlight
- optional transition to real DX12 embedded render target after UI fallback is stable
