# ACE-AQCPP1

## Scope

`ACE-AQCPP1 = C++ Aquarium Core Runtime M0-M2`

This patch ports the first grounded Aquarium runtime slice into the Arhqen Cognition Engine C++ codebase.

It is a headless runtime module. It is independent from the Direct2D/DX12 shell and is not wired into the UI yet.

## Implemented from Python Aquarium reference

### AQ-M0 = Homeostatic Body

Implemented:

- `AceAqBodyState`
- `AceAqBodyDelta`
- `AceAqIdealRange`
- `ComponentErrors`
- `HomeostaticError`
- `NaturalDecay`
- `IsDead`
- `HomeostaticReward`
- clamp-to-0..1 behavior

### AQ-M1 = MicroWorld + Local Observations

Implemented:

- `AceAqAction`
- `AceAqDirection`
- `AceAqGridWorld`
- agent position/direction
- turn left/right
- move forward
- front/left/right/current positions
- local 3x3 observation
- hidden object kinds
- agent-facing sensory features only

### AQ-M2 = Object Interactions + Episode Logging

Implemented:

- base object kinds: Empty, Wall, Water, Acid, Food, Ice, Stone
- object effects:
  - Water consume: hydration +0.35, temperature -0.03
  - Acid touch: integrity -0.20
  - Acid consume: integrity -0.55
  - Food consume: nutrition +0.35
  - Ice touch: temperature -0.10
  - Stone push if target cell behind is empty
- `AceAqLastActionResult`
- `AceAqEpisode`
- `AceAqEpisodeMemory`
- `AceAqEpisodeLogger`
- JSONL episode logging to `Build/Logs/`

## Observation privacy

Object kind labels are internal world truth.

Agent observations serialize only:

- `solid_hint`
- `liquid_like_hint`
- `color_rgb`
- `temperature_signal`
- `smell_signal`
- `body_state`
- `last_action_reason`

Observation JSON-like output must not contain:

- WATER
- ACID
- FOOD
- WALL
- STONE
- ICE
- ObjectKind
- debug_truth

Debug truth exists separately through `AceAqGridWorld::DebugTruthJsonLike()` / `AceAqEnvironment::GetDebugTruthJsonLike()` for evaluator tools and validation only.

## Validation

Run:

```powershell
.\Tools\validate_ace_aqcpp1.ps1
```

The validator compiles and runs `Tools/AceAqCpp1Probe.cpp` against the C++ Aquarium core.

It checks:

- ideal homeostatic error
- natural decay
- delta clamp
- wall blocking
- empty movement
- turn left/right
- acid touch damage
- water consume hydration and cell clearing
- wall consume failure
- stone push
- episode memory growth
- actual delta
- observation privacy
- debug truth separation
- JSONL logging

## Not implemented yet

This patch deliberately does not implement:

- TableWorldModel
- SafeCuriosityPlanner
- CounterfactualPlanner
- ProtoConceptMiner
- SymbolTable
- SelfModel
- scenario registry
- metrics
- randomization
- sensor noise
- delayed effects
- dynamic world
- Python bridge
- UI integration
- 3D environment
- DX12 Aquarium renderer

## Recommended next step

`ACE-AQCPP2` should port the table world model and safe curiosity planner, still headless and still independent from the UI.
