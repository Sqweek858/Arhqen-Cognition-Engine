# ACE-AQCPP4

## Scope

`ACE-AQCPP4 = Delayed Effects / Dynamic World / Full M0-M15 Parity`

This patch completes the headless C++ port of Aquarium M0-M15 inside Arhqen Cognition Engine.

Implemented:

- AQ-M14 = Delayed Consequences
- AQ-M15 = Dynamic Objects
- Full M0-M15 parity audit

## Delayed effects

Added:

- `AceAqDelayedEffect`
- `AceAqDelayedEffectQueue`
- `AceAqDelayedEffectApplication`
- `AceAqDelayedEffectConfig`

New object kinds:

- `PoisonFood`
- `SlowMedicine`
- `ColdLiquid`

Behavior:

- PoisonFood: immediate nutrition, delayed integrity damage
- SlowMedicine: small immediate integrity benefit, delayed healing
- ColdLiquid: hydration and temperature drop, plus delayed cooling

## Delayed effect order

`AceAqEnvironment::Step` order:

1. capture body_before
2. capture observation_before
3. execute chosen action
4. schedule delayed effects from consumed object
5. apply natural decay
6. apply due delayed effects
7. tick optional dynamic world
8. compute body_after / actual delta / termination
9. build observation_after
10. create episode
11. record scheduled delayed effects, applied delayed effects, external world events
12. update memory/logger

## Dynamic world

Added:

- `AceAqWorldEvent`
- `AceAqDynamicWorldConfig`
- `AceAqDynamicWorldSystem`

New object kinds:

- `MovingHazard`
- `SpreadingAcid`

Behaviors:

- MovingHazard moves deterministically and can produce external damage.
- SpreadingAcid expands deterministically into adjacent empty cells.
- Food decay converts Food into PoisonFood.

## External event separation

`AceAqSelfModel` now treats episodes with real external world events or external delayed effects as not self-caused.

This means MovingHazard / SpreadingAcid damage does not raise agency evidence as if the agent caused it.

## Scenario updates

Added scenarios:

- poison_food_front
- slow_medicine_front
- cold_liquid_front
- moving_hazard
- spreading_acid
- food_decay
- dynamic_hazard_damage
- delayed_poison_damage

## Metrics updates

`AceAqMetricSnapshot` now includes:

- pending_delayed_effect_count
- applied_delayed_effect_count
- external_world_event_count
- dynamic_damage_count
- food_decay_count

## Privacy

Agent-facing outputs must not contain:

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

Debug truth remains evaluator-side only.

## Full M0-M15 parity

Added:

- `AceAqParityCheck`
- `RunAceAqM0M15ParityChecks`

The parity probe verifies:

- M0 Body/Homeostasis
- M1 World/Observation
- M2 Actions/Episodes
- M3 WorldModel
- M4 SafePlanner
- M5 ProtoConcepts
- M6 CounterfactualPlanner
- M7 SymbolTable
- M8 SelfModel
- M9 MeaningTests
- M10 ExperimentHarness/Metrics
- M11 ScenarioRegistry
- M12 Randomization
- M13 SensorNoise
- M14 DelayedEffects
- M15 DynamicWorld
- integrated M0-M15 run

## Validation

Run:

```powershell
.\Tools\validate_ace_aqcpp1.ps1
.\Tools\validate_ace_aqcpp2.ps1
.\Tools\validate_ace_aqcpp3.ps1
.\Tools\validate_ace_aqcpp4.ps1
```

## Not implemented

This patch deliberately does not implement:

- 3D Environment
- DX12 Aquarium renderer
- UI integration
- topbar Aquarium controls
- Python bridge
- Panda/DearPyGui
- neural network
- LLM/chat/tokenizer
- AQ-M16+

## Next milestone

`ACE-AQUI0 = Shell UI Bridge / Inspector / Headless Run Panel`
