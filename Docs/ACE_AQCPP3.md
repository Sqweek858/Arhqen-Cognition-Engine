# ACE-AQCPP3

## Scope

`ACE-AQCPP3 = Scenarios / Meaning Tests / Randomization / Sensor Noise M9-M13`

This patch ports the next C++ headless Aquarium slice into Arhqen Cognition Engine.

Implemented:

- AQ-M9 = Contextual Meaning Tests
- AQ-M10 = Experiment Harness + Metrics
- AQ-M11 = Deterministic Scenario System
- AQ-M12 = Object Property Randomization
- AQ-M13 = Sensor Noise

## Scenario registry

Added:

- `AceAqScenarioDefinition`
- `AceAqScenarioBuildResult`
- `AceAqScenarioRegistry`
- `DefaultAceAqScenarioRegistry`
- `EvaluateScenarioExpectations`

Built-in scenarios:

- basic_wall
- water_front
- acid_front
- food_front
- stone_push
- unknown_liquid_fragile
- context_flip_cold_body
- same_appearance_liquids
- color_swap_train
- color_swap_test
- randomized_water_front
- randomized_acid_front
- cross_color_water_acid
- randomized_food_front

ObjectKind is allowed during evaluator-side scenario construction, but not in agent-facing observation outputs.

## Contextual meaning tests

Added:

- `AceAqScenarioResult`
- `AceAqMeaningTestResult`
- `AceAqMeaningTestRunner`

Implemented tests:

- color_swap
- same_appearance_different_effect
- context_flip
- unknown_liquid_safety
- symbol_grounding_sanity
- self_model_sanity
- integrated_contextual_run

Metrics include:

- color_dependency_score
- contradiction_count
- unsafe_unknown_consume_count
- safe_probe_before_consume_count
- context_flip_score_delta
- symbol_grounding_evidence_count
- self_external_event_separation_score

## Experiment harness and metrics

Added:

- `AceAqExperimentConfig`
- `AceAqExperimentResult`
- `AceAqExperimentRunSummary`
- `AceAqExperimentHarness`
- `AceAqMetricSnapshot`
- `AceAqRunMetrics`
- `ComputeRunMetrics`
- `AggregateMetricResults`

Supported planner names:

- random
- safe
- counterfactual

Random baseline is deterministic by seed.

## Object property randomization

Added:

- `AceAqRandomizationConfig`
- `AceAqRandomizedObjectProfile`
- `AceAqObjectPropertyRandomizer`
- `AceAqRandomizationToJsonLike`

Randomization can alter appearance features such as color/smell/temperature while preserving causal effects in the environment.

## Sensor noise

Added:

- `AceAqSensorNoiseConfig`
- `AceAqNoisyObservationMetadata`
- `AceAqSensorNoiseModel`
- `AceAqSensorNoiseToJsonLike`

Noise is deterministic by seed, clamps values, and does not introduce ObjectKind labels.

## Privacy rule

Agent-facing outputs must not contain:

- WATER
- ACID
- FOOD
- WALL
- STONE
- ICE
- ObjectKind
- debug_truth

Debug truth remains evaluator-side only.

## Validation

Run:

```powershell
.\Tools\validate_ace_aqcpp1.ps1
.\Tools\validate_ace_aqcpp2.ps1
.\Tools\validate_ace_aqcpp3.ps1
```

## Not implemented

This patch deliberately does not implement:

- DelayedEffectQueue
- DelayedEffect
- DelayedEffectApplication
- DynamicWorldSystem
- WorldEvent
- MOVING_HAZARD
- SPREADING_ACID
- POISON_FOOD
- SLOW_MEDICINE
- COLD_LIQUID
- FOOD decay
- real external world events
- 3D viewport
- DX12 Aquarium renderer
- UI integration
- Python bridge
- Panda/DearPyGui
- LLM/chat/tokenizer

## Next milestone

`ACE-AQCPP4 = Delayed Effects / Dynamic World / Full M0-M15 Parity`
