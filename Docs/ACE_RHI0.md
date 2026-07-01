# ACE-RHI0 / ACE-AQ3D13 Renderer Core Foundation

## Scope

ACE-RHI0 starts a real renderer architecture for Arhqen. This is not an Aquarium-only patch and not another D2D fake-3D workaround.

The UE renderer/RHI source was used as architecture reference only. No UE code was copied.

## What was added

```text
Renderer/RHI/AceRhi
Renderer/Scene/AceRenderScene
Renderer/Core/AceRenderer
Tools/AceRhi0RendererCoreProbe.cpp
Tools/validate_ace_rhi0.ps1
```

## Architecture

```text
Renderer facade
  -> RHI device interface
      -> resource registry
      -> command lists
      -> render graph
      -> backend implementation
  -> scene renderer
      -> render scene
      -> mesh batches
      -> render view
```

## RHI foundation

The RHI layer now has backend-neutral concepts:

```text
buffers
textures
samplers
shaders
graphics pipelines
render passes
resource access states
command lists
render graph passes
null backend
```

## Why this exists

The previous Environment work proved two things:

```text
child HWND + DX12 swapchain = resize flicker
D2D CPU projected 3D = no flicker, but weak renderer
```

The correct path is a real renderer core that can eventually do:

```text
single HWND
DX12 offscreen scene texture
proper depth
proper command recording
proper frame ownership
UI composition over/with the render target
```

## UE source lesson

The useful lesson from UE is architectural separation, not copy-paste:

```text
RHI resources are not UI widgets
viewport/backbuffer ownership is explicit
resize is a render lifecycle event
scene rendering is separated from platform presentation
command recording is separated from command execution
```

## Non-goals in RHI0

```text
no real DX12 backend yet
no descriptor heaps yet
no shader compiler/cache yet
no D3D12 resource upload yet
no render thread yet
no material graph
no terrain/water/particles
```

## Next milestones

```text
ACE-RHI1  DX12 backend shell
ACE-RHI2  descriptor heaps + upload buffers
ACE-RHI3  offscreen scene color/depth targets
ACE-RHI4  single-HWND composition of DX12 texture
ACE-RHI5  mesh/grid viewport renderer v1
ACE-RHI6  frame pacing + render thread
```

## Validation

```powershell
.\Tools\validate_ace_rhi0.ps1
.\Scripts\build_release.ps1
```
