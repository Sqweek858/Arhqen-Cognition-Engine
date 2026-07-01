# ACE-RHI5 + ACE-RHI6

## Scope

```text
ACE-RHI5 = main-HWND composition of offscreen DX12 scene texture
ACE-RHI6 = Aquarium/Environment viewport uses GPU mesh grid/blocks
```

RHI3/RHI4 introduced the first real DX12 draw path and offscreen SceneColor/SceneDepth targets. RHI5/RHI6 connects that work to the Environment viewport path without returning to the child HWND swapchain that caused resize flicker.

## Architecture

The active path is:

```text
Aquarium runtime
  -> AceAquariumSceneAdapter::BuildPrimitives
  -> AceAquariumGpuViewportRenderer
      -> GPU mesh vertices
      -> DX12 offscreen SceneColor / SceneDepth
      -> DrawInstanced
      -> readback BGRA8 snapshot
  -> main HWND D2D bitmap composition
```

This keeps the key architectural rule:

```text
No child HWND in the main Environment viewport path.
```

The viewport is still composed by the main shell HWND, but the grid/blocks/agent are now rendered by the GPU instead of the older CPU-projected D2D bridge.

## RHI5: scene texture readback/composition bridge

`Dx12Device` now exposes:

```cpp
bool readbackBgra8(Texture source, std::vector<U32>* pixels, Extent2D* extent, std::string* error);
```

It uses:

```text
- ID3D12Device::GetCopyableFootprints
- readback heap
- CopyTextureRegion
- ResourceBarrier to COPY_SOURCE
- fence wait through the existing command execution path
- row-by-row copy into BGRA8 CPU pixels
```

This is not the final zero-copy composition path. The next presentation milestone should replace readback with DComp/shared texture composition. But RHI5 is intentionally conservative and testable: the GPU renders the viewport scene and the main HWND receives a stable bitmap without child-window flicker.

## RHI6: Aquarium GPU viewport renderer

New files:

```text
Source/Public/ArhqenCognitionEngine/Renderer/Scene/AceAquariumGpuViewportRenderer.h
Source/Private/Renderer/Scene/AceAquariumGpuViewportRenderer.cpp
```

The renderer owns:

```text
- Dx12Device
- offscreen SceneColor
- offscreen SceneDepth
- persistent vertex buffer
- basic color pipeline
- GPU snapshot/readback stats
```

It converts Aquarium primitives to GPU triangles:

```text
- bounded grid -> thin GPU rectangles
- Tile -> two triangles
- Block -> body + highlight strip
- Agent -> glow/body rectangles
- DirectionArrow -> thick line + endpoint marker
- Highlight -> translucent rectangle
- DebugLabel -> marker only when Debug Truth is enabled
```

This path avoids the old D2D projected-grid movement problem. The old AQ3D12 D2D bridge remains as fallback if DX12 initialization or readback fails.

## Shell integration

`AceShellUi::renderAquariumSlateCompositeViewport` now tries the GPU viewport first:

```text
- builds Aquarium primitives
- renders them through AceAquariumGpuViewportRenderer
- creates a D2D bitmap from the BGRA8 snapshot
- draws that bitmap into the viewport rect
- draws a small overlay label
- returns before the old CPU-projected D2D path
```

If the GPU path fails, it falls through to the existing D2D projected path. The user gets a viewport instead of a black rectangle, because apparently graceful fallback is frowned upon only by people who enjoy debugging at 3 AM.

## Current limitation

RHI5 uses GPU readback for composition:

```text
DX12 SceneColor -> readback heap -> D2D bitmap -> main HWND
```

This is stable and simple, but not the final high-FPS path. Readback can limit performance. It exists to prove the full scene-rendering pipeline and main-HWND integration before moving to a zero-copy present path.

## Next milestone

```text
ACE-RHI7 = DirectComposition/shared-texture composition
```

Recommended next step:

```text
- expose SceneColor as a composition surface
- avoid CPU readback
- keep main HWND
- no child HWND
- keep DX12 GPU mesh viewport
```

After that:

```text
ACE-RHI8 = camera constants + world/view/projection
ACE-RHI9 = material/shader cache + texture sampling
ACE-RHI10 = terrain/water/particles v1
```

## Validation

Run:

```powershell
.\Tools\validate_ace_rhi5_rhi6.ps1
.\Tools\validate_ace_rhi3_rhi4.ps1
.\Tools\validate_ace_rhi1_rhi2.ps1
.\Tools\validate_ace_rhi0.ps1
.\Scripts\build_release.ps1
```
