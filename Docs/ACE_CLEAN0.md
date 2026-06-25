# ACE-CLEAN0

## Implemented

`ACE-CLEAN0 = Brutal Legacy AI Removal + Arhqen Cognition Engine Rebrand`

## Deleted

The old AI/cognitive backend was removed rather than quarantined:

- `Source/Private/Cognitive/`
- `Source/Public/ArchitectMind/Cognitive/`
- `Source/Private/Memory/`
- `Source/Public/ArchitectMind/Memory/`
- `Source/Private/Core/CognitiveUiBridge.cpp`
- `Source/Public/ArchitectMind/Core/CognitiveUiBridge.h`
- old milestone docs and validation scripts
- old Win32 mock `ChatBoxUi` files

## Preserved

- Native Windows app/window loop
- Custom cyberpunk UI shell
- Direct2D/DirectWrite UI framework
- optional DX12 renderer infrastructure
- theme/style/layout/render helpers
- command palette/layout/diagnostics UI infrastructure

## Rebranded

Main UI/docs/config now use:

`Arhqen Cognition Engine`

Internal abbreviation:

`ACE`

The project and output target were renamed to `ArhqenCognitionEngine` where safe.

## New UI placeholder

Top bar entry:

`Environment`

Clicking it opens:

`3D Cognitive Environment`

with placeholder text:

`The 3D sandbox is not implemented yet. This panel is reserved for the future Arhqen Cognition Engine visual environment.`

## Not implemented

- no 3D viewport
- no Aquarium bridge
- no Python backend launch
- no snapshot/command protocol
- no new cognition
- no LLM/chat/tokenizer/neural network

## Recommended next step

Implement a small shell-side `AceEnvironmentPanel` abstraction and only then connect it to a future external backend protocol. Keep the UI shell clean and do not reintroduce old AI assistant logic.
