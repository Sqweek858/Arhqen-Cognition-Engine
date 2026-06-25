# ACE-AQUI0

## Scope

`ACE-AQUI0 = Shell UI Aquarium Control Panel`

This patch integrates the completed headless C++ Aquarium M0-M15 runtime into the existing Arhqen Cognition Engine shell UI.

It does not implement a 3D viewport. The Environment topbar entry opens a control/inspector panel for the headless runtime.

## Architecture

```text
Shell UI Environment entry
    -> Cognitive Environment Control Panel
        -> AceAquariumRuntimeController
            -> AceAqEnvironment / ScenarioRegistry / Planners / Metrics
```

The UI talks to one central controller. It does not directly wire buttons to every Aquarium subsystem.

## Added runtime controller

Added:

- `AceAquariumRuntimeController`
- `AceAquariumUiSnapshot`
- `AceAquariumLogModel`
- `AceAquariumPanel`

Controller responsibilities:

- initialize scenario registry
- reset selected scenario
- select planner
- run/pause
- step planner-chosen action
- step manual action
- tick running mode with max steps/second cap
- maintain world model
- maintain proto-concepts
- maintain symbol table
- maintain self-model
- produce UI-friendly snapshots
- produce recent log entries

## UI panel

The existing topbar `Environment` entry now opens:

```text
Cognitive Environment Control Panel
```

The panel states:

```text
3D viewport is not implemented yet.
This panel controls the headless Aquarium C++ runtime.
```

Controls implemented:

- scenario previous/next
- planner previous/next
- reset
- step
- run/pause
- debug truth toggle
- manual forward
- manual turn left
- manual turn right
- manual wait
- manual touch
- manual consume
- manual push

Sections implemented:

- Body / Agent
- Observation
- Planner Trace
- Counterfactual
- Memory / Concepts
- Self Model
- Delayed / Dynamic
- World Events
- Metrics
- Logs / Episodes
- Debug Truth, only when explicitly enabled

## Debug truth separation

When Debug Truth is off:

- snapshot agent-facing text is checked for ObjectKind/debug labels
- UI does not show debug truth
- observation lines use only sensor features

When Debug Truth is on:

- a separate section appears
- it is marked:

```text
DEBUG TRUTH - NOT AGENT INPUT
```

## Privacy rule

Agent-facing snapshot lines must not contain:

- WATER
- ACID
- FOOD
- WALL
- STONE
- ICE
- POISON_FOOD
- SLOW_MEDICINE
- COLD_LIQUID
- MOVING_HAZARD
- SPREADING_ACID
- ObjectKind
- debug_truth

## Validation

Run:

```powershell
.\Tools\validate_ace_aqcpp1.ps1
.\Tools\validate_ace_aqcpp2.ps1
.\Tools\validate_ace_aqcpp3.ps1
.\Tools\validate_ace_aqcpp4.ps1
.\Tools\validate_ace_aqui0.ps1
```

`AceAqui0Probe` validates the runtime controller headlessly:

- initializes
- lists scenarios
- resets scenarios
- changes planner
- steps planner/manual action
- running tick advances
- paused tick does not advance
- snapshot body state
- observation lines
- planner lines
- counterfactual lines
- delayed/dynamic lines
- logs
- debug truth separation
- privacy when debug truth is off

## Not implemented

This patch deliberately does not implement:

- 3D Environment
- DX12 Aquarium renderer
- mesh/grid/cubes/camera
- Python bridge
- Panda/DearPyGui
- AQ-M16+
- neural network
- LLM/chat/tokenizer

## Next milestone

Recommended:

```text
ACE-AQUI1 = Better Inspector + Episode Timeline + Scenario Run Dashboard
```

Alternative later milestone:

```text
ACE-AQ3D0 = Minimal DX12 Aquarium Viewport
```
