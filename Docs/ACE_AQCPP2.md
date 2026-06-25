# ACE-AQCPP2

## Scope

`ACE-AQCPP2 = Prediction, Planning, Proto-Concepts, Symbols, Self Model M3-M8`

This patch extends the headless C++ Aquarium runtime inside Arhqen Cognition Engine.

It ports:

- AQ-M3 = TableWorldModel
- AQ-M4 = Safe Curiosity Planner
- AQ-M5 = Causal Proto-Concepts
- AQ-M6 = Counterfactual Planning
- AQ-M7 = Symbol Binding v0
- AQ-M8 = SelfModel v0

## Implemented

### M3 TableWorldModel

Added:

- `AceAqObservationSignature`
- `AceAqTransitionStats`
- `AceAqPrediction`
- `AceAqTableWorldModel`

The model learns from:

```text
observation_before + action -> actual body delta / result flags
```

The signature is derived only from agent-facing sensory features:

- solid hint
- liquid-like hint
- color buckets
- smell bucket
- temperature bucket

It does not use `ObjectKind`, debug truth, or labels such as `WATER`, `ACID`, `FOOD`, `WALL`, `STONE`, `ICE`.

### M4 Safe Curiosity Planner

Added:

- `AceAqPlannerConfig`
- `AceAqActionEvaluation`
- `AceAqDecisionTrace`
- `AceAqSafeCuriosityPlanner`

Score formula:

```text
score = pragmatic_value + beta * information_gain - gamma * risk - step_cost
```

The planner uses body state, observation, and world-model predictions. It does not use debug truth.

### M5 Causal Proto-Concepts

Added:

- `AceAqCausalSignature`
- `AceAqProtoConcept`
- `AceAqProtoConceptMiner`

Effect profile terms:

- hydration_up
- nutrition_up
- integrity_down
- temperature_down
- blocked
- moved
- pushable_effect
- not_consumable_or_blocked
- neutral

Concepts are grouped by observation signature + action + effect profile. No world truth labels are used.

### M6 Counterfactual Planning

Added:

- `AceAqCounterfactualConfig`
- `AceAqImaginedOutcome`
- `AceAqActionBranch`
- `AceAqPlanCandidate`
- `AceAqCounterfactualTrace`
- `AceAqCounterfactualPlanner`

Depth 1 is direct.

Depth 2 is approximate and explicitly marked in trace with:

```text
depth_2_rollout_approximation
reused_current_observation_for_depth_2
```

### M7 Symbol Binding v0

Added:

- `AceAqSymbolBinding`
- `AceAqSymbolActivation`
- `AceAqConceptActivation`
- `AceAqSymbolTable`

This is not NLP, not chat, not a tokenizer, and not an LLM. It is a small symbolic binding table from symbol string to proto-concept ID and back.

Activation is case-insensitive and ambiguity is supported.

### M8 SelfModel v0

Added:

- `AceAqAgencyAssessment`
- `AceAqSelfEpisodeRecord`
- `AceAqSelfModelState`
- `AceAqSelfModel`

This is a functional self-model for agency bookkeeping. It is not a consciousness claim.

It tracks:

- owned episodes
- external event separation placeholder
- agency confidence
- prediction error history
- previous/current body state

## Privacy

Agent-facing outputs must not include:

- WATER
- ACID
- FOOD
- WALL
- STONE
- ICE
- ObjectKind
- debug_truth

Debug truth remains separate and evaluator-only.

## Validation

Run:

```powershell
.\Tools\validate_ace_aqcpp1.ps1
.\Tools\validate_ace_aqcpp2.ps1
```

`AceAqCpp2Probe` checks:

- world model learning
- confidence increase
- prediction error
- planner risk avoidance
- planner hydration preference after learning
- decision trace completeness
- proto-concept effects/supporting IDs
- counterfactual candidate plans and depth-2 notes
- symbol binding case-insensitive ambiguity
- self-model agency/external event separation
- observation privacy
- debug truth separation

## Not implemented yet

This patch deliberately does not implement:

- ScenarioRegistry
- ExperimentHarness
- Contextual Meaning Tests M9
- ObjectPropertyRandomizer
- SensorNoiseModel
- DelayedEffectQueue
- DynamicWorldSystem
- 3D viewport
- DX12 aquarium renderer
- Python bridge
- Panda/DearPyGui
- LLM/chat/tokenizer

## Next milestone

`ACE-AQCPP3 = Scenarios / Meaning Tests / Randomization / Noise M9-M13`
